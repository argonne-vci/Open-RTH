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
// UDPSock.h - Defines a class for managing UDP sockets
//==========================================================================================================
#pragma once
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string>
#include "netutil.h"

//==========================================================================================================
// UDPSock() - UDP socket for sending or receiving UDP datagrams
//==========================================================================================================
class UDPSock
{
public:

    // Constructor, marks the socket as closed
    UDPSock() {m_sd = -1;}
    
    // Destructor - Closes the socket
    ~UDPSock() {close();}

    // Create a socket that we will send UDP packets on.
    bool    create_sender(int dst_port, std::string dest, int family = AF_INET, std::string interface = "", int src_port = 0, int timeout_ms = -1);

    // Create a socket that we will use to receive UDP packets
    bool    create_server(int port, std::string bind_to = "", int family = AF_UNSPEC, int timeout_ms = -1);

    // Closes the socket
    void    close();

    // Call this to send a message
    void    send(const void* msg, int length);

    // Call this to wait for a UDP packet to arrive
    int     receive(void* buffer, int buffer_length, std::string* p_peer_ip = NULL, int* p_source_port = 0);

    // Returns the socket descriptor of this socket
    int     get_sd() {return m_sd;}

protected:

    // The file descriptor
    int        m_sd;

    // The address IP address/port/etc of the UDP target
    addrinfo_t m_target;
};
//==========================================================================================================
