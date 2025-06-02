/* 
 * Copyright © 2025, UChicago Argonne, LLC
 * All Rights Reserved
 * Software Name: Remote Test Harness
 * By: Argonne National Laboratory
 * 
 * GNU GENERAL PUBLIC LICENSE
 * Version 3, 29 June 2007
 * Copyright © 2007 Free Software Foundation, Inc. <https://fsf.org/>
 * Everyone is permitted to copy and distribute verbatim copies of this license document, but changing it is not allowed.
 * 
 * See the LICENSE file for the full license text.
 */


//==========================================================================================================
// udpsock.cpp - Implements a class that manages UDP sockets
//==========================================================================================================
#include <unistd.h>
#include <net/if.h>
#include "udpsock.h"
#include "netutil.h"

//==========================================================================================================
// create_sender() - Create a socket useful for sending UDP packets
//==========================================================================================================
bool UDPSock::create_sender(int dst_port, std::string server, int family, std::string interface, int src_port, int timeout_ms)
{
    // If the socket is open, close it
    close();

    // Fetch information about the server we're trying to connect to
    if (!NetUtil::get_server_addrinfo(SOCK_DGRAM, server, dst_port, family, &m_target)) return false;

    // Create the socket
    m_sd = socket(m_target.family, m_target.socktype, m_target.protocol);

    // If that failed, tell the caller
    if (m_sd < 0) return false;

    // Set the scope ID for link-local IPv6 addresses
    if (family == AF_INET6)
    {
        // Cast the generic sockaddr_storage to sockaddr_in6
        struct sockaddr_in6* addr6 = reinterpret_cast<struct sockaddr_in6*>(&m_target.addr);

        // Check if IPv6 address is a link-local address
        if (IN6_IS_ADDR_LINKLOCAL(&addr6->sin6_addr))
        {
            // Set the scope ID using the interface name (eg: eth0, eth1, etc.)
            addr6->sin6_scope_id = if_nametoindex(interface.c_str());

            // If that failed, tell the caller
            if (addr6->sin6_scope_id == 0)
            {
                close();
                return false;
            }
        }
    }

    // If the caller specified a source port...
    if (src_port)
    {
        // Fetch information about the local machine
        addrinfo_t info = NetUtil::get_local_addrinfo(SOCK_DGRAM, src_port, "", family);

        // Bind the socket to the specified source port
        if (bind(m_sd, info, info.addrlen) < 0) return false;
    }

    // Apply the user-specified timeout, if valid
    if (timeout_ms >= 0)
    {
        struct timeval timeout;
        timeout.tv_sec = timeout_ms / 1000;
        timeout.tv_usec = (timeout_ms % 1000) * 1000;

        if (setsockopt(m_sd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0)
        {
            close();
            return false;
        }
    }

    // Tell the caller that all is well
    return true;
}
//==========================================================================================================


//==========================================================================================================
// create_server() - Creates a socket for listening on a UDP port
//==========================================================================================================
bool UDPSock::create_server(int port, std::string bind_to, int family, int timeout_ms)
{
    // If the socket is open, close it
    close();

    // Fetch information about the local machine
    addrinfo_t info = NetUtil::get_local_addrinfo(SOCK_DGRAM, port, bind_to, family);

    // Create the socket
    m_sd = socket(info.family, info.socktype, info.protocol);

    // If that failed, tell the caller
    if (m_sd < 0) return false;

    // Bind the socket to the specified port
    if (bind(m_sd, info, info.addrlen) < 0) return false;

    // Apply the user-specified timeout, if valid
    if (timeout_ms >= 0)
    {
        struct timeval timeout;
        timeout.tv_sec = timeout_ms / 1000;
        timeout.tv_usec = (timeout_ms % 1000) * 1000;

        if (setsockopt(m_sd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0)
        {
            close();
            return false;
        }
    }

    // Tell the caller that all is well
    return true;
}
//==========================================================================================================



//==========================================================================================================
// send() - Call this to transmit data on a "Sender" socket
//==========================================================================================================
void UDPSock::send(const void* msg, int length)
{
    sendto(m_sd, msg, length, 0, m_target, m_target.addrlen);
}
//==========================================================================================================



//==========================================================================================================
// receive() - Call this to wait for a packet on a server socket
//
// Returns: The number of bytes in the received message
//==========================================================================================================
int UDPSock::receive(void* buffer, int buf_size, std::string* p_source, int* p_source_port)
{
    sockaddr_storage peer;
    
    // Get a sockaddr* to the peer address
    // sockaddr* p_peer = (sockaddr*)&peer;
    sockaddr_in6 p_peer;

    // We need this for the call to ::recvfrom
    socklen_t addrlen = sizeof(peer);

    // Wait for a UDP message to arrive, and stuff it into the caller's buffer
    int byte_count = recvfrom(m_sd, buffer, buf_size, 0, (struct sockaddr*)&p_peer, &addrlen);

    // If there is room in the caller's buffer, as a convenience, put a nul-byte after the message
    if (byte_count < buf_size) ((char*)(buffer))[byte_count] = 0;

    // If the caller wants to know who sent the message...
    if (p_source) *p_source = NetUtil::ip_to_string((struct sockaddr*)&p_peer);

    // If the caller wants to know the client's port number
    if (p_source_port) *p_source_port = ntohs(p_peer.sin6_port);

    // Return the length of the message that was just fetched
    return byte_count;
}
//==========================================================================================================


//==========================================================================================================
// close() - closes the socket
//==========================================================================================================
void UDPSock::close()
{
    // If the socket is open, close it
    if (m_sd != -1) ::close(m_sd);

    // And mark the socket as closed
    m_sd = -1;
}
//==========================================================================================================
