/* Test processing of REDIRECT message when ND cache is full. */

#include    "nx_api.h"   

extern void    test_control_return(UINT status);

#if defined(FEATURE_NX_IPV6) && !defined(NX_DISABLE_ICMP_INFO) && !defined(NX_DISABLE_ICMPV6_ROUTER_ADVERTISEMENT_PROCESS)
 
#define     DEMO_STACK_SIZE    2048

/* Define the ThreadX and NetX object control blocks...  */

static TX_THREAD               thread_0;  
static NX_PACKET_POOL          pool_0;
static NX_IP                   ip_0;

/* Define the counters used in the demo application...  */

static ULONG                   error_counter;  

/* Define thread prototypes.  */
static VOID    thread_0_entry(ULONG thread_input);
extern VOID    test_control_return(UINT status);       
extern VOID    _nx_ram_network_driver_1500(struct NX_IP_DRIVER_STRUCT *driver_req);

/* RA packet with SLLA and PREFIX options. */
static char ra_pkt[110] = {
0x33, 0x33, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, /* 33...... */
0x00, 0x00, 0xa0, 0xa0, 0x86, 0xdd, 0x60, 0x00, /* ......`. */
0x00, 0x00, 0x00, 0x38, 0x3a, 0xff, 0xfe, 0x80, /* ...8:... */
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, /* ........ */
0x00, 0xff, 0xfe, 0x00, 0xa0, 0xa0, 0xff, 0x02, /* ........ */
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* ........ */
0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x86, 0x00, /* ........ */
0xa0, 0x49, 0x40, 0x00, 0x07, 0x08, 0x00, 0x00, /* .I@..... */
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, /* ........ */
0x00, 0x00, 0x00, 0x00, 0xa0, 0xa0, 0x03, 0x04, /* ........ */
0x40, 0xc0, 0x00, 0x27, 0x8d, 0x00, 0x00, 0x09, /* @..'.... */
0x3a, 0x80, 0x00, 0x00, 0x00, 0x00, 0x3f, 0xfe, /* :.....?. */
0x05, 0x01, 0xff, 0xff, 0x01, 0x00, 0x00, 0x00, /* ........ */
0x00, 0x00, 0x00, 0x00, 0x00, 0x00              /* ...... */
};

/* Redirect packet */
static char redirect_pkt[94] = {
0x00, 0x11, 0x22, 0x33, 0x44, 0x56, 0x00, 0x00, /* .."3DV.. */
0x00, 0x00, 0xa0, 0xa0, 0x86, 0xdd, 0x60, 0x00, /* ......`. */
0x00, 0x00, 0x00, 0x28, 0x3a, 0xff, 0xfe, 0x80, /* ...(:... */
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, /* ........ */
0x00, 0xff, 0xfe, 0x00, 0xa0, 0xa0, 0x3f, 0xfe, /* ......?. */
0x05, 0x01, 0xff, 0xff, 0x01, 0x00, 0x02, 0x11, /* ........ */
0x22, 0xff, 0xfe, 0x33, 0x44, 0x56, 0x89, 0x00, /* "..3DV.. */
0x9a, 0xe3, 0x00, 0x00, 0x00, 0x00, 0x3f, 0xfe, /* ......?. */
0x05, 0x01, 0xff, 0xff, 0x00, 0x00, 0x02, 0x00, /* ........ */
0x00, 0xff, 0xfe, 0x00, 0x01, 0x00, 0x3f, 0xfe, /* ......?. */
0x05, 0x01, 0xff, 0xff, 0x00, 0x00, 0x02, 0x00, /* ........ */
0x00, 0xff, 0xfe, 0x00, 0x01, 0x00              /* ...... */
};

/* Redirect packet */
static char redirect_tlla_pkt[102] = {
0x00, 0x11, 0x22, 0x33, 0x44, 0x56, 0x00, 0x00, /* .."3DV.. */
0x00, 0x00, 0xa0, 0xa0, 0x86, 0xdd, 0x60, 0x00, /* ......`. */
0x00, 0x00, 0x00, 0x30, 0x3a, 0xff, 0xfe, 0x80, /* ...0:... */
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, /* ........ */
0x00, 0xff, 0xfe, 0x00, 0xa0, 0xa0, 0x3f, 0xfe, /* ......?. */
0x05, 0x01, 0xff, 0xff, 0x01, 0x00, 0x02, 0x11, /* ........ */
0x22, 0xff, 0xfe, 0x33, 0x44, 0x56, 0x89, 0x00, /* "..3DV.. */
0x9d, 0x15, 0x00, 0x00, 0x00, 0x00, 0xfe, 0x80, /* ........ */
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, /* ........ */
0x00, 0xff, 0xfe, 0x00, 0xa1, 0xa1, 0x3f, 0xfe, /* ......?. */
0x05, 0x01, 0xff, 0xff, 0x00, 0x00, 0x02, 0x00, /* ........ */
0x00, 0xff, 0xfe, 0x00, 0x01, 0x00, 0x02, 0x01, /* ........ */
0x00, 0x00, 0x00, 0x00, 0xa1, 0xa1              /* ...... */
};

