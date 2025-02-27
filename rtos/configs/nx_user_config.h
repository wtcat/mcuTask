/***************************************************************************
 * Copyright (c) 2024 Microsoft Corporation 
 * 
 * This program and the accompanying materials are made available under the
 * terms of the MIT License which is available at
 * https://opensource.org/licenses/MIT.
 * 
 * SPDX-License-Identifier: MIT
 **************************************************************************/


/**************************************************************************/
/**************************************************************************/
/**                                                                       */
/** NetX Component                                                        */
/**                                                                       */
/**   User Specific                                                       */
/**                                                                       */
/**************************************************************************/
/**************************************************************************/


/**************************************************************************/
/*                                                                        */
/*  PORT SPECIFIC C INFORMATION                            RELEASE        */
/*                                                                        */
/*    nx_user.h                                           PORTABLE C      */
/*                                                           6.3.0        */
/*                                                                        */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Yuxin Zhou, Microsoft Corporation                                   */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */
/*    This file contains user defines for configuring NetX in specific    */
/*    ways. This file will have an effect only if the application and     */
/*    NetX library are built with NX_INCLUDE_USER_DEFINE_FILE defined.    */
/*    Note that all the defines in this file may also be made on the      */
/*    command line when building NetX library and application objects.    */
/*                                                                        */
/*  RELEASE HISTORY                                                       */
/*                                                                        */
/*    DATE              NAME                      DESCRIPTION             */
/*                                                                        */
/*  05-19-2020     Yuxin Zhou               Initial Version 6.0           */
/*  09-30-2020     Yuxin Zhou               Modified comment(s),          */
/*                                            resulting in version 6.1    */
/*  08-02-2021     Yuxin Zhou               Modified comment(s), and      */
/*                                            supported TCP/IP offload,   */
/*                                            resulting in version 6.1.8  */
/*  04-25-2022     Yuxin Zhou               Modified comment(s),          */
/*                                            resulting in version 6.1.11 */
/*  10-31-2023     Tiejun Zhou              Modified comment(s),          */
/*                                            supported random IP id,     */
/*                                            resulting in version 6.3.0  */
/*                                                                        */
/**************************************************************************/

#ifndef NX_USER_CONFIG_H
#define NX_USER_CONFIG_H


/* Define various build options for the NetX Duo port.  The application should either make changes
   here by commenting or un-commenting the conditional compilation defined OR supply the defines
   though the compiler's equivalent of the -D option.  */


/* Override various options with default values already assigned in nx_api.h or nx_port.h. Please
   also refer to nx_port.h for descriptions on each of these options.  */


/* Configuration options for Interface */

/* NX_MAX_PHYSICAL_INTERFACES defines the number physical network interfaces
   present to NetX Duo IP layer.  Physical interface does not include
   loopback interface. By default there is at least one physical interface
   in the system. */
#define NX_MAX_PHYSICAL_INTERFACES    CONFIG_NX_MAX_PHYSICAL_INTERFACES

/* Defined, this option disables NetX Duo support on the 127.0.0.1 loopback interface.
   127.0.0.1 loopback interface is enabled by default.  Uncomment out the follow code to disable
   the loopback interface. */
#ifdef CONFIG_NX_DISABLE_LOOPBACK_INTERFACE
#define NX_DISABLE_LOOPBACK_INTERFACE
#endif

/* If defined, the link driver is able to specify extra capability, such as checksum offloading features. */
#ifdef CONFIG_NX_ENABLE_INTERFACE_CAPABILITY
#define NX_ENABLE_INTERFACE_CAPABILITY
#endif


/* Configuration options for IP */

/* This defines specifies the number of ThreadX timer ticks in one second. The default value is based
   on ThreadX timer interrupt.  */
#define NX_IP_PERIODIC_RATE         CONFIG_TX_TIMER_TICKS_PER_SECOND


/* Defined, NX_ENABLE_IP_RAW_PACKET_FILTER allows an application to install a filter
   for incoming raw packets. This feature is disabled by default. */
#ifdef CONFIG_NX_ENABLE_IP_RAW_PACKET_FILTER
#define NX_ENABLE_IP_RAW_PACKET_FILTER
#endif

/* This define specifies the maximum number of RAW packets can be queued for receive.  The default
   value is 20.  */
