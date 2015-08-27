#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#define CALC_PKT 30
#define PORT 6001
#define MAX_PCKT 400
#define RATE_1 (1428*8*4)
#define RATE_2 (1428*8*1000*4)
#define CPUSPEEDSEND  ( 2992.453 * 1000000 )
//#define CPUSPEED  ( 800.000 * 1000000 )
#define CPUSPEEDRECV  (2193.317 * 1000000 )

void diep(char *s)
{
    perror(s);
    exit(1);
}

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

typedef struct stTime
{
//	unsigned long long int tsrc;
//	unsigned long long int tdest;
	double tsrc;
	double tdest;

}MYTIME;

MYTIME stTData[MAX_PCKT]; 


//Takes probe rates r1, r2 and the packets that have not experienced queuing
double CalculateBW (double r1, double r2, int iPkt1NoQ, int iPkt2NoQ, double *C)
{
    double c1, c0, A, pr1, pr2;
    
    //calculate pr1 and pr2
    pr1 = ((double)(iPkt1NoQ)/(double)(MAX_PCKT/2));
    pr2 = ((double)(iPkt2NoQ)/(double)(MAX_PCKT/2));

    c1 = (pr2 - pr1)/(r2 - r1);
    c0 = pr1 - r1*c1;
    
    //Tight link speed C
    *C = 1/c1;

    //available bandwidth A    
    A = (1 - c0)/c1;
    return A;
}

void bsort (double *numbers, int cnt)
{
  int i, j;
  double temp;

  for (i = cnt; i >= 0; i--)
    for (j = 1; j <= i; j++)
      if (numbers[j-1] > numbers[j])
      {
        temp = numbers[j-1];
        numbers[j-1] = numbers[j];
        numbers[j] = temp;
      }

}


int CalcThresh (int rate, int iMaxR)
{
    int cnt = 0, i,j, iTemp, jTemp;
    int i1cnt = 0, i2cnt = 0;
    int minindx = 0, maxindx = 0;
    int numcnt = 0;   
    
    double projval[100];
    double tmp, mintargety, maxtargety, bandvalue;
    double x,y;
    double aver = 0.0;


    if (rate == 1)
    {
        iTemp = i = 0;
        jTemp = j = iMaxR - CALC_PKT;
    } 
    else
    {
        iTemp = i = MAX_PCKT/2;
        jTemp = j = iMaxR - CALC_PKT;
    }

    i1cnt = i+CALC_PKT;
    i2cnt = j+CALC_PKT;
     
     
    printf("%d %d %d %d\n",i, i1cnt, j, i2cnt);
    //consider first 30 packets and last 30 packets
    //y2-y1/x2-x1
    for (; i < (i1cnt); i++)
    	for (; j < (i2cnt); j++)
            aver = aver + (stTData[j].tdest-stTData[i].tdest)/(stTData[j].tsrc-stTData[i].tsrc);
    
    
    aver /= (i1cnt*i2cnt);   
    /*#-----------------------------------------------------------------
    #STEP 2 : FIND THE  PROJETCTION OF THE Y ON Y AXIS AND GET THE BAND
    #y2 = y1 - m*x1
    #-----------------------------------------------------------------*/
    cnt = 0;
      
    mintargety = stTData[iTemp].tdest - aver*stTData[iTemp].tsrc;
    maxtargety = mintargety;
    
    projval[cnt++] = mintargety;
    

    //calcualte for the for the first CALC_PKT values followed by the last CALC_PKT packets
    for (i = iTemp; i < (i1cnt); i++)
    {
        tmp = stTData[i].tdest - aver*stTData[i].tsrc;
        projval[cnt++] = tmp;       

    	if (tmp < mintargety)
    	{
    		mintargety = tmp;
    		minindx = cnt-1;
    	}
    	
    	else if (tmp > maxtargety)
    	{
    		maxtargety = tmp;
    		maxindx = cnt-1;
        }
    }

    for (i = jTemp; i < (i2cnt); i++)
    {
        tmp = stTData[i].tdest - aver*stTData[i].tsrc;
        projval[cnt++] = tmp;       

    	if (tmp < mintargety)
    	{
    		mintargety = tmp;
    		minindx = cnt-1;
    	}
    	
    	else if (tmp > maxtargety)
    	{
    		maxtargety = tmp;
    		maxindx = cnt-1;
        }
    }   
    
    bsort (projval,  cnt-1);
    
    //#Take the 95% values below the line
    bandvalue = projval[(95*cnt)/100];
    
    
    /*#----------------------------------------------------------------------------
    #STEP 4 : Find the number of points below and above the point for a given slope
    #-----------------------------------------------------------------------------*/
    cnt = iTemp + CALC_PKT;
    
    while (cnt < i2cnt)
    {
        tmp = stTData[cnt].tdest - (aver * (stTData[cnt].tsrc));
        if (tmp > bandvalue)
            numcnt = numcnt+1;
//	printf("cnt:%d\r\n",cnt);
        cnt++;
    }
    
    return numcnt;
}


