/* 
   Description: Has all the functions required for a simple kernel TCP/UDP relay server
   version: 1.0
*/

#include "krelay.h"

MODULE_LICENSE("GPL");

/* This is the hook function for processing incoming packets from Client */
static unsigned int FromClient(unsigned int hooknum,
	         struct sk_buff **skb,
	         const struct net_device *in,
	         const struct net_device *out,
	         int (*okfn)(struct sk_buff *))
{
	struct sk_buff *pstSkBuff;
	struct iphdr *pstIph;
	struct tcphdr *pstTcpHdr ;
	struct udphdr *pstUdpHdr;
	unsigned short int iUdpLen, iTcpLen, cur_idx, dup = 0;

	pstSkBuff = *skb;
	pstIph = ip_hdr(pstSkBuff);
	
	//printk (KERN_INFO "In  Pre Hook func\n");

    if (pstIph->protocol == IPPROTO_UDP) 
	{
		pstUdpHdr = (struct udphdr*)((char*)pstIph + (pstIph->ihl<<2));

		//if the packet is for public IP of the relay server change the Dest IP to Real server IP
		if (pstIph->daddr == inet_addr(pPubIP->data))
		{
			pstIph->daddr = inet_addr(pRealIP->data);
		}	
		
		else
		{
			return NF_ACCEPT;
		}

//		printk (KERN_INFO "Dest Addr = %s\n", inet_ntoa(pstIph->daddr));
		//Get the UDP Packet len to calculate the UDP checksum
		iUdpLen = pstSkBuff->len - (pstIph->ihl<<2);

		//recalculate the IP and UDP header
		pstIph->check = 0;
		pstIph->check = ip_fast_csum((unsigned char *)pstIph, pstIph->ihl);

		pstUdpHdr->check = 0;
		pstUdpHdr->check = csum_tcpudp_magic(pstIph->saddr, pstIph->daddr,iUdpLen,IPPROTO_UDP, csum_partial((char *)pstUdpHdr, iUdpLen,0));
    }
	
    else if (pstIph->protocol == IPPROTO_TCP) 
	{
		pstTcpHdr = (struct tcphdr*)((char*)pstIph + (pstIph->ihl<<2));
        
		//FTP Handling
		//FTP Handling
		if (pstTcpHdr->dest == htons(20) || htons(21))
		{
			if (pstIph->daddr == inet_addr(pPubIP->data))
			{
				pstIph->daddr = inet_addr(pRealIP->data);
			}
		}
		else if (pstTcpHdr->dest != htons(20) || htons(21))
		{
		//if the packet is for public IP of the relay server change the Dest port(#80) to Relay server listening port
		if (pstIph->daddr == inet_addr(pPubIP->data))
		{

          		
       
            dup = 0;

            for (cur_idx = 0; cur_idx <= MAX_TBL_IDX -1; cur_idx++)
            {
                if(stTbl[cur_idx].iClntIP == pstIph->saddr)
                {				
                    if(ntohs(pstTcpHdr->source) == stTbl[cur_idx].iClntPort)
                    {
                        dup= 1; 
                        break;
                    }
                }
            }
            
            //Store if it is not from the duplicate client port (if different connection)
            if(dup == 0)
            {		
                //Store the Source addr and port in a table
                stTbl[iTblIdx].iClntIP = pstIph->saddr;
                stTbl[iTblIdx].iClntPort = pstTcpHdr->source;
                stTbl[iTblIdx].iClntSrvPort = pstTcpHdr->dest;
                
                if (iTblIdx == (MAX_TBL_IDX - 1))
                    iTblIdx = 0;
                else
                    iTblIdx++;
          
    			pstTcpHdr->dest = htons((short int)StrToInt(pListenPort->data));
    			

    			
		}
    			
    		
        	}		

	    //If the source IP is real server IP then change the dest IP to krelay IP
	    else if (pstIph->saddr == inet_addr(pRealIP->data))
	    {
		    pstIph->daddr = inet_addr(pPrivIP->data);
	    }
	}
	    else
	    {
		    return NF_ACCEPT;
	    }

	    //Get the TCP Packet len to calculate the TCP checksum
	    iTcpLen = pstSkBuff->len - (pstIph->ihl<<2);

	    //recalculate the IP and TCP header
	    pstIph->check = 0;
	    pstIph->check = ip_fast_csum((unsigned char *)pstIph, pstIph->ihl);

	    pstTcpHdr->check = 0;
	    pstTcpHdr->check = csum_tcpudp_magic(pstIph->saddr, pstIph->daddr,iTcpLen,IPPROTO_TCP, csum_partial((char *)pstTcpHdr, iTcpLen,0));
    }
    
	return NF_ACCEPT;
}


