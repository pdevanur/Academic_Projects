#ifndef _packetfilter_h_
#define _packetfilter_h_


#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/ip.h>
#include <linux/in.h>
#include <linux/netdevice.h>
#include <linux/udp.h>
#include <linux/kthread.h>
//#include <asm/semaphore.h>

//#include <net/ksocket.h>
#include <linux/string.h>
#include <linux/socket.h>
#include <linux/net.h>
#include <net/sock.h>
#include <asm/processor.h>
#include <asm/uaccess.h>

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("UDP Client");
MODULE_AUTHOR("Jeevan");

#define KSOCKET_NAME	"ksocket"
#define KSOCKET_VERSION	"0.01"
#define KSOCKET_DESCPT	"BSD-style socket APIs for kernel 2.6 developers"
#define KSOCKET_AUTHOR	"song.xian-guang@hotmail.com"
#define KSOCKET_DATE	"2007-08-21"

/* debug macro*/
#ifdef  _ksocket_debug_
#	define ksocket_debug(fmt, args...)	printk("ksocket : %s, %s, %d, "fmt, __FILE__, __FUNCTION__, __LINE__, ##args)
#else
#	define ksocket_debug(fmt, args...)
#endif


#define DEST_PORT (6001)
#define LISTEN_PORT (10002)
#define SERV_IP "192.168.2.2"
//#define MY_IP	"192.168.140.128"
#define MAX_PCKT 200
#define BUFLEN 512
#define RATE_1 (1428*8*4)
#define RATE_2 (1428*8*1000)
#define CALC_SLOPE 30


struct task_struct *pstUdpClnt = NULL;
struct task_struct *pstPollThd = NULL;
struct task_struct *pstUdpsrv = NULL;

struct socket;
struct sockaddr;
typedef struct socket * ksocket_t;

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


//sys variable
static int start = 0;
module_param(start, int, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);

int iUdpRun;
static int on=1;
static int iPoll = 1;

unsigned int inet_addr(char *str);
void UdpClient (void *arg);
//int UdpClntInit(void);
//void UdpClntCleanUp(void);


ksocket_t ksocket(int domain, int type, int protocol);
int ksetsockopt(ksocket_t socket, int level, int optname, void *optval, int optlen);
ssize_t ksendto(ksocket_t socket, void *message, size_t length, int flags, const struct sockaddr *dest_addr, int dest_len);
int kclose(ksocket_t socket);
void PollThread (void *arg);
void UdpClient (void *arg);
int CalcThresh (stamp *src, stamp *dest);
void bsort (double *numbers, int cnt);
double CalculateBW (double r1, double r2, int iPkt1NoQ, int iPkt2NoQ, double *C);
int tcp_srv(void *arg);

#endif
