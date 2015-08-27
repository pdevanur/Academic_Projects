#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/ip.h>
#include <linux/in.h>
#include <linux/netdevice.h>
#include <linux/udp.h>

//#define NF_IP_POST_ROUTING 4
#define NF_IP_LOCAL_IN 1

static struct nf_hook_ops fin;
//static struct nf_hook_ops fout;
static  struct sk_buff *sb ;
static unsigned long long int tmpstmp=0;

extern unsigned int cpu_khz;

struct stamp
{
	int marker;
	int source;
	int destination;
	int id;
	unsigned long long int tmpstmpsrc;
	unsigned long long int tmpstmp;
	char padding[1400];
	struct stamp *link;
};
typedef struct stamp stamp;



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
  lay = (void*)udph + 8;
  
  //printk(KERN_INFO "### UDPH got %d %d\n",ntohs(udph->source),ntohs(udph->dest));
  
  
  //We will print as SRCADR DESTADR IPID CPUCYCLE format our script will extract and print properly
  //printk(KERN_INFO "%d\n", ntohs(udph->dest));
  if(iph->protocol == IPPROTO_UDP && ntohs(udph->dest) == 6001 ) {

    lay->tmpstmp = tmpstmp;
    
    //recalculate the udp checksum
    udph->check = 0;
	udph->check = csum_tcpudp_magic(iph->saddr, iph->daddr, udplen,IPPROTO_UDP, csum_partial((char *)udph, udplen,0));

    //printk(KERN_INFO "### %d %d %d %Ld %Ld\n",iph->saddr,iph->daddr,iph->id, lay->tmpstmpsrc, tmpstmp);     
   // printk(KERN_INFO "### %d %d %d %Ld\n",iph->saddr,iph->daddr,iph->id, lay->tmpstmpsrc);
  }
  return NF_ACCEPT;
}

int init_module() {
  //For sender enable NF_IP_POST_ROUTING 
/*  fout.hook = myhook ;
  fout.pf = PF_INET ;
  fout.hooknum = NF_IP_POST_ROUTING ;
  fout.priority = NF_IP_PRI_FIRST;
  //nf_register_hook(&fout);*/

  //For  receiver enable NF_IP_LOCAL_IN , NF_IP_PRE_ROUTING 
  //i think gets called for broadcast  message also.IPPROTO_TCP

  //For receiver 
  fin.hook = myhook ;
  fin.pf = PF_INET ;
  fin.hooknum = NF_IP_LOCAL_IN;
  //fin.priority = NF_IP_PRI_FIRST;  
  nf_register_hook(&fin);
  printk(KERN_INFO "###SRCADR DESTADR IPID TIMESTMP\n");

  //create thread to 700 packets
  
  
  return 0;
}

void cleanup_module() {
  //Disable Receiver Hook
  nf_unregister_hook(&fin);

  //Disable Sender Hook
  //nf_unregister_hook(&fout);

}

