/* This NetX test concentrates on the TCP data transfer operation.  */

#include   "tx_api.h"
#include   "nx_api.h"
#include   "nx_tcp.h"
#include   "nx_ip.h"

extern void    test_control_return(UINT status);
#if !defined(NX_DISABLE_IPV4)

#define     DEMO_STACK_SIZE         2048


/* Define the ThreadX and NetX object control blocks...  */

static TX_THREAD               thread_0;
static TX_THREAD               thread_1;

static NX_PACKET_POOL          pool_0;
static NX_IP                   ip_0;
static NX_IP                   ip_1;
static NX_TCP_SOCKET           client_socket;
static NX_TCP_SOCKET           server_socket;



/* Define the counters used in the demo application...  */

static ULONG                   thread_0_counter =  0;
static ULONG                   thread_1_counter =  0;
static ULONG                   error_counter =     0;
static ULONG                   connections =       0;
static ULONG                   disconnections =    0;
static ULONG                   client_receives =   0;
static ULONG                   server_receives =   0;
static UINT                    client_port;


/* Define thread prototypes.  */

static void    thread_0_entry(ULONG thread_input);
static void    thread_1_entry(ULONG thread_input);
static void    thread_1_connect_received(NX_TCP_SOCKET *server_socket, UINT port);
static void    thread_1_disconnect_received(NX_TCP_SOCKET *server_socket);

static void    thread_0_receive_notify(NX_TCP_SOCKET *client_socket);
static void    thread_1_receive_notify(NX_TCP_SOCKET *server_socket);
extern void    _nx_ram_network_driver_256(struct NX_IP_DRIVER_STRUCT *driver_req);
static void    tcp_packet_receive(NX_IP *ip_ptr, NX_PACKET *packet_ptr);


/* Define what the initial system looks like.  */

#ifdef CTEST
VOID test_application_define(void *first_unused_memory)
#else
void    netx_tcp_data_transfer_test_application_define(void *first_unused_memory)
#endif
{

CHAR    *pointer;
UINT    status;

    
    /* Setup the working pointer.  */
    pointer =  (CHAR *) first_unused_memory;

    thread_0_counter =  0;
    thread_1_counter =  0;
    error_counter =     0;
    connections =       0;
    disconnections =    0;
    client_receives =   0;
    server_receives =   0;
    client_port =       0;

    /* Create the main thread.  */
    tx_thread_create(&thread_0, "thread 0", thread_0_entry, 0,  
            pointer, DEMO_STACK_SIZE, 
            4, 4, TX_NO_TIME_SLICE, TX_AUTO_START);

    pointer =  pointer + DEMO_STACK_SIZE;

    /* Create the main thread.  */
    tx_thread_create(&thread_1, "thread 1", thread_1_entry, 0,  
            pointer, DEMO_STACK_SIZE, 
            3, 3, TX_NO_TIME_SLICE, TX_AUTO_START);

    pointer =  pointer + DEMO_STACK_SIZE;


    /* Initialize the NetX system.  */
    nx_system_initialize();

    /* Create a packet pool.  */
    status =  nx_packet_pool_create(&pool_0, "NetX Main Packet Pool", 256, pointer, 8192);
    pointer = pointer + 8192;

    if (status)
        error_counter++;

    /* Create an IP instance.  */
    status = nx_ip_create(&ip_0, "NetX IP Instance 0", IP_ADDRESS(1, 2, 3, 4), 0xFFFFFF00UL, &pool_0, _nx_ram_network_driver_256,
                    pointer, 2048, 1);
    pointer =  pointer + 2048;

    /* Create another IP instance.  */
    status += nx_ip_create(&ip_1, "NetX IP Instance 1", IP_ADDRESS(1, 2, 3, 5), 0xFFFFFF00UL, &pool_0, _nx_ram_network_driver_256,
                    pointer, 2048, 1);
    pointer =  pointer + 2048;

    if (status)
        error_counter++;

    /* Enable ARP and supply ARP cache memory for IP Instance 0.  */
    status =  nx_arp_enable(&ip_0, (void *) pointer, 1024);
    pointer = pointer + 1024;

    /* Enable ARP and supply ARP cache memory for IP Instance 1.  */
    status +=  nx_arp_enable(&ip_1, (void *) pointer, 1024);
    pointer = pointer + 1024;

    /* Check ARP enable status.  */
    if (status)
        error_counter++;

    /* Enable TCP processing for both IP instances.  */
    status =  nx_tcp_enable(&ip_0);
    status += nx_tcp_enable(&ip_1);

    /* Check TCP enable status.  */
    if (status)
        error_counter++;
}