#define NX_IP_RAW_MAX_QUEUE_DEPTH  CONFIG_NX_IP_RAW_MAX_QUEUE_DEPTH


/* Defined, this option enables IP static routing feature.  By default IP static routing
   feature is not compiled in. */
#ifdef CONFIG_NX_ENABLE_IP_STATIC_ROUTING
#define NX_ENABLE_IP_STATIC_ROUTING
#endif

/* This define specifies the size of IP routing table. The default value is 8. */
#define NX_IP_ROUTING_TABLE_SIZE  CONFIG_NX_IP_ROUTING_TABLE_SIZE


/* Defined, this option enables random IP id. By default IP id is increased by one for each packet. */
#ifdef CONFIG_NX_ENABLE_IP_ID_RANDOMIZATION
#define NX_ENABLE_IP_ID_RANDOMIZATION
#endif

/* This define specifies the maximum number of multicast groups that can be joined.
   The default value is 7.  */
#define NX_MAX_MULTICAST_GROUPS     CONFIG_NX_MAX_MULTICAST_GROUPS


/* Configuration options for IPv6 */

/* Disable IPv6 processing in NetX Duo.  */
#ifdef CONFIG_NX_DISABLE_IPV6
#define NX_DISABLE_IPV6
#endif

/* Define the number of entries in IPv6 address pool. */
#ifdef NX_MAX_PHYSICAL_INTERFACES
#define NX_MAX_IPV6_ADDRESSES (NX_MAX_PHYSICAL_INTERFACES * 3)
#endif


/* Do not process IPv6 ICMP Redirect Messages. */
#ifdef CONFIG_NX_DISABLE_ICMPV6_REDIRECT_PROCESS
#define NX_DISABLE_ICMPV6_REDIRECT_PROCESS
#endif

/* Do not process IPv6 Router Advertisement Messages. */
#ifdef CONFIG_NX_DISABLE_ICMPV6_ROUTER_ADVERTISEMENT_PROCESS
#define NX_DISABLE_ICMPV6_ROUTER_ADVERTISEMENT_PROCESS
#endif

/* Do not send IPv6 Router Solicitation Messages. */
#ifdef CONFIG_NX_DISABLE_ICMPV6_ROUTER_SOLICITATION
#define NX_DISABLE_ICMPV6_ROUTER_SOLICITATION
#endif

/* Define the max number of router solicitations a host sends until a router response
   is received.  If no response is received, the host concludes no router is present. */
#define NX_ICMPV6_MAX_RTR_SOLICITATIONS         CONFIG_NX_ICMPV6_MAX_RTR_SOLICITATIONS

/* Define the interval between which the host sends router solicitations in seconds. */
#define NX_ICMPV6_RTR_SOLICITATION_INTERVAL     CONFIG_NX_ICMPV6_RTR_SOLICITATION_INTERVAL

/* Define the maximum delay for the initial router solicitation in seconds. */
#define NX_ICMPV6_RTR_SOLICITATION_DELAY        CONFIG_NX_ICMPV6_RTR_SOLICITATION_DELAY

/* Do not send ICMPv4 Error Messages. */
#ifdef CONFIG_NX_DISABLE_ICMPV4_ERROR_MESSAGE
#define NX_DISABLE_ICMPV4_ERROR_MESSAGE
#endif

/* Do not send ICMPv6 Error Messages. */
#ifdef CONFIG_NX_DISABLE_ICMPV6_ERROR_MESSAGE
#define NX_DISABLE_ICMPV6_ERROR_MESSAGE
#endif

/* Disable the Duplicate Address Detection (DAD) protocol when configuring the host IP address. */
#ifdef CONFIG_NX_DISABLE_IPV6_DAD
#define NX_DISABLE_IPV6_DAD
#endif

/* If defined, application is able to control whether or not to perform IPv6 stateless
   address autoconfiguration with nxd_ipv6_stateless_address_autoconfig_enable() or
   nxd_ipv6_stateless_address_autoconfig_disable() service.  If defined, the system starts
   with IPv6 stateless address autoconfiguration enabled.  This feature is disabled by default. */