/* This is the hook function for processing incoming packets from Server */
static unsigned int FromRealSrv(unsigned int hooknum,
	         struct sk_buff **skb,
	         const struct net_device *in,
	         const struct net_device *out,
	         int (*okfn)(struct sk_buff *))
{
	struct sk_buff *pstSkBuff = *skb;
	struct iphdr *pstIph;
	struct tcphdr *pstTcpHdr ;
	struct udphdr *pstUdpHdr;
	unsigned int iUdpLen, iTcpLen, iCnt;
    unsigned short client_port = 0;
    
	pstIph = ip_hdr(pstSkBuff);
	
//	printk (KERN_INFO "In Post Hook func\n");
	
    if (pstIph->protocol == IPPROTO_UDP) 
	{

		//if the packet is from Real Server IP then change the Source IP to Relay server Public IP
		if (pstIph->saddr == inet_addr(pRealIP->data))
		{
			pstIph->saddr = inet_addr(pPubIP->data);
		}	

		else
		{
			return NF_ACCEPT;
		}

		pstUdpHdr = (struct udphdr*)((char*)pstIph + (pstIph->ihl<<2));
		iUdpLen = pstSkBuff->len - (pstIph->ihl<<2);

		pstIph->check = 0;
		//recalculate the IP and UDP header
		pstIph->check = ip_fast_csum((unsigned char *)pstIph, pstIph->ihl);

		pstUdpHdr->check = 0;
		pstUdpHdr->check = csum_tcpudp_magic(pstIph->saddr, pstIph->daddr,iUdpLen,IPPROTO_UDP, csum_partial((char *)pstUdpHdr, iUdpLen,0));
    }

    else if (pstIph->protocol == IPPROTO_TCP) 
	{
		pstTcpHdr = (struct tcphdr*)((char*)pstIph + (pstIph->ihl<<2));
        	
		//FTP Handling
		if (pstTcpHdr->source == htons(20) || htons(21))
		{
			if (pstIph->saddr == inet_addr(pRealIP->data))
			{
				pstIph->saddr = inet_addr(pPubIP->data);
			}
		}
		else if (pstTcpHdr->dest != htons(20) || htons(21))
		{
		//if the packet is for real server change the Source Addr to that of client
		if (pstIph->daddr == inet_addr(pRealIP->data))
		{
			//change this part.....
			//Assign the Client IP address to maintain transperency when sending a request to Real server from Relay
			for (iCnt = 0; iCnt <  (MAX_TBL_IDX - 1); iCnt++)
			{
			    if (stTbl[iCnt].iRelayPort == pstTcpHdr->source)
        		{
        			pstIph->saddr = stTbl[iCnt].iClntIP;
        			break;
        		}
            }
        }
            
		//If the packet is for Client IP then change the Source port to Original port for which the request came
		else if (pstIph->saddr == inet_addr(pPubIP->data))
		{
              for (iCnt = 0; iCnt <= MAX_TBL_IDX-1; iCnt++) //iTblIdx
              {
                if(stTbl[iCnt].iClntPort == pstTcpHdr->dest)
                {
                    client_port = stTbl[iCnt].iClntSrvPort;
                    break;
                }       
              }

              if (0 == client_port)
              {
                printk(KERN_ERR "Invalid Client Port\n");
                return NF_ACCEPT;
              }
              
			pstTcpHdr->source = client_port;		
		
//			pstTcpHdr->source = htons(REAL_SRV_PORT);
		}
		}
		else
		{
			return NF_ACCEPT;
		}

//		printk (KERN_INFO "Dest Addr = %s\n", inet_ntoa(pstIph->daddr));
		
		//Get the TCP Packet len to calculate the TCP checksum
		iTcpLen = pstSkBuff->len - (pstIph->ihl<<2);

		//recalculate the IP and TCP header
		pstIph->check = 0;
		pstIph->check = ip_fast_csum((unsigned char *)pstIph, pstIph->ihl);

		pstTcpHdr->check = 0;
		pstTcpHdr->check = csum_tcpudp_magic(pstIph->saddr, pstIph->daddr,iTcpLen,IPPROTO_TCP, csum_partial((char *)pstTcpHdr, iTcpLen,0));
    }
	
	return NF_ACCEPT;
}