/* Define the test threads.  */

static void    thread_0_entry(ULONG thread_input)
{

UINT        status;
NX_PACKET   *my_packet;
UINT        compare_port;
ULONG       tcp_packets_sent, tcp_bytes_sent, tcp_packets_received, tcp_bytes_received, tcp_invalid_packets, tcp_receive_packets_dropped, tcp_checksum_errors, tcp_connections, tcp_disconnections, tcp_connections_dropped, tcp_retransmit_packets;
ULONG       packets_sent, bytes_sent, packets_received, bytes_received, retransmit_packets, packets_queued, checksum_errors, socket_state, transmit_queue_depth, transmit_window, receive_window;

    /* Print out some test information banners.  */
    printf("NetX Test:   TCP Data Transfer Test....................................");

    /* Check for earlier error.  */
    if (error_counter)
    {

        printf("ERROR!\n");
        test_control_return(1);
    }

    /* Get a free port for the client's use.  */
    status =  nx_tcp_free_port_find(&ip_0, 12, &client_port);

    /* Check for error.  */
    if (status)
    {

        printf("ERROR!\n");
        test_control_return(1);
    }

    /* Create a socket.  */
    status =  nx_tcp_socket_create(&ip_0, &client_socket, "Client Socket", 
                                NX_IP_NORMAL, NX_FRAGMENT_OKAY, NX_IP_TIME_TO_LIVE, 200,
                                NX_NULL, NX_NULL);
                                
    /* Check for error.  */
    if (status)
        error_counter++;

    /* Setup a receive notify function.  */
    status =  nx_tcp_socket_receive_notify(&client_socket, thread_0_receive_notify);

    /* Check for error.  */
    if (status)
        error_counter++;

    /* Bind the socket.  */
    status =  nx_tcp_client_socket_bind(&client_socket, client_port, NX_WAIT_FOREVER);

    /* Check for error.  */
    if (status)
        error_counter++;

    /* Pickup the port for the client socket.  */
    status =  nx_tcp_client_socket_port_get(&client_socket, &compare_port);

    /* Check for error.  */
    if ((status) || (client_port != compare_port))
    {

        printf("ERROR!\n");
        test_control_return(1);
    }

    /* Attempt to connect the socket.  */
    status =  nx_tcp_client_socket_connect(&client_socket, IP_ADDRESS(1, 2, 3, 5), 12, 5 * NX_IP_PERIODIC_RATE);

    /* Check for error.  */
    if (status)
        error_counter++;

    /* Wait for established state.  */
    status =  nx_tcp_socket_state_wait(&client_socket, NX_TCP_ESTABLISHED, 5 * NX_IP_PERIODIC_RATE);
        
    /* Check for error.  */
    if (status)
        error_counter++;

    /* Loop to send 1000 messages!  */
    while ((thread_0_counter < 1000) && (error_counter == 0))
    {

        /* Increment thread 0's counter.  */
        thread_0_counter++;

        /* Allocate a packet.  */
        status =  nx_packet_allocate(&pool_0, &my_packet, NX_TCP_PACKET, NX_WAIT_FOREVER);

        /* Check status.  */
        if (status != NX_SUCCESS)
            break;

        /* Write ABCs into the packet payload!  */
        memcpy(my_packet -> nx_packet_prepend_ptr, "ABCDEFGHIJKLMNOPQRSTUVWXYZ  ", 28);

        /* Adjust the write pointer.  */
        my_packet -> nx_packet_length =  28;
        my_packet -> nx_packet_append_ptr =  my_packet -> nx_packet_prepend_ptr + 28;

        /* Send the packet out!  */
        status =  nx_tcp_socket_send(&client_socket, my_packet, 5 * NX_IP_PERIODIC_RATE);

        /* Determine if the status is valid.  */
        if (status)
        {
            error_counter++;
            nx_packet_release(my_packet);
        }
    }

    /* Disconnect this socket.  */
    status =  nx_tcp_socket_disconnect(&client_socket, 5 * NX_IP_PERIODIC_RATE);

    /* Determine if the status is valid.  */
    if (status)
        error_counter++;

#if !defined(FEATURE_NX_IPV6) && defined(__PRODUCT_NETXDUO__)
    /* Sleep 2MSL and check the state of socket from timed_wait to closed. */
    if (client_socket.nx_tcp_socket_state != NX_TCP_TIMED_WAIT)
    {
        error_counter++;
    }
    else
    {
        tx_thread_sleep(_nx_tcp_2MSL_timer_rate + 1);
        if (client_socket.nx_tcp_socket_state != NX_TCP_CLOSED)
        {
            error_counter++;
        }
    }
#endif /* FEATURE_NX_IPV6 */

    /* Unbind the socket.  */
    status =  nx_tcp_client_socket_unbind(&client_socket);

    /* Check for error.  */
    if (status)
        error_counter++;

    /* Get information about this socket.  */
    status =  nx_tcp_socket_info_get(&client_socket, &packets_sent, &bytes_sent, 
                            &packets_received, &bytes_received, 
                            &retransmit_packets, &packets_queued,
                            &checksum_errors, &socket_state,
                            &transmit_queue_depth, &transmit_window,
                            &receive_window);

#ifndef NX_DISABLE_TCP_INFO
    if((packets_sent != 1000) || (bytes_sent != 1000*28))
        error_counter++;
#endif

    /* Check for errors.  */
    if ((error_counter) || (status) || (packets_received) || (bytes_received) ||
        (retransmit_packets) || (packets_queued) || (checksum_errors) || (socket_state != NX_TCP_CLOSED) ||
        (transmit_queue_depth) || (transmit_window != 100) || (receive_window != 200))
    {

        printf("ERROR!\n");
        test_control_return(1);
    }

    /* Delete the socket.  */
    status =  nx_tcp_socket_delete(&client_socket);

    /* Check for error.  */
    if (status)
        error_counter++;

    /* Get the overall TCP information.  */
    status =  nx_tcp_info_get(&ip_0, &tcp_packets_sent, &tcp_bytes_sent, &tcp_packets_received, &tcp_bytes_received,
                             &tcp_invalid_packets, &tcp_receive_packets_dropped, &tcp_checksum_errors, &tcp_connections, 
                             &tcp_disconnections, &tcp_connections_dropped, &tcp_retransmit_packets);

#ifndef NX_DISABLE_TCP_INFO
    if((tcp_packets_sent != 1000) || (tcp_bytes_sent != 1000*28) || (tcp_connections != 1))
        error_counter++;
#endif
 
    /* Check status.  */
    if ((error_counter) || (status) || (thread_0_counter != 1000) || (thread_1_counter != 1000) || (connections != 1) || (disconnections) ||
        (tcp_packets_received) || (tcp_bytes_received) || (tcp_invalid_packets) || (tcp_receive_packets_dropped) || (tcp_checksum_errors) ||
        (tcp_connections_dropped) || (tcp_retransmit_packets))
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
    

static void    thread_1_entry(ULONG thread_input)
{

UINT            status;
NX_PACKET       *packet_ptr;
ULONG           actual_status;


    /* Ensure the IP instance has been initialized.  */
    status =  nx_ip_status_check(&ip_1, NX_IP_INITIALIZE_DONE, &actual_status, NX_IP_PERIODIC_RATE);

    /* Check status...  */
    if (status != NX_SUCCESS)
    {

        error_counter++;
        test_control_return(1);
    }

    /* Create a socket.  */
    status =  nx_tcp_socket_create(&ip_1, &server_socket, "Server Socket", 
                                NX_IP_NORMAL, NX_FRAGMENT_OKAY, NX_IP_TIME_TO_LIVE, 100,
                                NX_NULL, thread_1_disconnect_received);
                                
    /* Check for error.  */
    if (status)
        error_counter++;

    /* Setup a receive notify function.  */
    status =  nx_tcp_socket_receive_notify(&server_socket, thread_1_receive_notify);

    /* Check for error.  */
    if (status)
        error_counter++;

    /* Configure the socket further.  */
    status =  nx_tcp_socket_transmit_configure(&server_socket, 10, 300, 10, 0);

    /* Check for error.  */
    if (status)
        error_counter++;

    /* Setup this thread to listen.  */
    status =  nx_tcp_server_socket_listen(&ip_1, 12, &server_socket, 5, thread_1_connect_received);

    /* Check for error.  */
    if (status)
        error_counter++;

    /* Inject the SYN packet. */
    ip_1.nx_ip_tcp_packet_receive = tcp_packet_receive;

    /* Accept a client socket connection.  */
    status =  nx_tcp_server_socket_accept(&server_socket, 5 * NX_IP_PERIODIC_RATE);

    /* Check for error.  */
    if (status)
        error_counter++;

    /* Loop to accept 1000 messages.  */
    while((thread_1_counter < 1000) && (error_counter == 0))
    {

        /* Increment thread 1's counter.  */
        thread_1_counter++;

        /* Receive a TCP message from the socket.  */
        status =  nx_tcp_socket_receive(&server_socket, &packet_ptr, 5 * NX_IP_PERIODIC_RATE);

        /* Check for error.  */
        if (status)
            error_counter++;
        else
            /* Release the packet.  */
            nx_packet_release(packet_ptr);
    }

    /* Let client disconnect first. */
    tx_thread_sleep(NX_IP_PERIODIC_RATE);

    /* Disconnect the server socket.  */
    status =  nx_tcp_socket_disconnect(&server_socket, 5 * NX_IP_PERIODIC_RATE);

    /* Check for error.  */
    if (status)
        error_counter++;

    /* Unaccept the server socket.  */
    status =  nx_tcp_server_socket_unaccept(&server_socket);

    /* Check for error.  */
    if (status)
        error_counter++;

    /* Setup server socket for listening again.  */
    status =  nx_tcp_server_socket_relisten(&ip_1, 12, &server_socket);

    /* Check for error.  */
    if (status)
        error_counter++;

    /* Unlisten on the server port.  */
    status =  nx_tcp_server_socket_unlisten(&ip_1, 12);

    /* Check for error.  */
    if (status)
        error_counter++;
}


static void  thread_1_connect_received(NX_TCP_SOCKET *socket_ptr, UINT port)
{

    /* Check for the proper socket and port.  */
    if ((socket_ptr != &server_socket) || (port != 12))
        error_counter++;
    else
        connections++;
}


static void  thread_1_disconnect_received(NX_TCP_SOCKET *socket)
{

    /* Check for proper disconnected socket.  */
    if (socket != &server_socket)
        error_counter++;
}

static void  thread_0_receive_notify(NX_TCP_SOCKET *client_socket)
{

    client_receives++;
}


static void  thread_1_receive_notify(NX_TCP_SOCKET *server_socket)
{

    server_receives++;
}

static void    tcp_packet_receive(NX_IP *ip_ptr, NX_PACKET *packet_ptr)
{
#if defined(NX_ENABLE_INTERFACE_CAPABILITY) && defined(__PRODUCT_NETXDUO__)
NX_IPV4_HEADER *ip_header;   
NX_PACKET      *loopback_src_port;
NX_PACKET      *broadcast_source;
    
    /* Set the source IP equal to destination IP. Verify whether or not system crashes. */
    /* Set the interface capability to ignore the checksum.  */
    packet_ptr -> nx_packet_address.nx_packet_interface_ptr -> nx_interface_capability_flag |= NX_INTERFACE_CAPABILITY_TCP_RX_CHECKSUM; 

    /* Copy from the original packet. */
    nx_packet_copy(packet_ptr, &loopback_src_port, &pool_0, NX_NO_WAIT);   
    nx_packet_copy(packet_ptr, &broadcast_source, &pool_0, NX_NO_WAIT);   

    /* Get IP header. */
    ip_header = (NX_IPV4_HEADER *)loopback_src_port -> nx_packet_ip_header;

    /* Copy the dest ip to source ip field. */
    ip_header -> nx_ip_header_source_ip = ip_header -> nx_ip_header_destination_ip;

    /* Pass the packet. */
    _nx_tcp_packet_receive(ip_ptr, loopback_src_port);

#ifndef NX_DISABLE_TCP_INFO
    if (ip_ptr -> nx_ip_tcp_invalid_packets != 1)
        error_counter++;
#endif /* NX_DISABLE_TCP_INFO */

    /* Get IP header. */
    ip_header = (NX_IPV4_HEADER *)broadcast_source -> nx_packet_ip_header;

    /* Copy the dest ip to source ip field. */
    ip_header -> nx_ip_header_source_ip = NX_IP_LIMITED_BROADCAST;

    /* Pass the packet. */
    _nx_tcp_packet_receive(ip_ptr, broadcast_source);

#ifndef NX_DISABLE_TCP_INFO
    if (ip_ptr -> nx_ip_tcp_invalid_packets != 2)
        error_counter++;
#endif /* NX_DISABLE_TCP_INFO */
#endif /* NX_ENABLE_INTERFACE_CAPABILITY */

    /* Restore tcp receive function pointer. */
    ip_ptr -> nx_ip_tcp_packet_receive = _nx_tcp_packet_receive;

    /* Pass the packet. */
    _nx_tcp_packet_receive(ip_ptr, packet_ptr);
}
#else

#ifdef CTEST
VOID test_application_define(void *first_unused_memory)
#else
void    netx_tcp_data_transfer_test_application_define(void *first_unused_memory)
#endif
{

    /* Print out test information banner.  */
    printf("NetX Test:   TCP Data Transfer Test....................................N/A\n"); 

    test_control_return(3);  
}      
#endif
