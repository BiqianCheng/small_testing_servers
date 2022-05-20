// Server side implementation of UDP client-server model
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <klee/klee.h>

typedef unsigned char u_char;
typedef unsigned short u_short;
typedef unsigned long u_long;
typedef unsigned int u_int;

#define MAXLINE 1024
#define PORT 55555

#define MSG_MONLIST 0
#define MSG_EXIT 1

#define MON_HASH_SIZE 128
#define MAXMONMEM 600 /* we allocate up to 600 structures */
#define MONMEMINC 40
#define RESP_DATA_SIZE 500

#define MON_HASH(addr) sock_hash(addr)
#define CAST_V4(src) ((struct sockaddr_in *)&(src))
#define CAST_V6(src) ((struct sockaddr_in6 *)&(src))
#define GET_INADDR(src) (CAST_V4(src)->sin_addr.s_addr)
#define GET_INADDR6(src) (CAST_V6(src)->sin6_addr)
#define MBZ_ITEMSIZE(itemsize) (htons((u_short)(itemsize)))
#define P(x) x

int sockfd;
u_char buffer[MAXLINE];
struct sockaddr_storage servaddr;
struct sockaddr_in cliaddr;
unsigned int cliaddLen = sizeof(cliaddr), n, o;

struct resp_pkt
{
    u_short mbz_itemsize;      /* item size */
    char data[RESP_DATA_SIZE]; /* data area */
};

static struct resp_pkt rpkt;
static int reqver;
static int seqno;
static int nitems;
static int itemsize;
static int databytes;
static char exbuf[RESP_DATA_SIZE];
static int usingexbuf;
static struct sockaddr_storage *toaddr;

struct recvbuf
{
    struct sockaddr_in recv_srcadr; // store reciever's address
    int recv_length; /* the size of the data received */
    u_char recv_buffer[MAXLINE]; // the data buffer
};

/*
 * Structure used optionally for monitoring when this is turned on.
 */
struct mon_data
{
    struct mon_data *hash_next; /* next structure in hash list */
    struct mon_data *mru_next;  /* next structure in MRU list */
    struct mon_data *mru_prev;  /* previous structure in MRU list */
    struct sockaddr_in rmtadr;  /* address of remote host */
};

/*
 * Pointers to the hash table, the MRU list and the count table.  Memory
 * for the hash and count tables is only allocated if monitoring is
 * turned on.
 */
static struct mon_data *mon_hash[MON_HASH_SIZE]; /* list ptrs */
struct mon_data mon_mru_list;

/*
 * List of free structures structures, and counters of free and total
 * structures.  The free structures are linked with the hash_next field.
 */
static struct mon_data *mon_free; /* free list or null if none */
static int mon_total_mem;         /* total structures allocated */
static int mon_mem_increments;    /* times called malloc() */

/*
 * Initialization state.  We may be monitoring, we may not.  If
 * we aren't, we may not even have allocated any memory yet.
 */
u_long mon_age = 3000; /* preemption limit */
static int mon_have_memory;
static void mon_getmoremem P((void));
static void remove_from_hash P((struct mon_data *));

void *
emalloc(
    u_int size)
{
    char *mem;

    if ((mem = (char *)malloc(size)) == 0)
    {
        perror("Exiting: No more memory!");
        exit(1);
    }
    return mem;
}

/*
 * sock_hash - hash an sockaddr_storage structure
 */
int sock_hash(
    struct sockaddr_in *addr)
{
    int hashVal;
    int i;
    int len;
    char *ch;

    hashVal = 0;
    len = 0;
    /*
     * We can't just hash the whole thing because there are hidden
     * fields in sockaddr_in6 that might be filled in by recvfrom(),
     * so just use the family, port and address.
     */
    ch = (char *)&addr->sin_family;
    hashVal = 37 * hashVal + (int)*ch;
    if (sizeof(addr->sin_family) > 1)
    {
        ch++;
        hashVal = 37 * hashVal + (int)*ch;
    }