/* Redirect packet with TLLA changed */
static char redirect_tlla_changed_pkt[102] = {
0x00, 0x11, 0x22, 0x33, 0x44, 0x56, 0x00, 0x00, /* .."3DV.. */
0x00, 0x00, 0xa0, 0xa0, 0x86, 0xdd, 0x60, 0x00, /* ......`. */
0x00, 0x00, 0x00, 0x30, 0x3a, 0xff, 0xfe, 0x80, /* ...0:... */
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, /* ........ */
0x00, 0xff, 0xfe, 0x00, 0xa0, 0xa0, 0x3f, 0xfe, /* ......?. */
0x05, 0x01, 0xff, 0xff, 0x01, 0x00, 0x02, 0x11, /* ........ */
0x22, 0xff, 0xfe, 0x33, 0x44, 0x56, 0x89, 0x00, /* "..3DV.. */
0x9e, 0x15, 0x00, 0x00, 0x00, 0x00, 0xfe, 0x80, /* ........ */
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, /* ........ */
0x00, 0xff, 0xfe, 0x00, 0xa0, 0xa0, 0x3f, 0xfe, /* ......?. */
0x05, 0x01, 0xff, 0xff, 0x00, 0x00, 0x02, 0x00, /* ........ */
0x00, 0xff, 0xfe, 0x00, 0x01, 0x00, 0x02, 0x01, /* ........ */
0x00, 0x01, 0x00, 0x00, 0xa1, 0xa1              /* ...... */
};



/* Define what the initial system looks like.  */

#ifdef CTEST
VOID test_application_define(void *first_unused_memory)
#else
void           netx_icmpv6_redirect_nd_full_test_application_define(void *first_unused_memory)
#endif
{
CHAR       *pointer;
UINT       status;

    /* Setup the working pointer.  */
    pointer = (CHAR *) first_unused_memory;

    /* Initialize the value.  */
    error_counter = 0;

    /* Create the main thread.  */
    tx_thread_create(&thread_0, "thread 0", thread_0_entry, 0,  
        pointer, DEMO_STACK_SIZE, 
        4, 4, TX_NO_TIME_SLICE, TX_AUTO_START);

    pointer = pointer + DEMO_STACK_SIZE;

    /* Initialize the NetX system.  */
    nx_system_initialize();

    /* Create a packet pool.  */
    status = nx_packet_pool_create(&pool_0, "NetX Main Packet Pool", 1536, pointer, 1536*16);
    pointer = pointer + 1536*16;

    if(status)
        error_counter++;

    /* Create an IP instance.  */
    status = nx_ip_create(&ip_0, "NetX IP Instance 0", IP_ADDRESS(1,2,3,4), 0xFFFFFF00UL, &pool_0, _nx_ram_network_driver_1500,
        pointer, 2048, 1);
    pointer = pointer + 2048;
  
    /* Enable IPv6 */
    status += nxd_ipv6_enable(&ip_0); 

    /* Check IPv6 enable status.  */
    if(status)
        error_counter++;        

    /* Enable IPv6 ICMP  */
    status += nxd_icmp_enable(&ip_0); 

    /* Check IPv6 ICMP enable status.  */
    if(status)
        error_counter++;
}

/* Define the test threads.  */

