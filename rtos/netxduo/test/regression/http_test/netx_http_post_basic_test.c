/* This case tests basic post method. */
#include    "tx_api.h"
#include    "nx_api.h"
#include    "fx_api.h"
#include    "nxd_http_client.h"
#include    "nxd_http_server.h"

extern void test_control_return(UINT);

#if !defined(NX_DISABLE_IPV4)

#define     DEMO_STACK_SIZE         4096

/* This is a HTTP get packet captured by wireshark. POST AAAAAAAAAA*/
static char pkt[] = {
    0x50, 0x4f, 0x53, 0x54, 0x20, 0x2f, 0x69, 0x6e, /* POST /in */
    0x64, 0x65, 0x78, 0x2e, 0x68, 0x74, 0x6d, 0x20, /* dex.htm  */
    0x48, 0x54, 0x54, 0x50, 0x2f, 0x31, 0x2e, 0x31, /* HTTP/1.1 */
    0x0d, 0x0a, 0x55, 0x73, 0x65, 0x72, 0x2d, 0x41, /* ..User-A */
    0x67, 0x65, 0x6e, 0x74, 0x3a, 0x20, 0x63, 0x75, /* gent: cu */
    0x72, 0x6c, 0x2f, 0x37, 0x2e, 0x33, 0x32, 0x2e, /* rl/7.32. */
    0x30, 0x0d, 0x0a, 0x48, 0x6f, 0x73, 0x74, 0x3a, /* 0..Host: */
    0x20, 0x31, 0x39, 0x32, 0x2e, 0x31, 0x36, 0x38, /*  192.168 */
    0x2e, 0x30, 0x2e, 0x31, 0x32, 0x33, 0x0d, 0x0a, /* .0.123.. */
    0x41, 0x63, 0x63, 0x65, 0x70, 0x74, 0x3a, 0x20, /* Accept:  */
    0x2a, 0x2f, 0x2a, 0x0d, 0x0a, 0x43, 0x6f, 0x6e, /*          */
    0x74, 0x65, 0x6e, 0x74, 0x2d, 0x4c, 0x65, 0x6e, /* tent-Len */
    0x67, 0x74, 0x68, 0x3a, 0x20, 0x31, 0x30, 0x0d, /* gth: 10. */
    0x0a, 0x43, 0x6f, 0x6e, 0x74, 0x65, 0x6e, 0x74, /* .Content */
    0x2d, 0x54, 0x79, 0x70, 0x65, 0x3a, 0x20, 0x61, /* -Type: a */
    0x70, 0x70, 0x6c, 0x69, 0x63, 0x61, 0x74, 0x69, /* pplicati */
    0x6f, 0x6e, 0x2f, 0x78, 0x2d, 0x77, 0x77, 0x77, /* on/x-www */
    0x2d, 0x66, 0x6f, 0x72, 0x6d, 0x2d, 0x75, 0x72, /* -form-ur */
    0x6c, 0x65, 0x6e, 0x63, 0x6f, 0x64, 0x65, 0x64, /* lencoded */
    0x0d, 0x0a, 0x0d, 0x0a, 0x41, 0x41, 0x41, 0x41, /* ....AAAA */
    0x41, 0x41, 0x41, 0x41, 0x41, 0x41              /* AAAAAA */
};


/* Set up FileX and file memory resources. */
static CHAR             *ram_disk_memory;
static FX_MEDIA         ram_disk;
static unsigned char    media_memory[512];

/* Define device drivers.  */
extern void _fx_ram_driver(FX_MEDIA *media_ptr);
extern void _nx_ram_network_driver_1024(NX_IP_DRIVER *driver_req_ptr);


/* Set up the HTTP client global variables. */

#define CLIENT_PACKET_SIZE  (NX_HTTP_SERVER_MIN_PACKET_SIZE * 2)

static TX_THREAD       client_thread;
static NX_PACKET_POOL  client_pool;
static NX_HTTP_CLIENT  my_client;
static NX_IP           client_ip;
static UINT            error_counter;

static NX_TCP_SOCKET   client_socket;

/* Set up the HTTP server global variables */

#define SERVER_PACKET_SIZE  (NX_HTTP_SERVER_MIN_PACKET_SIZE * 2)

static NX_HTTP_SERVER  my_server;
static NX_PACKET_POOL  server_pool;
static TX_THREAD       server_thread;
static NX_IP           server_ip;
#ifdef __PRODUCT_NETXDUO__
static NXD_ADDRESS     server_ip_address;
#else
static ULONG           server_ip_address;
#endif

 
static void thread_client_entry(ULONG thread_input);
static void thread_server_entry(ULONG thread_input);

