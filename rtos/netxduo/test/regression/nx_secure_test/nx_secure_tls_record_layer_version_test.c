/* This tests if the TLS record layer version number in all unencrypted TLS records is ignored.  */

#include   "nx_api.h"
#include   "nx_secure_tls_api.h"
#include   "test_ca_cert.c"
#include   "test_device_cert.c"

extern VOID    test_control_return(UINT status);


#if !defined(NX_SECURE_TLS_CLIENT_DISABLED) && !defined(NX_SECURE_TLS_SERVER_DISABLED) && !defined(NX_SECURE_DISABLE_X509)
#define NUM_PACKETS                 24
#define PACKET_SIZE                 1536
#define PACKET_POOL_SIZE            (NUM_PACKETS * (PACKET_SIZE + sizeof(NX_PACKET)))
#define THREAD_STACK_SIZE           1024
#define ARP_CACHE_SIZE              1024
#define BUFFER_SIZE                 64
#define METADATA_SIZE               16000
#define CERT_BUFFER_SIZE            2048
#define SERVER_PORT                 4433

/* Define the ThreadX and NetX object control blocks...  */

static TX_THREAD                thread_0;
static TX_THREAD                thread_1;
static NX_PACKET_POOL           pool_0;
static NX_IP                    ip_0;
static UINT                     error_counter;

static NX_TCP_SOCKET            client_socket_0;
static NX_TCP_SOCKET            server_socket_0;
static NX_SECURE_TLS_SESSION    tls_client_session_0;
static NX_SECURE_TLS_SESSION    tls_server_session_0;
static NX_SECURE_X509_CERT      client_trusted_ca;
static NX_SECURE_X509_CERT      client_remote_cert;
static NX_SECURE_X509_CERT      server_local_certificate;
extern const NX_SECURE_TLS_CRYPTO
                                nx_crypto_tls_ciphers;

static ULONG                    pool_0_memory[PACKET_POOL_SIZE / sizeof(ULONG)];
static ULONG                    thread_0_stack[THREAD_STACK_SIZE / sizeof(ULONG)];
static ULONG                    thread_1_stack[THREAD_STACK_SIZE / sizeof(ULONG)];
static ULONG                    ip_0_stack[THREAD_STACK_SIZE / sizeof(ULONG)];
static ULONG                    arp_cache[ARP_CACHE_SIZE];
static UCHAR                    client_metadata[METADATA_SIZE];
static UCHAR                    server_metadata[METADATA_SIZE];
static UCHAR                    client_cert_buffer[CERT_BUFFER_SIZE];

static UCHAR                    request_buffer[BUFFER_SIZE];
static UCHAR                    response_buffer[BUFFER_SIZE];
static UCHAR                    tls_packet_buffer[2][4000];

static USHORT                   test_versions[] = {
    NX_SECURE_TLS_VERSION_TLS_1_0,
    NX_SECURE_TLS_VERSION_TLS_1_1,
    NX_SECURE_TLS_VERSION_TLS_1_2,
};

/* Define thread prototypes.  */

static VOID    ntest_0_entry(ULONG thread_input);
static VOID    ntest_1_entry(ULONG thread_input);
extern VOID    _nx_ram_network_driver_1500(struct NX_IP_DRIVER_STRUCT *driver_req);


/* Define what the initial system looks like.  */


#define ERROR_COUNTER(status) _ERROR_COUNTER(status, __FILE__, __LINE__)

static VOID    _ERROR_COUNTER(UINT status, const char *file, int line)
{
	printf("Error (status = 0x%x) at %s:%d\n", status, file, line);
    error_counter++;
}