static void    thread_0_entry(ULONG thread_input)
{
UINT                    status;   
NX_PACKET              *packet_ptr;
ULONG                   address[4] = {0xfe800000, 0, 0, 1};
CHAR                    mac[6] = {0, 0, 0, 0, 0, 1};

    /* Print out test information banner.  */
    printf("NetX Test:   ICMPv6 Redirect ND Full Test.............................."); 

    /* Check for earlier error.  */
    if(error_counter)
    {
        printf("ERROR!\n");
        test_control_return(1);
    }                     

    /* Set the linklocal address.  */
    status = nxd_ipv6_address_set(&ip_0, 0, NX_NULL, 10, NX_NULL); 

    /* Check the status.  */
    if(status)
        error_counter++;  

    /* Sleep 5 seconds for linklocal address DAD.  */
    tx_thread_sleep(5 * NX_IP_PERIODIC_RATE);

    /* Inject RA packet. */
    status = nx_packet_allocate(&pool_0, &packet_ptr, NX_PHYSICAL_HEADER, NX_WAIT_FOREVER);

    /* Check status */
    if(status)
        error_counter++;

    /* Fill in the packet with data. Skip the MAC header.  */
    memcpy(packet_ptr -> nx_packet_prepend_ptr, &ra_pkt[14], sizeof(ra_pkt) - 14);
    packet_ptr -> nx_packet_length = sizeof(ra_pkt) - 14;
    packet_ptr -> nx_packet_append_ptr = packet_ptr -> nx_packet_prepend_ptr + packet_ptr -> nx_packet_length;

    /* Directly receive the RA packet.  */
    _nx_ip_packet_deferred_receive(&ip_0, packet_ptr);     

    /* Sleep 5 seconds for global address DAD.  */
    tx_thread_sleep(5 * NX_IP_PERIODIC_RATE);


    /* Inject REDIRECT packet with TLLA changed. */
    status = nx_packet_allocate(&pool_0, &packet_ptr, NX_PHYSICAL_HEADER, NX_WAIT_FOREVER);

    /* Check status */
    if(status)
        error_counter++;

    /* Fill in the packet with data. Skip the MAC header.  */
    memcpy(packet_ptr -> nx_packet_prepend_ptr, &redirect_tlla_changed_pkt[14], sizeof(redirect_tlla_changed_pkt) - 14);
    packet_ptr -> nx_packet_length = sizeof(redirect_tlla_changed_pkt) - 14;
    packet_ptr -> nx_packet_append_ptr = packet_ptr -> nx_packet_prepend_ptr + packet_ptr -> nx_packet_length;

    /* Directly receive the REDIRECT packet.  */
    _nx_ip_packet_deferred_receive(&ip_0, packet_ptr);     


    /* Occupy all ND caches. */
    while (nxd_nd_cache_entry_set(&ip_0, address, 0, mac) == NX_SUCCESS)
    {
        address[3]++;
        mac[5]++;
    }

    /* Inject REDIRECT packet. */
    status = nx_packet_allocate(&pool_0, &packet_ptr, NX_PHYSICAL_HEADER, NX_WAIT_FOREVER);

    /* Check status */
    if(status)
        error_counter++;

    /* Fill in the packet with data. Skip the MAC header.  */
    memcpy(packet_ptr -> nx_packet_prepend_ptr, &redirect_pkt[14], sizeof(redirect_pkt) - 14);
    packet_ptr -> nx_packet_length = sizeof(redirect_pkt) - 14;
    packet_ptr -> nx_packet_append_ptr = packet_ptr -> nx_packet_prepend_ptr + packet_ptr -> nx_packet_length;

    /* Directly receive the REDIRECT packet.  */
    _nx_ip_packet_deferred_receive(&ip_0, packet_ptr);     


    /* Inject REDIRECT packet with TLLA option. */
    status = nx_packet_allocate(&pool_0, &packet_ptr, NX_PHYSICAL_HEADER, NX_WAIT_FOREVER);

    /* Check status */
    if(status)
        error_counter++;

    /* Fill in the packet with data. Skip the MAC header.  */
    memcpy(packet_ptr -> nx_packet_prepend_ptr, &redirect_tlla_pkt[14], sizeof(redirect_tlla_pkt) - 14);
    packet_ptr -> nx_packet_length = sizeof(redirect_tlla_pkt) - 14;
    packet_ptr -> nx_packet_append_ptr = packet_ptr -> nx_packet_prepend_ptr + packet_ptr -> nx_packet_length;

    /* Directly receive the REDIRECT packet.  */
    _nx_ip_packet_deferred_receive(&ip_0, packet_ptr);     


    /* Verify the redirect message is dropped. */
    if (ip_0.nx_ip_icmp_invalid_packets != 2)
        error_counter++;

    /* Check the error.  */
    if(error_counter)
    {
        printf("ERROR!\n");
        test_control_return(1);
    }  
    else
    {
        printf("SUCCESS!\n");      
        test_control_return(0);
    }
}                     
#else

#ifdef CTEST
VOID test_application_define(void *first_unused_memory)
#else
void           netx_icmpv6_redirect_nd_full_test_application_define(void *first_unused_memory)
#endif
{

    /* Print out test information banner.  */
    printf("NetX Test:   ICMPv6 Redirect ND Full Test..............................N/A\n"); 
    test_control_return(3);

}
#endif /* FEATURE_NX_IPV6 */