#define HTTP_SERVER_ADDRESS  IP_ADDRESS(192,168,0,105)
#define HTTP_CLIENT_ADDRESS  IP_ADDRESS(192,168,0,123)

static UINT my_get_notify(NX_HTTP_SERVER *server_ptr, UINT request_type, CHAR *resource, NX_PACKET *packet_ptr);

#ifdef CTEST
VOID test_application_define(void *first_unused_memory)
#else
void    netx_http_post_basic_test_application_define(void *first_unused_memory)
#endif
{

CHAR    *pointer;
UINT    status;

    error_counter = 0;

    /* Setup the working pointer.  */
    pointer =  (CHAR *) first_unused_memory;

    /* Create a helper thread for the server. */
    tx_thread_create(&server_thread, "HTTP Server thread", thread_server_entry, 0,  
                     pointer, DEMO_STACK_SIZE, 
                     4, 4, TX_NO_TIME_SLICE, TX_AUTO_START);

    pointer =  pointer + DEMO_STACK_SIZE;

    /* Initialize the NetX system.  */
    nx_system_initialize();

    /* Create the server packet pool.  */
    status =  nx_packet_pool_create(&server_pool, "HTTP Server Packet Pool", SERVER_PACKET_SIZE, 
                                    pointer, SERVER_PACKET_SIZE*8);
    pointer = pointer + SERVER_PACKET_SIZE * 8;
    if (status)
        error_counter++;

    /* Create an IP instance.  */
    status = nx_ip_create(&server_ip, "HTTP Server IP", HTTP_SERVER_ADDRESS, 
                          0xFFFFFF00UL, &server_pool, _nx_ram_network_driver_1024,
                          pointer, 4096, 1);
    pointer =  pointer + 4096;
    if (status)
        error_counter++;

    /* Enable ARP and supply ARP cache memory for the server IP instance.  */
    status = nx_arp_enable(&server_ip, (void *) pointer, 1024);
    pointer = pointer + 1024;
    if (status)
        error_counter++;


     /* Enable TCP traffic.  */
    status = nx_tcp_enable(&server_ip);
    if (status)
        error_counter++;

    /* Set up the server's IPv4 address here. */
#ifdef __PRODUCT_NETXDUO__ 
    server_ip_address.nxd_ip_address.v4 = HTTP_SERVER_ADDRESS;
    server_ip_address.nxd_ip_version = NX_IP_VERSION_V4;
#else
    server_ip_address = HTTP_SERVER_ADDRESS;
#endif

    /* Create the HTTP Server.  */
    status = nx_http_server_create(&my_server, "My HTTP Server", &server_ip, &ram_disk, 
                          pointer, 2048, &server_pool, NX_NULL, my_get_notify);
    pointer =  pointer + 2048;
    if (status)
        error_counter++;

    /* Save the memory pointer for the RAM disk.  */
    ram_disk_memory =  pointer;

    /* Create the HTTP Client thread. */
    status = tx_thread_create(&client_thread, "HTTP Client", thread_client_entry, 0,  
                     pointer, DEMO_STACK_SIZE, 
                     6, 6, TX_NO_TIME_SLICE, TX_AUTO_START);
    pointer =  pointer + DEMO_STACK_SIZE;
    if (status)
        error_counter++;

    /* Create the Client packet pool.  */
    status =  nx_packet_pool_create(&client_pool, "HTTP Client Packet Pool", CLIENT_PACKET_SIZE, 
                                    pointer, CLIENT_PACKET_SIZE*8);
    pointer = pointer + CLIENT_PACKET_SIZE * 8;
    if (status)
        error_counter++;

    /* Create an IP instance.  */
    status = nx_ip_create(&client_ip, "HTTP Client IP", HTTP_CLIENT_ADDRESS, 
                          0xFFFFFF00UL, &client_pool, _nx_ram_network_driver_1024,
                          pointer, 2048, 1);
    pointer =  pointer + 2048;
    if (status)
        error_counter++;

    status  = nx_arp_enable(&client_ip, (void *) pointer, 1024);
    pointer =  pointer + 2048;
    if (status)
        error_counter++;

     /* Enable TCP traffic.  */
    status = nx_tcp_enable(&client_ip);
    if (status)
        error_counter++;

}


