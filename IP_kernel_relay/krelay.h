/* Author: Prasanna Jeevan Devanur Nagabhushan
   Unity ID: pdevanu
   Description: header file for krelay.c
   version: 1.0
*/

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/socket.h>
#include <linux/net.h>
#include <net/sock.h>
//#include <asm/processor.h>
//#include <asm/uaccess.h>
#include <linux/proc_fs.h>
#include <linux/vmalloc.h>
#include <linux/unistd.h>
#include <linux/syscalls.h>
#include <linux/delay.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/udp.h>
#include <linux/skbuff.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
//#include <net/checksum.h>
#include <linux/in.h>
#include <net/ksocket.h>
#include <linux/poll.h>

#define MAX_CONFIG_LENGTH   (24)
#define MAX_TBL_IDX         (120)
#define MAX_THD_IDX	    (300)
#define DEF_PORT	        "8086"
#define DEF_REAL_IP	        "192.1.1.129"
#define DEF_REAL_IP2	        "192.1.1.129"
#define DEF_REAL_IP3	        "192.1.1.129"
#define DEF_REAL_IP4	        "192.1.1.129"
#define DEF_PRIV_IP	        "192.1.1.128"
#define DEF_PUB_IP	        "10.1.1.128"
#define DEF_NO_SERV  		"4"
#define REAL_SRV_PORT        80
#define DEF_START           "1"
#define BUFFER_SIZE         1024

struct ConfigData
{
    char arrValue [MAX_CONFIG_LENGTH];
};

struct NatTbl
{
    unsigned int iClntIP;
    unsigned short iClntPort;
    unsigned short iRelayPort;
    unsigned short iClntSrvPort;
};

/*struct stToRedirect
{
    ksocket_t sockfd_chld;
    unsigned short iClntPort;
};*/
    
struct NatTbl stTbl[MAX_TBL_IDX];
int iTblIdx = 0;
int iCurIdx = 0;

static struct proc_dir_entry *pConfigDir, *pListenPort, *pRealIP, *pRealIP2, *pRealIP3, *pRealIP4, *pPubIP, *pPrivIP, *pStart, *pServ ;
static struct ConfigData stPort, stRealIP, stRealIP2, stRealIP3, stRealIP4, stPubIP, stPrivIP, stStart, stServ;

/* This is the structure we shall use to register our function */
static struct nf_hook_ops stPreHook;    //Incoming
static struct nf_hook_ops stPostHook;   //Outgoing

static int start = 1;
module_param(start, int, 0774);

//These are added for the TCP-relay part
pid_t ChldThdPid[MAX_THD_IDX];
pid_t RelThd;
unsigned int iThdCnt = 0;
ksocket_t SockfdRelayClntSide;



int Read_File(char *Filename, int StartPos, char *Buffer);

static int my_module_init(void);
static void my_module_exit(void);

void create_config_file (void);
void remove_config_file (void);

int kproc_read_config (char *page, char **start,
			    off_t off, int count, 
			    int *eof, void *data);

int kproc_write_config (struct file *file,
			     const char *buffer,
			     unsigned long count, 
			     void *data);

static unsigned int FromClient(unsigned int hooknum,
	         struct sk_buff **skb,
	         const struct net_device *in,
	         const struct net_device *out,
	         int (*okfn)(struct sk_buff *));

static unsigned int FromRealSrv(unsigned int hooknum,
	         struct sk_buff **skb,
	         const struct net_device *in,
	         const struct net_device *out,
	         int (*okfn)(struct sk_buff *));

int StrToInt (char *str);
static void HookReg (void);
void CheckStart (void *arg);


//Added for Tcp Relay
int tcp_serv (void *arg) ;
void RedirectProc (ksocket_t SockfdFromClnt);