#ifdef CONFIG_NX_IPV6_STATELESS_AUTOCONFIG_CONTROL
#define NX_IPV6_STATELESS_AUTOCONFIG_CONTROL
#endif

/* If enabled, application is able to install a callback function to get notified
   when an interface IPv6 address is changed. By default this feature is disabled. */
#ifdef CONFIG_NX_ENABLE_IPV6_ADDRESS_CHANGE_NOTIFY
#define NX_ENABLE_IPV6_ADDRESS_CHANGE_NOTIFY
#endif

/* Defined, this option prevents NetX Duo from removing stale (old) cache table entries
   whose timeout has not expired so are otherwise still valid) to make room for new entries
   when the table is full.  Static and router entries are not purged.  */
#ifdef CONFIG_NX_DISABLE_IPV6_PURGE_UNUSED_CACHE_ENTRIES
#define NX_DISABLE_IPV6_PURGE_UNUSED_CACHE_ENTRIES
#endif

/* This define enables simple IPv6 multicast group join/leave function.  By default
   the IPv6 multicast join/leave function is not enabled. */
#ifdef CONFIG_NX_ENABLE_IPV6_MULTICAST
#define NX_ENABLE_IPV6_MULTICAST
#endif

/* Defined, Minimum Path MTU Discovery feature is enabled.  */
#ifdef CONFIG_NX_ENABLE_IPV6_PATH_MTU_DISCOVERY
#define NX_ENABLE_IPV6_PATH_MTU_DISCOVERY
#endif

/* Define wait interval in seconds to reset the path MTU for a destination
   table entry after decreasing it in response to a packet too big error message.
   RFC 1981 Section 5.4 states the minimum time to wait is
   5 minutes and recommends 10 minutes.
*/
#define NX_PATH_MTU_INCREASE_WAIT_INTERVAL               CONFIG_NX_PATH_MTU_INCREASE_WAIT_INTERVAL


/* Configuration options for Neighbor Discovery.  */
/* Define values used for Neighbor Discovery protocol.
   The default values are suggested by RFC2461, chapter 10. */

/* Define the maximum number of multicast Neighbor Solicitation packets
   NetX Duo sends for a packet destination needing physical mapping
   to the IP address. */
#define NX_MAX_MULTICAST_SOLICIT        CONFIG_NX_MAX_MULTICAST_SOLICIT

/* Define the maximum number of unicast Neighbor Solicitation packets
   NetX Duo sends for a cache entry whose reachable time has expired
   and gone "stale". */
#define NX_MAX_UNICAST_SOLICIT          CONFIG_NX_MAX_UNICAST_SOLICIT

/* Define the length of time, in seconds, that a Neighbor Cache table entry
   remains in the reachable state before it becomes state. */
#define NX_REACHABLE_TIME               CONFIG_NX_REACHABLE_TIME

/* Define the length of time, in milliseconds, between retransmitting
   Neighbor Solicitation (NS) packets. */
#define NX_RETRANS_TIMER                CONFIG_NX_RETRANS_TIMER

/* Define the length of time, in seconds, for a Neighbor Cache entry
   to remain in the Delay state.  This is the Delay first probe timer. */
#define NX_DELAY_FIRST_PROBE_TIME       CONFIG_NX_DELAY_FIRST_PROBE_TIME

/* This defines specifies the maximum number of packets that can be queued while waiting for a
   Neighbor Discovery to resolve an IPv6 address. The default value is 4.  */
#define NX_ND_MAX_QUEUE_DEPTH           CONFIG_NX_ND_MAX_QUEUE_DEPTH

/* Define the maximum ICMPv6 Duplicate Address Detect Transmit .  */
#define NX_IPV6_DAD_TRANSMITS           CONFIG_NX_IPV6_DAD_TRANSMITS

/* Define the number of neighbor cache entries. */
#define NX_IPV6_NEIGHBOR_CACHE_SIZE     CONFIG_NX_IPV6_NEIGHBOR_CACHE_SIZE

/* Define the size of the IPv6 destination table. */
#define NX_IPV6_DESTINATION_TABLE_SIZE  CONFIG_NX_IPV6_DESTINATION_TABLE_SIZE

/* Define the size of the IPv6 prefix table. */
#define NX_IPV6_PREFIX_LIST_TABLE_SIZE  CONFIG_NX_IPV6_PREFIX_LIST_TABLE_SIZE



