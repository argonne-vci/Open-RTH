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
// netutil.cpp - Implements some common networking utility functions
//==========================================================================================================
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <string>
#include "netutil.h"

//==========================================================================================================
// get_local_addrinfo() - Returns an addrinfo structure for the local machine
//
// Passed:  type    = SOCK_STREAM or SOCK_DGRAM
//          port    = The TCP port number we want to create a socket on
//          bind_to = The IP address of the network card to bind to (optional)
//          family  = AF_UNSPEC, AF_INET, or AF_INET6
//
// Returns: an addrinfo_t structure.
//          if addrinfo.family == 0, the call failed
//==========================================================================================================
addrinfo_t NetUtil::get_local_addrinfo(int type, int port, std::string bind_to, int family)
{
    char ascii_port[20];
    struct addrinfo hints, *p_res;
    addrinfo_t result;

    // If we fail, our entire return structure will be zero
    memset(&result, 0, sizeof(result));

    // Get a pointer to the IP address we want to bind to
    const char* bind_addr = bind_to.empty() ? NULL : bind_to.c_str();

    // Get an ASCII version of the port number
    sprintf(ascii_port, "%i", port);

    // Tell getaddrinfo about the socket family and type
    memset(&hints, 0, sizeof hints);
    hints.ai_family   = family;  
    hints.ai_socktype = type;
    
    // Handle the case where we're not binding to a specific IP address
    if (bind_addr == NULL) hints.ai_flags = AI_PASSIVE;  

    // Fetch important information about the socket we're going to create
    if (getaddrinfo(bind_addr, ascii_port, &hints, &p_res) != 0) return result;

    // If we didn't get a result from getaddrinfo, something's wrong
    if (p_res == NULL) return result;

    // Save a copy of the results
    result = *p_res;

    // Free the memory that was allocated by getaddrinfo
    freeaddrinfo(p_res);

    // And hand the result to the caller
    return result;
}
//==========================================================================================================



//==========================================================================================================
// get_server_addrinfo() - Returns connection information for a remote server
//==========================================================================================================
bool NetUtil::get_server_addrinfo(int type, std::string server, int port, int family, addrinfo_t* p_result)
{
    char ascii_port[20];
    struct addrinfo hints, *p_res;

    // If we fail, our entire return structure will be zero
    memset(p_result, 0, sizeof(addrinfo));

    // Get an ASCII version of the port number
    sprintf(ascii_port, "%i", port);

    // We're going to build an IPv4/IPv6 TCP socket
    memset(&hints, 0, sizeof hints);
    hints.ai_family   = family;
    hints.ai_socktype = type;
    
    // Get information about this server.  If we can't, it doesn't exist
    if (getaddrinfo(server.c_str(), ascii_port, &hints, &p_res) != 0) return false;

    // If we didn't get a result from getaddrinfo, something's wrong
    if (p_res == NULL) return false;

    // Save a copy of the results
    *p_result = *p_res;

    // Free the memory that was allocated by getaddrinfo
    freeaddrinfo(p_res);

    // Tell the caller that we have information about the server he wants to connect o
    return true;
}
//==========================================================================================================



//==========================================================================================================
// ip_to_string() - Converts an IP address to a string
//==========================================================================================================
std::string NetUtil::ip_to_string(sockaddr* addr)
{
    // This is the field that will get returned
    char ip_address[64] = {0};

    // Figure out how long the socket address structure is for this family
    socklen_t socklen = (addr->sa_family == AF_INET) ? 
              sizeof(struct sockaddr_in) :
              sizeof(struct sockaddr_in6);

    // Fetch the ASCII IP address for this entry
    int rc = getnameinfo(addr, socklen, ip_address, sizeof(ip_address), NULL, 0, NI_NUMERICHOST);

    // On the off chance that the call to getnameinfo() fails, tell the caller
    if (rc != 0) return "";

    // Is there a '%' in this IP address?
    char* p = strchr(ip_address, '%');
        
    // If so, truncate the IP address at that '%' character
    if (p) *p = 0;

    // Hand the resulting IP address to the caller
    return ip_address;
}
//==========================================================================================================


//==========================================================================================================
// wait_for_data() - Waits for the specified amount of time for data to be available for reading
//
// Passed:  timeout_ms = timeout in milliseconds.  -1 = Wait forever
//
// Returns: a bitmap of which descriptors are available for reading
//==========================================================================================================
int NetUtil::wait_for_data(int timeout_ms, int fd1, int fd2, int fd3, int fd4)
{
    int    i, fd;
    fd_set rfds;
    timeval timeout;

    // Put them into an array
    int fd_list[] = {fd1, fd2, fd3, fd4};

    // Find out how many items are in the array
    const int ARRAY_COUNT = sizeof(fd_list) / sizeof(fd_list[0]);

    // We don't know what the highest file descriptor is yet
    int highest_fd = -1;

    // Clear our file descriptor set
    FD_ZERO(&rfds);

    // Loop through each file descriptor that was provided by the caller...
    for (i=0; i<ARRAY_COUNT; ++i)
    {
        // Fetch this file descriptor
        fd = fd_list[i];

        // Skip any invalid file descriptor
        if (fd < 0) continue;   

        // Include this file descriptor in our fd_set
        FD_SET(fd, &rfds);

        // Keep track of the highest file descriptor we have
        if (fd > highest_fd) highest_fd = fd;
    }

    // Assume for the moment that we are going to wait forever
    timeval* p_timeout = NULL;

    // If the caller wants us to wait for a finite amount of time...
    if (timeout_ms != -1)
    {
        // Convert milliseconds to microseconds
        int usecs = timeout_ms * 1000;

        // Determine the timeout in seconds and microseconds
        timeout.tv_sec  = usecs / 1000000;
        timeout.tv_usec = usecs % 1000000;

        // Point to the timeout structure we just initialized
        p_timeout = &timeout;
    }

    // Wait for one of the descriptors to become available for reading
    if (select(highest_fd+1, &rfds, NULL, NULL, p_timeout) < 1) return 0;

    // This is going to be a bitmap of which descriptors are readable
    int result = 0;

    // Loop through each possible descriptor...
    for (i=0; i<ARRAY_COUNT; ++i)
    {
        // Fetch this file descriptor
        fd = fd_list[i];

        // Skip any invalid file descriptor
        if (fd < 0) continue;   

        // If this descriptor is readable, set the appropriate bit in the result
        if (FD_ISSET(fd, &rfds)) result |= (1 << i);
    }

    // Hand the caller a bitmap of which of his descriptors are readable
    return result;
}
//==========================================================================================================


//==========================================================================================================
// from_ai() - Fill in a addrinfo_t structure from a 'struct addrinfo'
//==========================================================================================================
void addrinfo_t::from_ai(addrinfo& ai)
{
    // Save the IP address, port, family, etc
    addr = *(sockaddr_storage*)ai.ai_addr;

    // Save the length of m_addr
    addrlen = ai.ai_addrlen;

    // Save the address family (AF_INET or AF_INET6)
    family = ai.ai_family;
    
    // Save the socket type (SOCK_DGRAM or SOCK_STREAM)
    socktype = ai.ai_socktype;

    // Save the protocol
    protocol = ai.ai_protocol;
}
//==========================================================================================================