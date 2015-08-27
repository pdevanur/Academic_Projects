/* Author: Prasanna Jeevan Devanur Nagabhushan
   Unity ID: pdevanu
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
	unsigned short int iUdpLen, iTcpLen;

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
        
		//if the packet is for public IP of the relay server change the Dest port(#80) to Relay server listening port
		if (pstIph->daddr == inet_addr(pPubIP->data))
		{
	        //If the request is not for port #80, do nothing
		    if (REAL_SRV_PORT == ntohs(pstTcpHdr->source))
    		    return NF_DROP;
		
//            printk (KERN_INFO "relay server port(before) = %d\n", pstTcpHdr->dest);
			pstTcpHdr->dest = htons((short int)StrToInt(pListenPort->data));
//            printk (KERN_INFO "relay server port = %d\n", ntohs(pstTcpHdr->dest));

            //Store the Source addr and port in a table
            stTbl[iTblIdx].iSrcIP = pstIph->saddr;
            stTbl[iTblIdx].iSrcPort = pstTcpHdr->source;
            
  //          printk (KERN_INFO "source server ip = %s\n", inet_ntoa(stTbl[iTblIdx].iSrcIP));

            if (iTblIdx == (MAX_TBL_IDX - 1))
                iTblIdx = 0;
            else
                iTblIdx++;
		}	
		
		//If the source IP is real server IP then change the dest IP to krelay IP
		else if (pstIph->saddr == inet_addr(pRealIP->data))
		{
			pstIph->daddr = inet_addr(pPrivIP->data);
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
	unsigned int iUdpLen, iTcpLen;

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

		pstUdpHdr = (struct udphdr*)((char*)pstIph+ (pstIph->ihl<<2));
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
        
		//if the packet is for real server change the Source Addr to that of client
		if (pstIph->daddr == inet_addr(pRealIP->data))
		{
			pstIph->saddr = stTbl[iCurIdx].iSrcIP;

            /*if (iCurIdx == (MAX_TBL_IDX - 1))
                iCurIdx = 0;
            else
                iCurIdx++;			*/
		}	

		//If the packet is for Client IP then change the Source port to Original port for which the request came
		else if (pstIph->saddr == inet_addr(pPubIP->data))
		{
			pstTcpHdr->source = htons(REAL_SRV_PORT);
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

static int my_module_init(void)
{
    create_config_file ();
    HookReg ();

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
    nf_unregister_hook(&stPreHook);
    nf_unregister_hook(&stPostHook);
    remove_config_file ();
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

    //for the Real Server IP config
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
}

void remove_config_file (void)
{
    remove_proc_entry("start", pConfigDir);
    remove_proc_entry("gport", pConfigDir);
    remove_proc_entry("real_ip", pConfigDir);
    remove_proc_entry("private_ip", pConfigDir);
    remove_proc_entry("public_ip", pConfigDir);
    remove_proc_entry("krelay", &proc_root);
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