/* Configuration options for IPSEC */

/* This define enables IPSEC in NetX Duo.  */
#ifdef CONFIG_NX_IPSEC_ENABLE
#define NX_IPSEC_ENABLE
#endif


/* Configuration options for NAT */

/* This define enables NAT process in NetX Duo.  */
#ifdef CONFIG_NX_NAT_ENABLE
#define NX_NAT_ENABLE
#endif


/* Configuration options for IGMP */

/* Defined, IGMP v2 support is disabled.  By default NetX Duo
   is built with IGMPv2 enabled .  By uncommenting this option,
   NetX Duo reverts back to IGMPv1 only. */
#ifdef CONFIG_NX_DISABLE_IGMPV2
#define NX_DISABLE_IGMPV2
#endif


/* Configuration options for ARP */

/* When defines, ARP reply is sent when address conflict occurs. */
#ifdef CONFIG_NX_ARP_DEFEND_BY_REPLY
#define NX_ARP_DEFEND_BY_REPLY
#endif

/* To use the ARP collision handler to check for invalid ARP messages
   matching existing entries in the table (man in the middle attack),
   enable this feature.  */
#ifdef CONFIG_NX_ENABLE_ARP_MAC_CHANGE_NOTIFICATION
#define  NX_ENABLE_ARP_MAC_CHANGE_NOTIFICATION
#endif

/* This define specifies the number of seconds ARP entries remain valid. The default value of 0 disables
   aging of ARP entries.  */
#define NX_ARP_EXPIRATION_RATE      CONFIG_NX_ARP_EXPIRATION_RATE

/* This define specifies the number of seconds between ARP retries. The default value is 10, which represents
   10 seconds.  */
#define NX_ARP_UPDATE_RATE          CONFIG_NX_ARP_UPDATE_RATE

/* This define specifies the maximum number of ARP retries made without an ARP response. The default
   value is 18.  */
#define NX_ARP_MAXIMUM_RETRIES      CONFIG_NX_ARP_MAXIMUM_RETRIES

/* This defines specifies the maximum number of packets that can be queued while waiting for an ARP
   response. The default value is 4.  */
#define NX_ARP_MAX_QUEUE_DEPTH      CONFIG_NX_ARP_MAX_QUEUE_DEPTH

/* Defined, this option disables entering ARP request information in the ARP cache.  */
#ifdef CONFIG_NX_DISABLE_ARP_AUTO_ENTRY
#define NX_DISABLE_ARP_AUTO_ENTRY
#endif

/* Define the ARP defend interval. The default value is 10 seconds.  */
#define NX_ARP_DEFEND_INTERVAL     CONFIG_NX_ARP_DEFEND_INTERVAL



/* Configuration options for TCP */

/* This define specifies how the number of system ticks (NX_IP_PERIODIC_RATE) is divided to calculate the
   timer rate for the TCP delayed ACK processing. The default value is 5, which represents 200ms.  */
#define NX_TCP_ACK_TIMER_RATE       CONFIG_NX_TCP_ACK_TIMER_RATE

/* This define specifies how the number of system ticks (NX_IP_PERIODIC_RATE) is divided to calculate the
   fast TCP timer rate. The fast TCP timer is used to drive various TCP timers, including the delayed ACK
   timer. The default value is 10, which represents 100ms.  */
#define NX_TCP_FAST_TIMER_RATE      CONFIG_NX_TCP_FAST_TIMER_RATE

/* This define specifies how the number of system ticks (NX_IP_PERIODIC_RATE) is divided to calculate the
   timer rate for the TCP transmit retry processing. The default value is 1, which represents 1 second.  */
#define NX_TCP_TRANSMIT_TIMER_RATE  CONFIG_NX_TCP_TRANSMIT_TIMER_RATE

/* This define specifies how many seconds of inactivity before the keepalive timer activates. The default
   value is 7200, which represents 2 hours.   */
#define NX_TCP_KEEPALIVE_INITIAL    CONFIG_NX_TCP_KEEPALIVE_INITIAL

/* This define specifies how many seconds between retries of the keepalive timer assuming the other side
   of the connection is not responding. The default value is 75, which represents 75 seconds between
   retries.  */