int main(void)
{
    struct sockaddr_in si_me, si_other;
    int s, i, slen=sizeof(si_other);
    stamp buf;
    int n1, n2, iMaxR1, iMaxR2;
    double A,C;
	double temprate;
    
    if ((s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))==-1)
        diep("socket");
        
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, NULL,0);

    memset((char *) &si_me, 0, sizeof(si_me));
    si_me.sin_family = AF_INET;
    si_me.sin_port = htons(PORT);
    si_me.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(s, &si_me, sizeof(si_me))==-1)
    diep("bind");
    
    
    //A = CalculateBW ((double)RATE_1, (double)RATE_2, 3, 10, &C);
      //  printf ("Available Bandwidth = %f, Tight Link Speed = %f\n", A, C);
    
//    while (1)
    {
        iMaxR1 = iMaxR2 = 0;
        
        for (i=0; i<MAX_PCKT+10; i++) 
        {
            if (recvfrom(s, &buf, sizeof(stamp), 0, &si_other, &slen)==-1)
                diep("recvfrom()");
            //printf("Received packet from %d:%d\nID = %d, Tmpstmps: %Ld %Ld\n\n", 
            //(buf.source), (buf.destination), buf.id, buf.tmpstmpsrc, buf.tmpstmp);

            printf ("ID = %d\n", buf.id);
            //set the limit for first set of packets sent at rate r1
            if (buf.id >= (MAX_PCKT/2) && (!iMaxR1))
            {
                iMaxR1 = i;
                i = MAX_PCKT/2;
            }

            //chk for the first marker packet and set MaxR2 and skip other marker packets
            if (buf.marker == 1)
            {
                if (!iMaxR2)
                    iMaxR2 = i;
                memset((char *) &buf, 0, sizeof(stamp));
                //continue;
                break;
            }

            printf("Received packet Tmpstmps: %Ld %Ld\n", buf.tmpstmpsrc, buf.tmpstmp);
            //store the receiver timestamp and absolute value of the time difference between receiver and sender timestamps
            stTData[i].tsrc = (double)buf.tmpstmp/(CPUSPEEDRECV);
            
            printf("%lf\t",stTData[i].tsrc);

            stTData[i].tdest =         (  (  (double)buf.tmpstmp/(CPUSPEEDRECV)  ) -   ( (double)buf.tmpstmpsrc/(CPUSPEEDSEND)  )  )   ;  ;//
            if(stTData[i].tdest < 0)
             stTData[i].tdest *=-1;
             
            printf("%lf\n",stTData[i].tdest);

            memset((char *) &buf, 0, sizeof(stamp));
        }
            
            if (0 == iMaxR2)
                iMaxR2 = MAX_PCKT;
             printf("Rate :%lf \n" , temprate=   (double)( ( iMaxR2 - MAX_PCKT/2)*1428 *8   )  /  (     stTData[iMaxR2-1].tsrc  - stTData[MAX_PCKT/2].tsrc )       )  ;
            printf ("Maxr1 = %d, r2 = %d\n", iMaxR1, iMaxR2);        
        //caclcualte the no. of packets experienced queueing
        n1 = CalcThresh (1, iMaxR1);
        n2 = CalcThresh (2, iMaxR2);
        
           printf ("Packets queued(2), r1 = %d, r2 = %d", n1, n2);
        //consider the lost packets as the queued packets
        n1 = n1 + (MAX_PCKT/2) - iMaxR1;
        n2 = n2 + (MAX_PCKT) - iMaxR2;
        
        printf ("Packets queued, r1 = %d, r2 = %d", n1, n2);
        A = CalculateBW ((double)RATE_1, (double)temprate, n1, n2, &C);
        printf ("Available Bandwidth = %f, Tight Link Speed = %f\n", A, C);
        
        sleep (20);
    }
    close(s);
    return 0;
}