void thread_client_entry(ULONG thread_input)
{

UINT            status;
NX_PACKET       *send_packet;
NX_PACKET       *recv_packet;
NX_PACKET       *my_packet;
CHAR            *buffer_ptr;

    /* Format the RAM disk - the memory for the RAM disk was setup in 
      tx_application_define above.  This must be set up before the client(s) start
      sending requests. */
    status = fx_media_format(&ram_disk, 
                            _fx_ram_driver,         /* Driver entry               */
                            ram_disk_memory,        /* RAM disk memory pointer    */
                            media_memory,           /* Media buffer pointer       */
                            sizeof(media_memory),   /* Media buffer size          */
                            "MY_RAM_DISK",          /* Volume Name                */
                            1,                      /* Number of FATs             */
                            32,                     /* Directory Entries          */
                            0,                      /* Hidden sectors             */
                            256,                    /* Total sectors              */
                            128,                    /* Sector size                */
                            1,                      /* Sectors per cluster        */
                            1,                      /* Heads                      */
                            1);                     /* Sectors per track          */

    /* Check the media format status.  */
    if (status != FX_SUCCESS)
        error_counter++;

    /* Open the RAM disk.  */
    status =  fx_media_open(&ram_disk, "RAM DISK", _fx_ram_driver, ram_disk_memory, media_memory, sizeof(media_memory));

    /* Check the media open status.  */
    if (status != FX_SUCCESS)
        error_counter++;

    /* Create an HTTP client instance.  */
    status = nx_http_client_create(&my_client, "HTTP Client", &client_ip, &client_pool, 600);

    /* Check status.  */
    if (status)
        error_counter++;

#ifdef __PRODUCT_NETXDUO__

    /* Now upload an HTML file to the HTTP IP server using the 'duo' service (supports IPv4 and IPv6). */
    status =  nxd_http_client_put_start(&my_client, &server_ip_address, "/index.htm", 
                                            "name", "password", 103, 5 * NX_IP_PERIODIC_RATE);
#else

    /* Now upload an HTML file to the HTTP IP server using the 'NetX' service (supports only IPv4). */
    status =  nx_http_client_put_start(&my_client, HTTP_SERVER_ADDRESS, "/index.htm", 
                                   "name", "password", 103, 5 * NX_IP_PERIODIC_RATE);
#endif

    /* Check status.  */
    if (status)
        error_counter++;

    /* Allocate a packet.  */
    status =  nx_packet_allocate(&client_pool, &send_packet, NX_TCP_PACKET, NX_WAIT_FOREVER);

    /* Check status.  */
    if(status)
        error_counter++;

    /* Build a simple 103-byte HTML page.  */
    nx_packet_data_append(send_packet, "<HTML>\r\n", 8, 
                        &client_pool, NX_WAIT_FOREVER);
    nx_packet_data_append(send_packet, 
                 "<HEAD><TITLE>NetX HTTP Test</TITLE></HEAD>\r\n", 44,
                        &client_pool, NX_WAIT_FOREVER);
    nx_packet_data_append(send_packet, "<BODY>\r\n", 8, 
                        &client_pool, NX_WAIT_FOREVER);
    nx_packet_data_append(send_packet, "<H1>Another NetX Test Page!</H1>\r\n", 25, 
                        &client_pool, NX_WAIT_FOREVER);
    nx_packet_data_append(send_packet, "</BODY>\r\n", 9,
                        &client_pool, NX_WAIT_FOREVER);
    nx_packet_data_append(send_packet, "</HTML>\r\n", 9,
                        &client_pool, NX_WAIT_FOREVER);

    /* Complete the PUT by writing the total length.  */
    status =  nx_http_client_put_packet(&my_client, send_packet, 1 * NX_IP_PERIODIC_RATE);
    if(status)
        error_counter++;

    /* Now GET the test file  */

#ifdef __PRODUCT_NETXDUO__ 

    /* Use the 'duo' service to send a GET request to the server (can use IPv4 or IPv6 addresses). */
    status =  nxd_http_client_get_start(&my_client, &server_ip_address, 
                                        "/index.htm", NX_NULL, 0, "name", "password", NX_IP_PERIODIC_RATE);
#else

    /* Use the 'NetX' service to send a GET request to the server (can only use IPv4 addresses). */
    status =  nx_http_client_get_start(&my_client, HTTP_SERVER_ADDRESS, "/index.htm", 
                                       NX_NULL, 0, "name", "password", NX_IP_PERIODIC_RATE);
#endif  /* USE_DUO */

    if(status)
        error_counter++;

    while (1)
    {
        status = nx_http_client_get_packet(&my_client, &recv_packet, 1 * NX_IP_PERIODIC_RATE);

        if (status == NX_HTTP_GET_DONE)
            break;

        if (status)
            error_counter++;
        else
            nx_packet_release(recv_packet);
    }

    status = nx_http_client_delete(&my_client);
    if(status)
        error_counter++;

    /* Create a socket.  */
    status = nx_tcp_socket_create(&client_ip, &client_socket, "Client Socket", 
                                  NX_IP_NORMAL, NX_FRAGMENT_OKAY, NX_IP_TIME_TO_LIVE, 1024,
                                  NX_NULL, NX_NULL);
    if(status)
        error_counter++;

    /* Bind the socket.  */
    status = nx_tcp_client_socket_bind(&client_socket, 50295, 1 * NX_IP_PERIODIC_RATE);
    if(status)
        error_counter++;

    /* Call connect to send an SYN.  */ 
    status = nx_tcp_client_socket_connect(&client_socket, HTTP_SERVER_ADDRESS, 80, 2 * NX_IP_PERIODIC_RATE);
    if(status)
        error_counter++;

    /* Allocate a packet.  */
    status = nx_packet_allocate(&client_pool, &my_packet, NX_TCP_PACKET, 1 * NX_IP_PERIODIC_RATE);
    if(status)
        error_counter++;

    /* Write POST packet into the packet payload.  */
    status = nx_packet_data_append(my_packet, pkt, sizeof(pkt), &client_pool, 1 * NX_IP_PERIODIC_RATE);
    if(status)
        error_counter++;

    /* Send the packet out.  */
    status = nx_tcp_socket_send(&client_socket, my_packet, 1 * NX_IP_PERIODIC_RATE);
    if(status)
        error_counter++;
    

    /* Receive the response from http server. */
    status =  nx_tcp_socket_receive(&client_socket, &recv_packet, 1 * NX_IP_PERIODIC_RATE);
    if(status)
        error_counter++;
    else
    {
        buffer_ptr = (CHAR *)recv_packet ->nx_packet_prepend_ptr;

        /* Check the status, If success , it should be 200. */
        if((buffer_ptr[9] != '2') || (buffer_ptr[10] != '0') || (buffer_ptr[11] != '0'))
            error_counter++;

        nx_packet_release(recv_packet);
    }

    tx_thread_sleep(1 * NX_IP_PERIODIC_RATE);
    
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


/* Define the helper HTTP server thread.  */
void    thread_server_entry(ULONG thread_input)
{

UINT            status;

    /* Print out test information banner.  */
    printf("NetX Test:   HTTP Post Basic Test......................................");

    /* Check for earlier error. */
    if(error_counter)
    {
        printf("ERROR!\n");
        test_control_return(1);
    }

    /* OK to start the HTTP Server.   */
    status = nx_http_server_start(&my_server);
    if(status)
        error_counter++;

    tx_thread_sleep(2 * NX_IP_PERIODIC_RATE);

    status = nx_http_server_delete(&my_server);
    if(status)
        error_counter++;

}


/* Test extended functions.  */
UINT    my_get_notify(NX_HTTP_SERVER *server_ptr, UINT request_type, CHAR *resource, NX_PACKET *packet_ptr)
{

UINT       status;
NX_PACKET *response_pkt;
UCHAR      data[] = "hello";

    /* Process multipart data. */
    if (request_type == NX_HTTP_SERVER_GET_REQUEST)
    {

        /* Generate HTTP header. */
        status = nx_http_server_callback_generate_response_header_extended(server_ptr, &response_pkt, NX_HTTP_STATUS_OK, sizeof(NX_HTTP_STATUS_OK) - 1, sizeof(data) - 1, "text/html", 9, "Server: NetXDuo HTTP 5.3\r\n", 26);
        if (status == NX_SUCCESS)
        {
            if (nx_http_server_callback_packet_send(server_ptr, response_pkt))
                nx_packet_release(response_pkt);
        }
        else
            error_counter++;

        status = nx_http_server_callback_data_send(server_ptr, data, sizeof(data) - 1);
        if (status)
            error_counter++;
    }
    else if (request_type == NX_HTTP_SERVER_POST_REQUEST)
    {
        status = nx_http_server_callback_response_send_extended(server_ptr, NX_HTTP_STATUS_OK, sizeof(NX_HTTP_STATUS_OK) - 1, "Server: NetXDuo HTTP 5.3\r\n", 26, NX_NULL, 0);
        if (status)
            error_counter++;
    }
    else
        return NX_SUCCESS;

    return(NX_HTTP_CALLBACK_COMPLETED);
}

#else

#ifdef CTEST
VOID test_application_define(void *first_unused_memory)
#else
void    netx_http_post_basic_test_application_define(void *first_unused_memory)
#endif
{

    /* Print out test information banner.  */
    printf("NetX Test:   HTTP Post Basic Test......................................N/A\n"); 

    test_control_return(3);  
}      
#endif