#define NX_TCP_KEEPALIVE_RETRY      CONFIG_NX_TCP_KEEPALIVE_RETRY

/* This define specifies the maximum packets that are out of order. The default value is 8.  */
#define NX_TCP_MAX_OUT_OF_ORDER_PACKETS CONFIG_NX_TCP_MAX_OUT_OF_ORDER_PACKETS

/* This define specifies the maximum number of TCP server listen requests. The default value is 10.  */
#define NX_MAX_LISTEN_REQUESTS      CONFIG_NX_MAX_LISTEN_REQUESTS

/* Defined, this option enables the optional TCP keepalive timer.  */
#ifdef CONFIG_NX_ENABLE_TCP_KEEPALIVE
#define NX_ENABLE_TCP_KEEPALIVE
#endif

/* Defined, this option enables the optional TCP immediate ACK response processing.  */
#ifdef CONFIG_NX_TCP_IMMEDIATE_ACK
#define NX_TCP_IMMEDIATE_ACK
#endif

/* This define specifies the number of TCP packets to receive before sending an ACK. */
/* The default value is 2: ack every 2 packets.                                      */
#define NX_TCP_ACK_EVERY_N_PACKETS  CONFIG_NX_TCP_ACK_EVERY_N_PACKETS

/* Automatically define NX_TCP_ACK_EVERY_N_PACKETS to 1 if NX_TCP_IMMEDIATE_ACK is defined.
   This is needed for backward compatibility. */
// #if (defined(NX_TCP_IMMEDIATE_ACK) && !defined(NX_TCP_ACK_EVERY_N_PACKETS))
// #define NX_TCP_ACK_EVERY_N_PACKETS 1
// #endif

/* This define specifies how many transmit retires are allowed before the connection is deemed broken.
   The default value is 10.  */
#define NX_TCP_MAXIMUM_RETRIES      CONFIG_NX_TCP_MAXIMUM_RETRIES

/* This define specifies the maximum depth of the TCP transmit queue before TCP send requests are
   suspended or rejected. The default value is 20, which means that a maximum of 20 packets can be in
   the transmit queue at any given time.  */
#define NX_TCP_MAXIMUM_TX_QUEUE     CONFIG_NX_TCP_MAXIMUM_TX_QUEUE

/* This define specifies how the retransmit timeout period changes between successive retries. If this
   value is 0, the initial retransmit timeout is the same as subsequent retransmit timeouts. If this
   value is 1, each successive retransmit is twice as long. The default value is 0.  */
#define NX_TCP_RETRY_SHIFT          CONFIG_NX_TCP_RETRY_SHIFT

/* This define specifies how many keepalive retries are allowed before the connection is deemed broken.
   The default value is 10.  */
#define NX_TCP_KEEPALIVE_RETRIES    CONFIG_NX_TCP_KEEPALIVE_RETRIES

/* Defined, this option enables the TCP window scaling feature. (RFC 1323). Default disabled. */
#ifdef CONFIG_NX_ENABLE_TCP_WINDOW_SCALING
#define NX_ENABLE_TCP_WINDOW_SCALING
#endif

/* Defined, this option disables the reset processing during disconnect when the timeout value is
   specified as NX_NO_WAIT.  */
#ifdef CONFIG_NX_DISABLE_RESET_DISCONNECT
#define NX_DISABLE_RESET_DISCONNECT
#endif

/* If defined, the incoming SYN packet (connection request) is checked for a minimum acceptable
   MSS for the host to accept the connection. The default minimum should be based on the host
   application packet pool payload, socket transmit queue depth and relevant application specific parameters. */
#ifdef CONFIG_NX_ENABLE_TCP_MSS_CHECK
#define NX_ENABLE_TCP_MSS_CHECK
#endif

#define NX_TCP_MSS_MINIMUM              CONFIG_NX_TCP_MSS_MINIMUM

/* If defined, NetX Duo has a notify callback for the transmit TCP socket queue decreased from
   the maximum queue depth.  */
#ifdef CONFIG_NX_ENABLE_TCP_QUEUE_DEPTH_UPDATE_NOTIFY
#define NX_ENABLE_TCP_QUEUE_DEPTH_UPDATE_NOTIFY
#endif