static void HookReg ()
{
	/* Fill in Pre hook structure */
	stPreHook.hook     = FromClient;
	/* Handler function */
	stPreHook.hooknum  = NF_IP_PRE_ROUTING;
	stPreHook.pf       = PF_INET;
	//stPreHook.priority = NF_IP_PRI_FIRST;
	nf_register_hook(&stPreHook);

	/* Fill in Post hook structure */
	stPostHook.hook     = FromRealSrv;
	// Handler function
	stPostHook.hooknum  = NF_IP_POST_ROUTING;
	stPostHook.pf       = PF_INET;
	//stPostHook.priority = NF_IP_PRI_FIRST;
	nf_register_hook(&stPostHook);
	
	return;
}

/* These has all the functions related to the TCP-Relay server handling part*/
/****************************************************************************/
/****************************************************************************/

/*this handles the redirection from client to real server*/
void RedirectProc (ksocket_t SockfdFromClnt)
{
    struct sockaddr_in RealSrvAddr, stRelayAddr, stTemp1, stTemp;
    ksocket_t SockfdRelay;
    int on = 1;
    char frwd_buffer[BUFFER_SIZE];
    int iRBytes, iWBytes, iNWritten, cur, iLen;
    unsigned short iClntPort, server_port = 0;

    
    /* kernel thread name*/
    sprintf(current->comm, "krelaychld");

    allow_signal(SIGKILL);
    allow_signal(SIGTERM);

	
	//Copy the values for later use
    if ((kgetpeername(SockfdFromClnt, (struct sockaddr *)&stTemp1, &iLen)) < 0)
    {
        printk (KERN_ERR "kgetpeername() failed\n");
    }

    iClntPort = stTemp1.sin_port;

    /*Create socket*/
    if ((SockfdRelay = ksocket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
	printk(KERN_ERR "Can't create socket.");

    memset(&stRelayAddr, 0, sizeof(stRelayAddr));
    stRelayAddr.sin_family      = AF_INET;
    stRelayAddr.sin_addr.s_addr = INADDR_ANY;
    stRelayAddr.sin_port        = htons(0);

    if (ksetsockopt(SockfdRelay, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0)
    {
    	printk(KERN_ERR "ksetsockopt(SO_REUSEADDR) failed\n");
    }
  
    /*Bind our local addrress so that  client can send*/ 
    if (kbind(SockfdRelay, (struct sockaddr *) &stRelayAddr, sizeof(stRelayAddr)) < 0)
    {
        printk(KERN_ERR "Can't bind local address\n");
        return;
    }
    //printk (KERN_ERR "Relay eph port= %d", ntohs(stRelayAddr.sin_port));
    //Need to analyze this part@@@@@@@@@   
    //Store the relay server ephemeral port (Y) in NAT
    if (kgetsockname(SockfdRelay, (struct sockaddr *) &stTemp, &iLen) < 0)
    {
    	printk(KERN_ERR "kgetpeername failed\n");
    }

   // printk (KERN_ERR "Relay eph port= %d", ntohs(stTemp.sin_port));

	for (cur = 0; cur <= MAX_TBL_IDX -1; cur++)
	{
		if (stTbl[cur].iClntPort == iClntPort)
		{
			stTbl[cur].iRelayPort = stTemp.sin_port;
			server_port = stTbl[cur].iClntSrvPort;
			break;
		}

	}

 	if (0 == server_port)
    	{
       		 printk (KERN_ERR "Error..Cannot get the server port\n");
       		 kclose (SockfdRelay);
        	return;
   	 }	
    
    //Connect to the Real server
    RealSrvAddr.sin_family = PF_INET;
    RealSrvAddr.sin_addr.s_addr = (inet_addr(DEF_REAL_IP));
    RealSrvAddr.sin_port = server_port;
    
    if (kconnect(SockfdRelay, (struct sockaddr *) &RealSrvAddr, sizeof(RealSrvAddr)) < 0)
    {
    	printk(KERN_ERR "Connect failed\n");
    }


   
    
    for (;;)
    {
    	//printk (KERN_ERR "trying krecv from client\n");

	  	/* Read from Client and send to Real Server */
        iRBytes = krecv(SockfdFromClnt, &frwd_buffer, BUFFER_SIZE - 8, MSG_DONTWAIT);

		//printk (KERN_ERR "tried krecv = %d\n", iRBytes);

        //Client Socket is closed
        if (iRBytes == 0)
            break;
            
		iWBytes = 0;
		iNWritten = 0;
		//If some data is there to be read
		while (iWBytes != iRBytes && iRBytes > 0)
		{
			if ((iNWritten = ksend(SockfdRelay, &frwd_buffer + iWBytes , iRBytes-iWBytes, 0)) < 0)
			{
				printk (KERN_ERR "error occured while writing to receiver\n");
				return;
			}
			iWBytes += iNWritten;
		}


	    //printk (KERN_ERR "trying krecv from real server\n");

        /* Read from Real server and write to client */
        iRBytes = krecv(SockfdRelay, &frwd_buffer, BUFFER_SIZE - 8, MSG_DONTWAIT);
	
	    //printk (KERN_ERR "tried krecv1 = %d\n", iRBytes);
        if (iRBytes == 0)
            break;

		iWBytes = 0;
		iNWritten = 0;
		while (iWBytes != iRBytes && iRBytes > 0)
		{
			if ((iNWritten = ksend(SockfdFromClnt, &frwd_buffer + iWBytes , iRBytes-iWBytes, 0)) < 0)
			{
				printk (KERN_ERR "error occured while writing to receiver\n");
				return;
			}
			iWBytes += iNWritten;
		}

        //Sleep for sometime
	//    mdelay (1);
	}

	(void)kclose(SockfdRelay);
	(void)kclose(SockfdFromClnt);
}


/*This function creates a server process listening at the socket rport*/
int tcp_serv (void *arg) 
{
    struct sockaddr_in relay_srv_addr, stClntAddr;
    int iAddrlen;
    int on = 1;
    ksocket_t sockfd_chld;

    /* kernel thread name*/
    sprintf(current->comm, "krelaysrv");
    allow_signal(SIGKILL);
    allow_signal(SIGTERM);

    /*Create socket*/
    if ((SockfdRelayClntSide = ksocket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
	printk(KERN_ERR "Can't create socket.");

    memset(&relay_srv_addr, 0, sizeof(relay_srv_addr));
    relay_srv_addr.sin_family      = AF_INET;
    relay_srv_addr.sin_addr.s_addr = INADDR_ANY;
    relay_srv_addr.sin_port        = htons(StrToInt((char *)(pListenPort->data)));

    if (ksetsockopt(SockfdRelayClntSide, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0)
    {
        printk(KERN_ERR "ksetsockopt(SO_REUSEADDR) failed\n");
    }
  
    /*Bind our local addrress so that  client can send*/ 
    if (kbind(SockfdRelayClntSide, (struct sockaddr *) &relay_srv_addr, sizeof(relay_srv_addr)) < 0)
    {
        printk(KERN_ERR "Can't bind local address\n");
        return -1;
    }

    /*  Make socket a listening socket  */
    if (klisten(SockfdRelayClntSide, 15) < 0)
    {
        printk(KERN_ERR "Call to listen failed.");
        return -1;
    }
   
     /*  Loop infinitely to accept and service connections  */
    while ( 1 ) 
    {
	/*  Wait for connection  */
        if ( (sockfd_chld = kaccept(SockfdRelayClntSide, (struct sockaddr *) &stClntAddr, &iAddrlen)) < 0 )
        {
            printk(KERN_ERR "Error calling accept()");
            return -1;
        }           
        
        ChldThdPid[iThdCnt] = kernel_thread ((void *)RedirectProc, sockfd_chld, 0);	
       
        if (iThdCnt == (MAX_THD_IDX - 1))
                    iThdCnt = 0;
        else
                    iThdCnt++;
                

        //Sleep for some time
  //      mdelay (1);

    }
    
    (void)kclose (SockfdRelayClntSide);
    return -1;    /*  We shouldn't get here  */
}






static int my_module_init(void)
{
    create_config_file ();
    HookReg ();
    RelThd = kernel_thread((void *)tcp_serv, NULL, 0);
    printk("krelay srv init ok\n");

    return 0;
}

void CheckStart (void *arg)
{
    int iStart, iStop = 0;
    
    daemonize ("chkstrt", 0);
    while (1)
    {
        iStart = StrToInt(pStart->data);
        if (1 == iStart)
        {
            if (1 == iStop)
            {
                iStop = 0;
                HookReg ();
            }
            mdelay (2000);
        }

        else if (0 == iStart)
        {
            if (0 == iStop)
            {
                iStop = 1;
                nf_unregister_hook(&stPreHook);
                nf_unregister_hook(&stPostHook);
            }
            mdelay (2000);
        }
    }
}


static void my_module_exit(void)
{
    int cnt=0;

    (void)kclose (SockfdRelayClntSide);
    nf_unregister_hook(&stPreHook);
    nf_unregister_hook(&stPostHook);
    remove_config_file ();
    for(cnt=0; cnt<=iThdCnt; cnt++){
       if(kill_proc(ChldThdPid[cnt],SIGKILL,1) == -ESRCH){
          printk(KERN_INFO "thread already killed\n");
       }
    }
   /* kill_proc(RelThd,SIGKILL,1);*/

    printk("krelay srv exit\n");
}


void create_config_file (void)
{
    pConfigDir = proc_mkdir("krelay", NULL);
    if(pConfigDir == NULL)
    {
        printk(KERN_INFO "Couldn't create proc dir\n");
        do_exit (0);
    }
    pConfigDir->owner = THIS_MODULE;
    
    //for the PORT config
    pListenPort = create_proc_entry( "gport", 0666, pConfigDir);
    if (pListenPort == NULL)
    {
        printk(KERN_INFO "Couldn't create proc entry\n");
        do_exit (0);
    }

    strcpy (stPort.arrValue, DEF_PORT);
    pListenPort->data = &stPort;
    pListenPort->read_proc = kproc_read_config;
    pListenPort->write_proc = kproc_write_config;
    pListenPort->owner = THIS_MODULE;

    //for the Real Server IP config 1
    pRealIP = create_proc_entry( "real_ip", 0666, pConfigDir);
    if (pRealIP == NULL)
    {
        printk(KERN_INFO "Couldn't create proc entry\n");
        do_exit (0);
    }

    strcpy (stRealIP.arrValue, DEF_REAL_IP);
    pRealIP->data = &stRealIP;
    pRealIP->read_proc = kproc_read_config;
    pRealIP->write_proc = kproc_write_config;
    pRealIP->owner = THIS_MODULE;

    //for the Real Server IP config 2
    pRealIP2 = create_proc_entry( "real_ip2", 0666, pConfigDir);
    if (pRealIP2 == NULL)
    {
        printk(KERN_INFO "Couldn't create proc entry\n");
        do_exit (0);
    }

    strcpy (stRealIP2.arrValue, DEF_REAL_IP2);
    pRealIP2->data = &stRealIP2;
    pRealIP2->read_proc = kproc_read_config;
    pRealIP2->write_proc = kproc_write_config;
    pRealIP2->owner = THIS_MODULE;

    //for the Real Server IP config 3
    pRealIP3 = create_proc_entry( "real_ip3", 0666, pConfigDir);
    if (pRealIP3 == NULL)
    {
        printk(KERN_INFO "Couldn't create proc entry\n");
        do_exit (0);
    }

    strcpy (stRealIP3.arrValue, DEF_REAL_IP3);
    pRealIP3->data = &stRealIP3;
    pRealIP3->read_proc = kproc_read_config;
    pRealIP3->write_proc = kproc_write_config;
    pRealIP3->owner = THIS_MODULE;

    //for the Real Server IP config
    pRealIP4 = create_proc_entry( "real_ip4", 0666, pConfigDir);
    if (pRealIP4 == NULL)
    {
        printk(KERN_INFO "Couldn't create proc entry\n");
        do_exit (0);
    }

    strcpy (stRealIP4.arrValue, DEF_REAL_IP4);
    pRealIP4->data = &stRealIP4;
    pRealIP4->read_proc = kproc_read_config;
    pRealIP4->write_proc = kproc_write_config;
    pRealIP4->owner = THIS_MODULE;

    //for the Public IP config
    pPubIP = create_proc_entry( "public_ip", 0666, pConfigDir);
    if (pPubIP == NULL)
    {
        printk(KERN_INFO "Couldn't create proc entry\n");
        do_exit (0);
    }

    strcpy (stPubIP.arrValue, DEF_PUB_IP);
    pPubIP->data = &stPubIP;
    pPubIP->read_proc = kproc_read_config;
    pPubIP->write_proc = kproc_write_config;
    pPubIP->owner = THIS_MODULE;

    //for the Private IP config
    pPrivIP = create_proc_entry( "private_ip", 0666, pConfigDir);
    if (pPrivIP == NULL)
    {
        printk(KERN_INFO "Couldn't create proc entry\n");
        do_exit (0);
    }

    strcpy (stPrivIP.arrValue, DEF_PRIV_IP);
    pPrivIP->data = &stPrivIP;
    pPrivIP->read_proc = kproc_read_config;
    pPrivIP->write_proc = kproc_write_config;
    pPrivIP->owner = THIS_MODULE;
    
    //for the Start config
    pStart = create_proc_entry( "start", 0666, pConfigDir);
    if (pStart == NULL)
    {
        printk(KERN_INFO "Couldn't create proc entry\n");
        do_exit (0);
    }

    strcpy (stStart.arrValue, DEF_START);
    pStart->data = &stStart;
    pStart->read_proc = kproc_read_config;
    pStart->write_proc = kproc_write_config;
    pStart->owner = THIS_MODULE;

     //for the No of servers config
    pServ = create_proc_entry( "no_serv", 0666, pConfigDir);
    if (pServ == NULL)
    {
        printk(KERN_INFO "Couldn't create proc entry\n");
        do_exit (0);
    }

    strcpy (stServ.arrValue, DEF_NO_SERV);
    pServ->data = &stServ;
    pServ->read_proc = kproc_read_config;
    pServ->write_proc = kproc_write_config;
    pServ->owner = THIS_MODULE;
}

void remove_config_file (void)
{
    remove_proc_entry("start", pConfigDir);
    remove_proc_entry("gport", pConfigDir);
    remove_proc_entry("real_ip", pConfigDir);
    remove_proc_entry("real_ip2", pConfigDir);
    remove_proc_entry("real_ip3", pConfigDir);
    remove_proc_entry("real_ip4", pConfigDir);	
    remove_proc_entry("private_ip", pConfigDir);
    remove_proc_entry("public_ip", pConfigDir);
    remove_proc_entry("no_serv", pConfigDir);
}

int kproc_read_config (char *page, char **start,
			    off_t off, int count, 
			    int *eof, void *data)
{
    int len;
    struct ConfigData *pstData = (struct ConfigData *)data;
    len = sprintf(page, "%s\n", pstData->arrValue);
    return len;
}

int kproc_write_config (struct file *file,
			     const char *buffer,
			     unsigned long count, 
			     void *data)
{
    int len;
    struct ConfigData *pstData = (struct ConfigData *)data;

    if(count > MAX_CONFIG_LENGTH)
        len = MAX_CONFIG_LENGTH;
    else
        len = count;	

    if(copy_from_user(pstData, buffer, len))
        return -EFAULT;

    pstData->arrValue[len-1] = '\0';

    return len;
}

/*int Read_File(char *Filename, int StartPos, char *Buffer)
{
	struct file 	*filp;
	mm_segment_t	oldfs;
	int		BytesRead;

	//Buffer = kmalloc(4096,GFP_KERNEL);
	if (Buffer==NULL) 
		return -1;
	
	filp = filp_open(Filename,00,O_RDONLY);
	if (IS_ERR(filp)||(filp==NULL))
		return -1;
      
        // File(system) doesn't allow reads
	if (filp->f_op->read==NULL)
		return -1;

	// Now read BUFSIZE (4096) bytes from postion "StartPos"
	filp->f_pos = StartPos;
	oldfs = get_fs();
	set_fs(KERNEL_DS);
	BytesRead = filp->f_op->read(filp, Buffer, BUFSIZE, &filp->f_pos);
	set_fs(oldfs);

	// Close the file 
	fput(filp);

        return BytesRead;
}*/

int StrToInt (char *str)
{
    int result = 0;
    char ch;
    /*Convert the string to integer*/
    while((ch = (*str++)))
    {
        result = (result * 10) + (ch-'0'); 
    }
    return result;
}

module_init(my_module_init);
module_exit(my_module_exit);