#ifdef CTEST
void test_application_define(void *first_unused_memory);
void test_application_define(void *first_unused_memory)
#else
VOID    nx_secure_tls_record_layer_version_test_application_define(void *first_unused_memory)
#endif
{
UINT     status;
CHAR    *pointer;


    error_counter = 0;


    /* Setup the working pointer.  */
    pointer =  (CHAR *) first_unused_memory;

    /* Create the server thread.  */
    tx_thread_create(&thread_0, "thread 0", ntest_0_entry, 0,
                     thread_0_stack, sizeof(thread_0_stack),
                     7, 7, TX_NO_TIME_SLICE, TX_AUTO_START);

    /* Create the client thread.  */
    tx_thread_create(&thread_1, "thread 1", ntest_1_entry, 0,
                     thread_1_stack, sizeof(thread_1_stack),
                     8, 8, TX_NO_TIME_SLICE, TX_AUTO_START);

    /* Initialize the NetX system.  */
    nx_system_initialize();

    /* Create a packet pool.  */
    status =  nx_packet_pool_create(&pool_0, "NetX Main Packet Pool", PACKET_SIZE,
                                    pool_0_memory, PACKET_POOL_SIZE);
    if (status)
    {
        ERROR_COUNTER(status);
    }

    /* Create an IP instance.  */
    status = nx_ip_create(&ip_0, "NetX IP Instance 0", IP_ADDRESS(1, 2, 3, 4), 0xFFFFFF00UL,
                          &pool_0, _nx_ram_network_driver_1500,
                          ip_0_stack, sizeof(ip_0_stack), 1);
    if (status)
    {
        ERROR_COUNTER(status);
    }

    /* Enable ARP and supply ARP cache memory for IP Instance 0.  */
    status =  nx_arp_enable(&ip_0, (VOID *)arp_cache, sizeof(arp_cache));
    if (status)
    {
        ERROR_COUNTER(status);
    }

    /* Enable TCP traffic.  */
    status =  nx_tcp_enable(&ip_0);
    if (status)
    {
        ERROR_COUNTER(status);
    }

    nx_secure_tls_initialize();
}

static VOID client_tls_setup(NX_SECURE_TLS_SESSION *tls_session_ptr)
{
UINT status;

    status = nx_secure_tls_session_create(tls_session_ptr,
                                           &nx_crypto_tls_ciphers,
                                           client_metadata,
                                           sizeof(client_metadata));
    if (status)
    {
        ERROR_COUNTER(status);
    }

    memset(&client_remote_cert, 0, sizeof(client_remote_cert));
    status = nx_secure_tls_remote_certificate_allocate(tls_session_ptr,
                                                       &client_remote_cert,
                                                       client_cert_buffer,
                                                       sizeof(client_cert_buffer));
    if (status)
    {
        ERROR_COUNTER(status);
    }

    status = nx_secure_x509_certificate_initialize(&client_trusted_ca,
                                                   test_ca_cert_der,
                                                   test_ca_cert_der_len, NX_NULL, 0, NULL, 0, NX_SECURE_X509_KEY_TYPE_NONE);
    if (status)
    {
        ERROR_COUNTER(status);
    }

    status = nx_secure_tls_trusted_certificate_add(tls_session_ptr,
                                                   &client_trusted_ca);
    if (status)
    {
        ERROR_COUNTER(status);
    }

    status = nx_secure_tls_session_packet_buffer_set(tls_session_ptr, tls_packet_buffer[0],
                                                     sizeof(tls_packet_buffer[0]));
    if (status)
    {
        ERROR_COUNTER(status);
    }

}

static VOID server_tls_setup(NX_SECURE_TLS_SESSION *tls_session_ptr)
{
UINT status;

    status = nx_secure_tls_session_create(tls_session_ptr,
                                           &nx_crypto_tls_ciphers,
                                           server_metadata,
                                           sizeof(server_metadata));
    if (status)
    {
        ERROR_COUNTER(status);
    }

    memset(&server_local_certificate, 0, sizeof(server_local_certificate));
    status = nx_secure_x509_certificate_initialize(&server_local_certificate,
                                                   test_device_cert_der, test_device_cert_der_len,
                                                   NX_NULL, 0, test_device_cert_key_der,
                                                   test_device_cert_key_der_len, NX_SECURE_X509_KEY_TYPE_RSA_PKCS1_DER);
    if (status)
    {
        ERROR_COUNTER(status);
    }

    status = nx_secure_tls_local_certificate_add(tls_session_ptr,
                                                 &server_local_certificate);
    if (status)
    {
        ERROR_COUNTER(status);
    }

    status = nx_secure_tls_session_packet_buffer_set(tls_session_ptr, tls_packet_buffer[1],
                                                     sizeof(tls_packet_buffer[1]));
    if (status)
    {
        ERROR_COUNTER(status);
    }

}