/* Defined, feature of low watermark is enabled. */
#ifdef CONFIG_NX_ENABLE_LOW_WATERMARK
#define NX_ENABLE_LOW_WATERMARK
#endif

/* Define the maximum receive queue for TCP socket. */
#ifdef NX_ENABLE_LOW_WATERMARK
#define NX_TCP_MAXIMUM_RX_QUEUE    20
#endif


/* Configuration options for fragmentation */

/* Defined, this option disables both IPv4 and IPv6 fragmentation and reassembly logic.  */
#ifdef CONFIG_NX_DISABLE_FRAGMENTATION
#define NX_DISABLE_FRAGMENTATION
#endif

/* Defined, this option process IP fragmentation immediately.  */
#ifdef CONFIG_NX_FRAGMENT_IMMEDIATE_ASSEMBLY
#define NX_FRAGMENT_IMMEDIATE_ASSEMBLY
#endif

/* This define specifies the maximum time of IP reassembly.  The default value is 60.
   By default this option is not defined.  */
// #define NX_IP_MAX_REASSEMBLY_TIME   60

/* This define specifies the maximum time of IPv4 reassembly.  The default value is 15.
   Note that if NX_IP_MAX_REASSEMBLY_TIME is defined, this option is automatically defined as 60.
   By default this option is not defined.  */
#ifndef NX_IP_MAX_REASSEMBLY_TIME
#define NX_IPV4_MAX_REASSEMBLY_TIME CONFIG_NX_IPV4_MAX_REASSEMBLY_TIME
#endif

/* This define specifies the maximum time of IPv6 reassembly.  The default value is 60.
   Note that if NX_IP_MAX_REASSEMBLY_TIME is defined, this option is automatically defined as 60.
   By default this option is not defined.  */
#ifndef NX_IP_MAX_REASSEMBLY_TIME
#define NX_IPV6_MAX_REASSEMBLY_TIME CONFIG_NX_IPV6_MAX_REASSEMBLY_TIME
#endif

/* Configuration options for checksum */

/* Defined, this option disables checksum logic on received ICMPv4 packets.
   Note that if NX_DISABLE_ICMP_RX_CHECKSUM is defined, this option is
   automatically defined. By default this option is not defined.*/
#ifdef CONFIG_NX_DISABLE_ICMPV4_RX_CHECKSUM
#define NX_DISABLE_ICMPV4_RX_CHECKSUM
#endif

/* Defined, this option disables checksum logic on received ICMPv6 packets.
   Note that if NX_DISABLE_ICMP_RX_CHECKSUM is defined, this option is
   automatically defined. By default this option is not defined.*/
#ifdef CONFIG_NX_DISABLE_ICMPV6_RX_CHECKSUM
#define NX_DISABLE_ICMPV6_RX_CHECKSUM
#endif

/* Defined, this option disables checksum logic on received ICMPv4 or ICMPv6 packets.
   Note that if NX_DISABLE_ICMP_RX_CHECKSUM is defined, NX_DISABLE_ICMPV4_RX_CHECKSUM
   and NX_DISABLE_ICMPV6_RX_CHECKSUM are automatically defined. */
#ifdef CONFIG_NX_DISABLE_ICMP_RX_CHECKSUM
#define NX_DISABLE_ICMP_RX_CHECKSUM
#endif

/* Defined, this option disables checksum logic on transmitted ICMPv4 packets.
   Note that if NX_DISABLE_ICMP_TX_CHECKSUM is defined, this option is
   automatically defined. By default this option is not defined.*/
#ifdef CONFIG_NX_DISABLE_ICMPV4_TX_CHECKSUM
#define NX_DISABLE_ICMPV4_TX_CHECKSUM
#endif

/* Defined, this option disables checksum logic on transmitted ICMPv6 packets.
   Note that if NX_DISABLE_ICMP_TX_CHECKSUM is defined, this option is
   automatically defined. By default this option is not defined.*/
#ifdef CONFIG_NX_DISABLE_ICMPV6_TX_CHECKSUM
#define NX_DISABLE_ICMPV6_TX_CHECKSUM
#endif