    ch = (char *)&((struct sockaddr_in *)addr)->sin_addr;
    len = sizeof(struct in_addr);

    for (i = 0; i < len; i++)
        hashVal = 37 * hashVal + (int)*(ch + i);

    hashVal = hashVal % 128; /* % MON_HASH_SIZE hardcoded */

    if (hashVal < 0)
        hashVal += 128;

    return hashVal;
}

static char *prepare_pkt(u_int structsize)
{
    rpkt.mbz_itemsize = MBZ_ITEMSIZE(structsize);
    /*
     * Compute the static data needed to carry on.
     */
    nitems = 0;
    itemsize = structsize;
    databytes = 0;
    usingexbuf = 0;

    /*
     * return the beginning of the packet buffer.
     */
    return &rpkt.data[0];
}

static char *more_pkt()
{
    if (usingexbuf)
    {
        sendto(sockfd, (char *)&rpkt, 8 + databytes, MSG_WAITALL,
               (struct sockaddr *)&cliaddr, cliaddLen);
        memmove(&rpkt.data[0], exbuf, (unsigned)itemsize);
        seqno++;
        databytes = 0;
        nitems = 0;
        usingexbuf = 0;
    }
    databytes += itemsize;
    nitems++;
    if (databytes + itemsize <= RESP_DATA_SIZE)
    {
        return &rpkt.data[databytes];
    }
    else
    {
        usingexbuf = 1;
        return exbuf;
    }
}

/*
 * flush_pkt - we're done, return remaining information.
 */
static void
flush_pkt(void)
{
    sendto(sockfd, (char *)&rpkt, 8 + databytes, MSG_WAITALL,
           (struct sockaddr *)&cliaddr, cliaddLen);
}

/*
 * init_mon - initialize monitoring global data
 */
void init_mon(void)
{
    /*
     * Don't do much of anything here.  We don't allocate memory
     * until someone explicitly starts us.
     */

    mon_have_memory = 0;

    mon_total_mem = 0;
    mon_mem_increments = 0;
    mon_free = NULL;
    memset(&mon_hash[0], 0, sizeof mon_hash);
    memset(&mon_mru_list, 0, sizeof mon_mru_list);
}

/*
 * mon_start - start up the monitoring software
 */
void mon_start()
{
    if (!mon_have_memory)
    {
        mon_total_mem = 0;
        mon_mem_increments = 0;
        mon_free = NULL;
        mon_getmoremem();
        mon_have_memory = 1;
    }

    mon_mru_list.mru_next = &mon_mru_list;
    mon_mru_list.mru_prev = &mon_mru_list;
}

/*
 * ntp_monitor - record stats about this packet
 *
 * Returns 1 if the packet is at the head of the list, 0 otherwise.
 */
int ntp_monitor(
    struct recvbuf *rbufp)
{
    register struct pkt *pkt;
    register struct mon_data *md;
    struct sockaddr_in addr;
    register int hash;
    register int mode;

    memset(&addr, 0, sizeof(addr));
    memcpy(&addr, &(rbufp->recv_srcadr), sizeof(addr));
    hash = MON_HASH(&addr);
    md = mon_hash[hash];

    /*
     * If we got here, this is the first we've heard of this
     * guy.  Get him some memory, either from the free list
     * or from the tail of the MRU list.
     */
    if (mon_free == NULL && mon_total_mem >= MAXMONMEM)
    {

        /*
         * Preempt from the MRU list if old enough.
         */
        md = mon_mru_list.mru_prev;

        md->mru_prev->mru_next = &mon_mru_list;
        mon_mru_list.mru_prev = md->mru_prev;
        remove_from_hash(md);
    }
    else
    {
        if (mon_free == NULL)
            mon_getmoremem();
        md = mon_free;
        mon_free = md->hash_next;
    }

