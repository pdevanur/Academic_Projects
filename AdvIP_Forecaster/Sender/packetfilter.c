#include "packetfilter.h"
//#include <unistd.h>


#define NF_IP_POST_ROUTING 4
//#define NF_IP_LOCAL_IN 1

//static struct nf_hook_ops fin;
short iPktCnt;
static struct nf_hook_ops fout;
static  struct sk_buff *sb ;
static unsigned long long int tmpstmp=0;

extern unsigned int cpu_khz;


//
//Udp client part
//
//The ksocket wrapper functions are used from ksocket-0.0.0.1
//BSD socket APIs implementation
ksocket_t ksocket(int domain, int type, int protocol)
{
	struct socket *sk = NULL;
	int ret = 0;
	
	ret = sock_create(domain, type, protocol, &sk);
	if (ret < 0)
	{
		ksocket_debug("sock_create failed\n");
		return NULL;
	}
	
	ksocket_debug("sock_create sk= 0x%p\n", sk);
	
	return sk;
}



int kbind(ksocket_t socket, struct sockaddr *address, int address_len)
{
	struct socket *sk;
	int ret = 0;

	sk = (struct socket *)socket;
	ret = sk->ops->bind(sk, address, address_len);
	ksocket_debug("kbind ret = %d\n", ret);
	
	return ret;
}

int klisten(ksocket_t socket, int backlog)
{
	struct socket *sk;
	int ret;

	sk = (struct socket *)socket;
	
	if ((unsigned)backlog > SOMAXCONN)
		backlog = SOMAXCONN;
	
	ret = sk->ops->listen(sk, backlog);
	
	return ret;
}


ksocket_t kaccept(ksocket_t socket, struct sockaddr *address, int *address_len)
{
	struct socket *sk;
	struct socket *new_sk = NULL;
	int ret;
	
	sk = (struct socket *)socket;

	ksocket_debug("family = %d, type = %d, protocol = %d\n",
					sk->sk->sk_family, sk->type, sk->sk->sk_protocol);
	//new_sk = sock_alloc();
	//sock_alloc() is not exported, so i use sock_create() instead
	ret = sock_create(sk->sk->sk_family, sk->type, sk->sk->sk_protocol, &new_sk);
	if (ret < 0)
		return NULL;
	if (!new_sk)
		return NULL;
	
	new_sk->type = sk->type;
	new_sk->ops = sk->ops;
	
	ret = sk->ops->accept(sk, new_sk, 0 /*sk->file->f_flags*/);
	if (ret < 0)
		goto error_kaccept;
	
	if (address)
	{
		ret = new_sk->ops->getname(new_sk, address, address_len, 2);
		if (ret < 0)
			goto error_kaccept;
	}
	
	return new_sk;

error_kaccept:
	sock_release(new_sk);
	return NULL;
}




ssize_t ksendto(ksocket_t socket, void *message, size_t length,
              int flags, const struct sockaddr *dest_addr,
              int dest_len)
{
	struct socket *sk;
	struct msghdr msg;
	struct iovec iov;
	int len;
	mm_segment_t old_fs;

	sk = (struct socket *)socket;

	iov.iov_base = (void *)message;
	iov.iov_len = (__kernel_size_t)length;

	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;
	msg.msg_control = NULL;
	msg.msg_controllen = 0;

	msg.msg_flags = flags;
	if (dest_addr)
	{
		msg.msg_name = (void *)dest_addr;
		msg.msg_namelen = dest_len;
	}

	old_fs = get_fs();
	set_fs(KERNEL_DS);
	len = sock_sendmsg(sk, &msg, length);//?
	set_fs(old_fs);
	
	return len;//len ?
}


ssize_t krecv(ksocket_t socket, void *buffer, size_t length, int flags)
{
	struct socket *sk;
	struct msghdr msg;
	struct iovec iov;
	int ret;
	mm_segment_t old_fs;

	sk = (struct socket *)socket;

	iov.iov_base = (void *)buffer;
	iov.iov_len = (__kernel_size_t)length;

	msg.msg_name = NULL;
	msg.msg_namelen = 0;
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;
	msg.msg_control = NULL;
	msg.msg_controllen = 0;

	/*
	 * msg.msg_iov->iov_base is declared as follows:
	 * void __user *iov_base;
	 * which means there is an user space pointer in 'msg'
	 * use set_fs(KERNEL_DS) to make the pointer available to kernel space
	 */
	old_fs = get_fs();
	set_fs(KERNEL_DS);
	ret = sock_recvmsg(sk, &msg, length, flags);
	set_fs(old_fs);
	if (ret < 0)
		goto out_krecv;
	//ret = msg.msg_iov.iov_len;//?
	
out_krecv:
	return ret;

}