/* Defined, this option disables checksum logic on transmitted ICMPv4 or ICMPv6 packets.
   Note that if NX_DISABLE_ICMP_TX_CHECKSUM is defined, NX_DISABLE_ICMPV4_TX_CHECKSUM
   and NX_DISABLE_ICMPV6_TX_CHECKSUM are automatically defined. */
#ifdef CONFIG_NX_DISABLE_ICMP_TX_CHECKSUM
#define NX_DISABLE_ICMP_TX_CHECKSUM
#endif

/* Defined, this option disables checksum logic on received IP packets. This is useful if the link-layer
   has reliable checksum or CRC logic.  */
#ifdef CONFIG_NX_DISABLE_IP_RX_CHECKSUM
#define NX_DISABLE_IP_RX_CHECKSUM
#endif

/* Defined, this option disables checksum logic on transmitted IP packets.  */
#ifdef CONFIG_NX_DISABLE_IP_TX_CHECKSUM
#define NX_DISABLE_IP_TX_CHECKSUM
#endif

/* Defined, this option disables checksum logic on received TCP packets.  */
#ifdef CONFIG_NX_DISABLE_TCP_RX_CHECKSUM
#define NX_DISABLE_TCP_RX_CHECKSUM
#endif

/* Defined, this option disables checksum logic on transmitted TCP packets.  */
#ifdef CONFIG_NX_DISABLE_TCP_TX_CHECKSUM
#define NX_DISABLE_TCP_TX_CHECKSUM
#endif

/* Defined, this option disables checksum logic on received UDP packets.  */

#ifdef CONFIG_NX_DISABLE_UDP_RX_CHECKSUM
#define NX_DISABLE_UDP_RX_CHECKSUM
#endif

/* Defined, this option disables checksum logic on transmitted UDP packets.  Note that
   IPV6 requires the UDP checksum computed for outgoing packets.  If this option is
   defined, the IPv6 NetX Duo host must ensure the UDP checksum is computed elsewhere
   before the packet is transmitted. */
#ifdef CONFIG_NX_DISABLE_UDP_TX_CHECKSUM
#define NX_DISABLE_UDP_TX_CHECKSUM
#endif


/* Configuration options for statistics.  */

/* Defined, ARP information gathering is disabled.  */
#ifdef CONFIG_NX_DISABLE_ARP_INFO
#define NX_DISABLE_ARP_INFO
#endif

/* Defined, IP information gathering is disabled.  */
#ifdef CONFIG_NX_DISABLE_IP_INFO
#define NX_DISABLE_IP_INFO
#endif

/* Defined, ICMP information gathering is disabled.  */
#ifdef CONFIG_NX_DISABLE_ICMP_INFO
#define NX_DISABLE_ICMP_INFO
#endif

/* Defined, IGMP information gathering is disabled.  */
#ifdef CONFIG_NX_DISABLE_IGMP_INFO
#define NX_DISABLE_IGMP_INFO
#endif

/* Defined, packet information gathering is disabled.  */
#ifdef CONFIG_NX_DISABLE_PACKET_INFO
#define NX_DISABLE_PACKET_INFO
#endif

/* Defined, RARP information gathering is disabled.  */
#ifdef CONFIG_NX_DISABLE_RARP_INFO
#define NX_DISABLE_RARP_INFO
#endif

/* Defined, TCP information gathering is disabled.  */
#ifdef CONFIG_NX_DISABLE_TCP_INFO
#define NX_DISABLE_TCP_INFO
#endif

/* Defined, UDP information gathering is disabled.  */
#ifdef CONFIG_NX_DISABLE_UDP_INFO
#define NX_DISABLE_UDP_INFO
#endif


/* Configuration options for Packet Pool */

/* This define specifies the size of the physical packet header. The default value is 16 (based on
   a typical 16-byte Ethernet header).  */
#define NX_PHYSICAL_HEADER          CONFIG_NX_PHYSICAL_HEADER

/* This define specifies the size of the physical packet trailer and is typically used to reserve storage
   for things like Ethernet CRCs, etc.  */
#define NX_PHYSICAL_TRAILER         CONFIG_NX_PHYSICAL_TRAILER

/* Defined, this option disables the addition size checking on received packets.  */
#ifdef CONFIG_NX_DISABLE_RX_SIZE_CHECKING
#define NX_DISABLE_RX_SIZE_CHECKING
#endif