    /*
     * Got one, initialize it
     */
    memset(&md->rmtadr, 0, sizeof(md->rmtadr));
    memcpy(&md->rmtadr, &addr, sizeof(addr));

    /*
     * Drop him into front of the hash table. Also put him on top of
     * the MRU list.
     */
    md->hash_next = mon_hash[hash];
    mon_hash[hash] = md;
    md->mru_next = mon_mru_list.mru_next;
    md->mru_prev = &mon_mru_list;
    mon_mru_list.mru_next->mru_prev = md;
    mon_mru_list.mru_next = md;
    return 1;
}

/*
 * mon_getmoremem - get more memory and put it on the free list
 */
static void
mon_getmoremem(void)
{
    register struct mon_data *md;
    register int i;
    struct mon_data *freedata; /* 'old' free list (null) */

    md = (struct mon_data *)emalloc(MONMEMINC *
                                    sizeof(struct mon_data));
    freedata = mon_free;
    mon_free = md;
    for (i = 0; i < (MONMEMINC - 1); i++)
    {
        md->hash_next = (md + 1);
        md++;
    }

    /*
     * md now points at the last.  Link in the rest of the chain.
     */
    md->hash_next = freedata;
    mon_total_mem += MONMEMINC;
    mon_mem_increments++;
}

static void
remove_from_hash(
    struct mon_data *md)
{
    register int hash;
    register struct mon_data *md_prev;

    hash = MON_HASH(&md->rmtadr);
    if (mon_hash[hash] == md)
    {
        mon_hash[hash] = md->hash_next;
    }
    else
    {
        md_prev = mon_hash[hash];
        while (md_prev->hash_next != md)
        {
            md_prev = md_prev->hash_next;
            if (md_prev == NULL)
            {
                /* logic error */
                return;
            }
        }
        md_prev->hash_next = md->hash_next;
    }
}

/*
 * Structure used for returning monitor data
 */
struct info_monitor
{
    u_int addr; /* host address */
};

//send connection states to the client
static void mon_getlist()
{
    struct mon_data *md;
    struct info_monitor *im;

    im = (struct info_monitor *)prepare_pkt(sizeof(struct info_monitor));
    for (md = mon_mru_list.mru_next; md != &mon_mru_list && im != 0;
         md = md->mru_next)
    {
        im->addr = GET_INADDR(md->rmtadr);
        im = (struct info_monitor *)more_pkt();
    }
    flush_pkt();
}
// Driver code
int main()
{
    // Creating socket file descriptor
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&servaddr, 0, sizeof(servaddr));
    memset(&cliaddr, 0, sizeof(cliaddr));

    struct sockaddr_in *saddr;

    saddr = CAST_V4(servaddr);

    // Filling server information
    saddr->sin_family = AF_INET; // IPv4
    saddr->sin_addr.s_addr = INADDR_ANY;
    saddr->sin_port = htons(PORT);

    // Bind the socket with the server address
    if (bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) <
        0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    init_mon();
    mon_start();

    for (int i = 0; i < 10; ++i)
    {
        n = recvfrom(sockfd, (char *)buffer, MAXLINE, MSG_WAITALL,
                     (struct sockaddr *)&cliaddr, &cliaddLen);
        struct recvbuf tempbuf;
        memset(&tempbuf.recv_buffer, 0, sizeof(tempbuf.recv_buffer));
        memset(&tempbuf.recv_length, 0, sizeof(tempbuf.recv_length));
        memcpy(&tempbuf.recv_buffer, buffer, sizeof(tempbuf.recv_buffer));

        tempbuf.recv_srcadr = cliaddr;

        tempbuf.recv_length = n;

        if (buffer[0] == MSG_MONLIST)
        {
            mon_getlist();
        }
        else
        {
            printf("Getting Connection from someone\n");
            ntp_monitor(&tempbuf);
        }
    }

    return 0;
}