static void ntest_0_entry(ULONG thread_input)
{
UINT i;
UINT status;
ULONG response_length;
NX_PACKET *packet_ptr;

    /* Print out test information banner.  */
    printf("NetX Secure Test:   TLS Record Layer Version Test......................");

    /* Create TCP socket. */
    status = nx_tcp_socket_create(&ip_0, &server_socket_0, "Server socket", NX_IP_NORMAL,
                                  NX_DONT_FRAGMENT, NX_IP_TIME_TO_LIVE, 8192, NX_NULL, NX_NULL);
    if (status)
    {
        ERROR_COUNTER(status);
    }

    status = nx_tcp_server_socket_listen(&ip_0, SERVER_PORT, &server_socket_0, 5, NX_NULL);
    if (status)
    {
        ERROR_COUNTER(status);
    }

    for (i = 0; i < sizeof(test_versions) / sizeof(USHORT); i++)
    {

        /* Make sure client thread is ready. */
        tx_thread_suspend(&thread_0);

        server_tls_setup(&tls_server_session_0);

        status = nx_tcp_server_socket_accept(&server_socket_0, NX_WAIT_FOREVER);
        if (status)
        {
            ERROR_COUNTER(status);
        }

        /* Start TLS session. */
        status = nx_secure_tls_session_start(&tls_server_session_0, &server_socket_0,
                                              NX_WAIT_FOREVER);
        if (status)
        {
            ERROR_COUNTER(status);
        }
        
        status = nx_secure_tls_session_receive(&tls_server_session_0, &packet_ptr, NX_WAIT_FOREVER);
        if (status)
        {
            ERROR_COUNTER(status);
        }

        nx_packet_data_retrieve(packet_ptr, response_buffer, &response_length);
        nx_packet_release(packet_ptr);
        if ((response_length != sizeof(request_buffer)) ||
            memcmp(request_buffer, response_buffer, response_length))
        {
            ERROR_COUNTER(status);
        }

        nx_secure_tls_session_end(&tls_server_session_0, NX_NO_WAIT);
        nx_secure_tls_session_delete(&tls_server_session_0);

        nx_tcp_socket_disconnect(&server_socket_0, NX_NO_WAIT);
        nx_tcp_server_socket_unaccept(&server_socket_0);
        nx_tcp_server_socket_relisten(&ip_0, SERVER_PORT, &server_socket_0);
    }

    if (error_counter)
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

static void ntest_1_entry(ULONG thread_input)
{
UINT i, j;
UINT status;
NX_PACKET *send_packet;;
NXD_ADDRESS server_address;

    server_address.nxd_ip_version = NX_IP_VERSION_V4;
    server_address.nxd_ip_address.v4 = IP_ADDRESS(127, 0, 0, 1);

    /* Create TCP socket. */
    status = nx_tcp_socket_create(&ip_0, &client_socket_0, "Client socket", NX_IP_NORMAL,
                                  NX_DONT_FRAGMENT, NX_IP_TIME_TO_LIVE, 8192, NX_NULL, NX_NULL);
    if (status)
    {
        ERROR_COUNTER(status);
    }

    status = nx_tcp_client_socket_bind(&client_socket_0, NX_ANY_PORT, NX_NO_WAIT);
    if (status)
    {
        ERROR_COUNTER(status);
    }

    for (i = 0; i < sizeof(test_versions) / sizeof(USHORT); i++)
    {

        /* Let server thread run first. */
        tx_thread_resume(&thread_0);

        for (j = 0; j < sizeof(request_buffer); j++)
        {
            request_buffer[j] = j;
            response_buffer[j] = 0;
        }

        client_tls_setup(&tls_client_session_0);

        status =  nxd_tcp_client_socket_connect(&client_socket_0, &server_address, SERVER_PORT,
                                                NX_WAIT_FOREVER);
        if (status)
        {
            ERROR_COUNTER(status);
        }

        /* Start TLS session. */
        tx_mutex_get(&_nx_secure_tls_protection, TX_WAIT_FOREVER);
        tls_client_session_0.nx_secure_tls_packet_pool = client_socket_0.nx_tcp_socket_ip_ptr -> nx_ip_default_packet_pool;
        tls_client_session_0.nx_secure_tls_tcp_socket = &client_socket_0;
        tls_client_session_0.nx_secure_record_queue_header = NX_NULL;
        tls_client_session_0.nx_secure_record_decrypted_packet = NX_NULL;
        tls_client_session_0.nx_secure_tls_local_session_active = 0;
        tls_client_session_0.nx_secure_tls_remote_session_active = 0;
        tls_client_session_0.nx_secure_tls_received_remote_credentials = NX_FALSE;
        tls_client_session_0.nx_secure_tls_received_alert_level = 0;
        tls_client_session_0.nx_secure_tls_received_alert_value = 0;
        tls_client_session_0.nx_secure_tls_socket_type = NX_SECURE_TLS_SESSION_TYPE_CLIENT;
#if (NX_SECURE_TLS_TLS_1_3_ENABLED)
        /* Initialize TLS 1.3 cryptographic primitives. */
        if(tls_client_session_0.nx_secure_tls_1_3)
        {
            status = _nx_secure_tls_1_3_crypto_init(&tls_client_session_0);

            if(status != NX_SUCCESS)
            {
                ERROR_COUNTER(status);
            }
        }
#endif
        /* Allocate a handshake packet so we can send the ClientHello. */
        status = _nx_secure_tls_allocate_handshake_packet(&tls_client_session_0, tls_client_session_0.nx_secure_tls_packet_pool, &send_packet, NX_WAIT_FOREVER);
        if (status)
        {
            ERROR_COUNTER(status);
        }

        /* Populate our packet with clienthello data. */
        status = _nx_secure_tls_send_clienthello(&tls_client_session_0, send_packet);
        if (status)
        {
            ERROR_COUNTER(status);
        }

        tls_client_session_0.nx_secure_tls_protocol_version = test_versions[i];

        /* Send the ClientHello to kick things off. */
        status = _nx_secure_tls_send_handshake_record(&tls_client_session_0, send_packet, NX_SECURE_TLS_CLIENT_HELLO, NX_WAIT_FOREVER);
        if (status)
        {
            ERROR_COUNTER(status);
        }

        tls_client_session_0.nx_secure_tls_protocol_version = 0;

        tx_mutex_put(&_nx_secure_tls_protection);

        /* Now handle our incoming handshake messages. Continue processing until the handshake is complete
           or an error/timeout occurs. */
        status = _nx_secure_tls_handshake_process(&tls_client_session_0, NX_WAIT_FOREVER);
        if (status)
        {
            ERROR_COUNTER(status);
        }

        /* Prepare packet to send. */
        status = nx_packet_allocate(&pool_0, &send_packet, NX_TCP_PACKET, NX_NO_WAIT);
        if (status)
        {
            ERROR_COUNTER(status);
        }

        send_packet -> nx_packet_prepend_ptr += NX_SECURE_TLS_RECORD_HEADER_SIZE;
        send_packet -> nx_packet_append_ptr = send_packet -> nx_packet_prepend_ptr;

        status = nx_packet_data_append(send_packet, request_buffer, sizeof(request_buffer),
                                       &pool_0, NX_NO_WAIT);
        if (status)
        {
            ERROR_COUNTER(status);
        }

        /* Send the packet. */
        status = nx_secure_tls_session_send(&tls_client_session_0, send_packet, NX_NO_WAIT);
        if (status)
        {
            ERROR_COUNTER(status);
        }

        nx_secure_tls_session_end(&tls_client_session_0, NX_NO_WAIT);
        nx_secure_tls_session_delete(&tls_client_session_0);

        nx_tcp_socket_disconnect(&client_socket_0, NX_NO_WAIT);
    }
}
#else
#ifdef CTEST
void test_application_define(void *first_unused_memory);
void test_application_define(void *first_unused_memory)
#else
VOID    nx_secure_tls_record_layer_version_test_application_define(void *first_unused_memory)
#endif
{

    /* Print out test information banner.  */
    printf("NetX Secure Test:   TLS Record Layer Version Test......................N/A\n");
    test_control_return(3);
}
#endif