/* Defined, packet debug infromation is enabled.  */
#ifdef CONFIG_NX_ENABLE_PACKET_DEBUG_INFO
#define NX_ENABLE_PACKET_DEBUG_INFO
#endif

/* Defined, NX_PACKET structure is padded for alignment purpose. The default is no padding. */
#ifdef CONFIG_NX_PACKET_HEADER_PAD
#define NX_PACKET_HEADER_PAD
#endif
#define NX_PACKET_HEADER_PAD_SIZE   CONFIG_NX_PACKET_HEADER_PAD_SIZE

/* Defined, packet header and payload are aligned automatically by the value. The default value is sizeof(ULONG). */
/*
#define NX_PACKET_ALIGNMENT sizeof(ULONG)
*/

/* If defined, the packet chain feature is removed. */
#ifdef CONFIG_NX_DISABLE_PACKET_CHAIN
#define NX_DISABLE_PACKET_CHAIN
#endif

/* Defined, the IP instance manages two packet pools. */
#ifdef CONFIG_NX_ENABLE_DUAL_PACKET_POOL
#define NX_ENABLE_DUAL_PACKET_POOL
#endif

/* Configuration options for Others */

/* Defined, this option bypasses the basic NetX error checking. This define is typically used
   after the application is fully debugged.  */
#ifdef CONFIG_NX_DISABLE_ERROR_CHECKING
#define NX_DISABLE_ERROR_CHECKING
#endif

/* Defined, this option enables deferred driver packet handling. This allows the driver to place a raw
   packet on the IP instance and have the driver's real processing routine called from the NetX internal
   IP helper thread.  */
#ifdef CONFIG_NX_DRIVER_DEFERRED_PROCESSING
#define NX_DRIVER_DEFERRED_PROCESSING
#endif

/* Defined, the source address of incoming packet is checked. The default is disabled. */
#ifdef CONFIG_NX_ENABLE_SOURCE_ADDRESS_CHECK
#define NX_ENABLE_SOURCE_ADDRESS_CHECK
#endif

/* Defined, the extended notify support is enabled.  This feature adds additional callback/notify services
   to NetX Duo API for notifying the application of socket events, such as TCP connection and disconnect
   completion.  These extended notify functions are mainly used by the BSD wrapper. The default is this
   feature is disabled.  */
#ifdef CONFIG_NX_ENABLE_EXTENDED_NOTIFY_SUPPORT
#define NX_ENABLE_EXTENDED_NOTIFY_SUPPORT
#endif

/* Defined, ASSERT is disabled. The default is enabled. */
#ifdef CONFIG_NX_DISABLE_ASSERT
#define NX_DISABLE_ASSERT
#endif

/* Define the process when assert fails. */
/*
#define NX_ASSERT_FAIL while (1) tx_thread_sleep(NX_WAIT_FOREVER);
*/

/* Defined, the IPv4 feature is disabled. */
#ifdef CONFIG_NX_DISABLE_IPV4
#define NX_DISABLE_IPV4
#endif

/* Defined, the destination address of ICMP packet is checked. The default is disabled.
   An ICMP Echo Request destined to an IP broadcast or IP multicast address will be silently discarded.
*/
#ifdef CONFIG_NX_ENABLE_ICMP_ADDRESS_CHECK
#define NX_ENABLE_ICMP_ADDRESS_CHECK
#endif

/* Define the max string length. The default value is 1024.  */
#define NX_MAX_STRING_LENGTH                                CONFIG_NX_MAX_STRING_LENGTH

/* Defined, the TCP/IP offload feature is enabled.
   NX_ENABLE_INTERFACE_CAPABILITY must be defined to enable this feature.  */
#ifdef CONFIG_NX_ENABLE_TCPIP_OFFLOAD
#define NX_ENABLE_TCPIP_OFFLOAD
#endif

/* Defined, the VLAN feature is enabled.
   Note: Require driver support to use APIs from this file.
         A quick check in driver is to search for
         NX_LINK_RAW_PACKET_SEND. VLAN APIs are not supported if not found. */
#ifdef CONFIG_NX_ENABLE_VLAN
#define NX_ENABLE_VLAN
#endif

#endif /* NX_USER_CONFIG_H */