int recv_fullmsg(ksocket_t in_sock,void *buf,int pktlen,int flags)
{

       int offset=0,iError;


       while(offset<pktlen)
       {
               iError = krecv(in_sock,buf+offset,pktlen-offset,flags);
               if(iError <= 0)
               {
                       if(iError != -1 * EAGAIN)
                               return iError;
               }
               else
               {
                       //printk(KERN_INFO "Got a packet of %d bytes\n",iError);
                       offset+=iError;
               }

       }
       return offset;

}

int ksetsockopt(ksocket_t socket, int level, int optname, void *optval, int optlen)
{
	struct socket *sk;
	int ret;
	mm_segment_t old_fs;

	sk = (struct socket *)socket;
	
	old_fs = get_fs();
	set_fs(KERNEL_DS);

	if (level == SOL_SOCKET)
		ret = sock_setsockopt(sk, level, optname, optval, optlen);
	else
		ret = sk->ops->setsockopt(sk, level, optname, optval, optlen);
	
	set_fs(old_fs);

	return ret;
}

int kclose(ksocket_t socket)
{
	struct socket *sk;
	int ret;

	sk = (struct socket *)socket;
	ret = sk->ops->release(sk);

	if (sk)
		sock_release(sk);

	return ret;
}

unsigned int inet_addr(char *str)
{
    int a,b,c,d;
    char arr[4];
    sscanf(str,"%d.%d.%d.%d",&a,&b,&c,&d);
    arr[0] = a; arr[1] = b; arr[2] = c; arr[3] = d;
    return *(unsigned int*)arr;
}

ksocket_t forecaster_sd;
ksocket_t forecaster_clsd;


void UdpClient (void *arg)
{
	ksocket_t stSockfdCli;
	struct sockaddr_in stAddrSrv;
	struct sockaddr_in stAddrCli;
	int i, opt=1;        
	int iAddrLen;
    int iID = 0;
   	stamp pktbuf; 

    iUdpRun = 1;

    printk("Server Starting....\n");
    /* kernel thread initialization */
    lock_kernel();
    /* daemonize (take care with signals, after daemonize() they are disabled) */
    daemonize("UdpClient");
    allow_signal(SIGKILL);	/*statement for signal handling to all SIGKILL*/
    unlock_kernel();
    sprintf(current->comm, "UdpClient");

	stSockfdCli = NULL;
    /*create a UDP socket*/	
	stSockfdCli = ksocket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (stSockfdCli == NULL)
	{
		printk("socket creation failed\n");
		return;
	}
	
	/*This options is to make the ports reusable so that the sockets can be bounded again the next time*/
	ksetsockopt(stSockfdCli,SOL_SOCKET,SO_REUSEADDR, (char *)&opt,sizeof(opt)); 

	memset(&stAddrSrv, 0, sizeof(stAddrSrv));
	memset(&stAddrCli, 0, sizeof(stAddrCli));
	stAddrCli.sin_family = AF_INET;
	stAddrCli.sin_port = htons(DEST_PORT);
	stAddrCli.sin_addr.s_addr = inet_addr(SERV_IP);
	iAddrLen = sizeof(struct sockaddr_in);

//	buf[0]=0;
	pktbuf.marker=0;
    iPktCnt=0;

    for (i = 0; i < (MAX_PCKT*2 + 200); i++) 
    {
        //printk("Sending packet %d\n", i);

    pktbuf.id = i;
	if(i==400)	
	pktbuf.marker=1;

        //sprintf(buf+1, "This is packet %d\n", i);
        if (ksendto(stSockfdCli, (void *)&pktbuf,sizeof(stamp), 0, (struct sockaddr *)&stAddrCli, iAddrLen)==-1)
        {
            return;
        }
	if(i<199)
		msleep(250);
	else if(i==199)
		msleep(5000);
	else if(i%4==0)
		msleep(1);	
   
   // if(i==230)
    {
       // i+=50;
    }
    
    }

/*8	printk("Going into krecv\n");
	for (i = 0; i < MAX_PCKT+1; i++) 
	{	
	krecv(stSockfdCli,(void *)buf,BUFLEN,0);
		printk("Got from krecv:%s\n",buf+1);
	}
	printk("OUt of krecv\n");	
*/
    /*Close socket*/
	kclose(stSockfdCli); 
    iUdpRun = 0;
	return;
} 

void PollThread (void *arg)
{
	int iPrev=0;
    //printk("Deamon thread started...\n");
	do
    {
	    //sleep for 5 sec	
        ssleep(5);
		if(iPrev != start)
		{   //if the start is one and the UDP server thread is not running
			if(start == 1 && iUdpRun == 0)
			{
                /* start UDP server thread */
                pstUdpClnt = kthread_run((void *)UdpClient, NULL, "UdpClnt");
                if (IS_ERR(pstUdpClnt)) 
                {
                    printk("UdpClnt: unable to start kernel thread\n");
                    return;
                }
			}
            iPrev = start;
        }
    } while(on == 1);
    iPoll = 0;
}

//
//packet filter part
//
unsigned int myhook(unsigned int hooknum,
		    struct sk_buff *skb,
		    const struct net_device *in,
		    const struct net_device *out,
		    int (*okfn)(struct sk_buff *)) {

  static struct iphdr *iph;
  static struct udphdr *udph;
  char *tip;
  unsigned int udplen;
  stamp *lay;
  
  //Assembly code to get the cpu cycle the opcode is corresponding to RDTSC
  __asm__ volatile (".byte 0x0f, 0x31" : "=A" (tmpstmp));


  sb = skb;
  iph=(struct iphdr*)sb->network_header;
  tip = (char*)iph;
  udph=(struct udphdr*)(tip + 4*(__u8)iph->ihl);
  udplen = sb->len - (iph->ihl<<2);
  lay= (void*)udph + 8;
  //printk(KERN_INFO "### UDPH got %d %d\n",ntohs(udph->source),ntohs(udph->dest));
  
 
 
  //We will print as SRCADR DESTADR IPID CPUCYCLE format our script will extract and print properly
  if(iph->protocol == IPPROTO_UDP && ntohs(udph->dest) == DEST_PORT ) {
    
    //add the IP ID for each packet since it is 0 for all the UDP packets
    //if (iPktCnt >= MAX_PCKT)
      //  iPktCnt = 0;

    //printk ("%d\n", iPktCnt);
    iph->id = (iPktCnt++);
    lay->tmpstmpsrc = tmpstmp;

    iph->check = 0;
    iph->check = ip_fast_csum((unsigned char *) iph, iph->ihl);

    udph->check = 0;
	udph->check = csum_tcpudp_magic(iph->saddr, iph->daddr, udplen,IPPROTO_UDP, csum_partial((char *)udph, udplen,0));
    
   // printk(KERN_INFO "### %d %d %d %Ld %Ld\n",iph->saddr,iph->daddr,iph->id,tmpstmp, lay->tmpstmpsrc);     
  }
  return NF_ACCEPT;
}

int init_module(void) {
  //For sender enable NF_IP_POST_ROUTING 
  fout.hook = myhook ;
  fout.pf = PF_INET ;
  fout.hooknum = NF_IP_POST_ROUTING ;
  fout.priority = NF_IP_PRI_FIRST;
  nf_register_hook(&fout);

  //For  receiver enable NF_IP_LOCAL_IN , NF_IP_PRE_ROUTING 
  //i think gets called for broadcast  message also.IPPROTO_TCP

  //For receiver 
  /*fin.hook = myhook ;
  fin.pf = PF_INET ;
  fin.hooknum = NF_IP_LOCAL_IN;
  fin.priority = NF_IP_PRI_FIRST;  
  nf_register_hook(&fin);*/
  printk(KERN_INFO "###SRCADR DESTADR IPID TIMESTMP\n");
	msleep(1);




    //pstUdpsrv = kthread_run((void *)tcp_srv, NULL, "TcpSrv");
    //create thread to receive 700 packets
    pstUdpClnt = kthread_run((void *)UdpClient, NULL, "UdpClnt");


    //create the polling thread to send the 700 packets as required
 //   pstPollThd = kthread_run((void *)PollThread, NULL, "UdpClnt");

  return 0;
}

void cleanup_module(void) {
  //Disable Receiver Hook
  //nf_unregister_hook(&fin);

  //signal the poll thread to exit
  on = 0;
  //Disable Sender Hook
  nf_unregister_hook(&fout);
printk("cleanup_module");
 // while (0 != iPoll)
  //  ssleep(5);
//printk("iPoll=%d", iPoll);
 /* if (iPoll)
    {
        if((kill_proc(pstPollThd->pid, SIGKILL, 1))<0)
        {
            printk("Error while trying to kill the thread\n");
        }
    }
*/
}

//module_init (init_module);
//module_exit (cleanup_module);

