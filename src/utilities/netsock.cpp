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
// netsock.cpp - Implements a network socket
//==========================================================================================================
#include <unistd.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/tcp.h>
#include "netsock.h"
#include "netutil.h"

//==========================================================================================================
// Constructor
//==========================================================================================================
NetSock::NetSock()
{
    // Ensure that we start out with an invalid socket descriptor
    m_sd = -1;

    // This socket has not yet been created
    m_is_created = false;
}
//==========================================================================================================


//==========================================================================================================
// close() - Closes the socket
//==========================================================================================================
void NetSock::close()
{
    if (m_sd >= 0) ::close(m_sd);
    m_sd = -1;
}
//==========================================================================================================


//==========================================================================================================
// connect() - Creates the socket and connects it to a server
//==========================================================================================================
bool NetSock::connect(std::string server, int port, int family, std::string interface)
{
    addrinfo_t info;

    // The socket is not yet created
    m_is_created = false;

    // Close this socket if it happens to be open
    close();

    // If we can't find any information about that server, tell the caller
    if (!NetUtil::get_server_addrinfo(SOCK_STREAM, server, port, family, &info)) 
    {
        m_error_str = "no such server: "+server;
        m_error     = NO_SUCH_SERVER;
        return false;
    }

    // Create the socket
    m_sd = socket(info.family, info.socktype, info.protocol);


    // If the socket() call fails, complain
    if (m_sd < 0) throw std::runtime_error("failure on socket()");

    // Set the scope ID for link-local IPv6 addresses
    if (family == AF_INET6)
    {
        // Cast the generic sockaddr_storage to sockaddr_in6
        struct sockaddr_in6* addr6 = reinterpret_cast<struct sockaddr_in6*>(&info.addr);

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

    // Attempt to connect to the server
    if (::connect(m_sd, info, info.addrlen) < 0)
    {
        m_error_str = "can't connect to "+server;
        m_error     = CANT_CONNECT;
        close();
        return false;
    }

    // If we get here, we have a connected socket
    return true;
}
//==========================================================================================================



//==========================================================================================================
// create_server() - Creates a server socket
//
// Passed:  port      = The TCP port number to create the socket on
//          bind_to   = The IP address of the network card to bind to (optional)
//          family    = AF_UNSPEC, AF_INET, or AF_INET6 (optional)
//          interface = network interface name such as eth0, wlan0, ens33, etc. (optional)
//
// Returns: 'true'  = The server socket was created succesfully.
//          'false' = The call to "bind" failed, typically because another socket is already
//                    bound to that port.
//==========================================================================================================
bool NetSock::create_server(int port, std::string bind_to, int family, std::string interface)
{
    // The socket is not yet created
    m_is_created = false;

    // Close this socket if it happens to be open
    close();

    // Fetch information about the server socket we're going to create
    addrinfo_t server = NetUtil::get_local_addrinfo(SOCK_STREAM, port, bind_to, family);

    // If that call to "get_local_info" failed, we throw an exception
    if (server.family == 0) throw std::runtime_error("failure on getaddrinfo()");

    // Create the socket
    m_sd = socket(server.family, server.socktype, server.protocol);

    // If the socket() call fails, complain
    if (m_sd < 0) throw std::runtime_error("failure on socket()");

    // Allow us to re-use this port if it's still in TIME_WAIT
    int optval = 1;
    setsockopt(m_sd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval);

    // Set the scope ID for link-local IPv6 addresses
    if (family == AF_INET6)
    {
        // Cast the generic sockaddr_storage to sockaddr_in6
        struct sockaddr_in6* addr6 = reinterpret_cast<struct sockaddr_in6*>(&server.addr);

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

    // Bind it to the port we passed in to getaddrinfo():
    if (bind(m_sd, server, server.addrlen) < 0)
    {
        m_error_str = "failure on bind()";
        m_error     = BIND_FAILED;
        close();
        return false;
    }

    // This socket has been created
    m_is_created = true;

    // Tell the caller that all is well
    return true;
}
//==========================================================================================================


//==========================================================================================================
// listen() - Starts listening for connections on a server socket.   
//
// Throws runtime_error if something goes wrong.
//==========================================================================================================
void NetSock::listen(int concurrent_connections)
{
    // If the socket isn't created yet, don't even try 
    if (!m_is_created) throw std::runtime_error("listen called on non existent socket");

    // Start listening for an incoming connection
    int status = ::listen(m_sd, concurrent_connections);

    // If listen() barfed on us, tell the caller
    if (status < 0) throw std::runtime_error("failure on listen()");
}
//==========================================================================================================


//==========================================================================================================
// accept() - Waits for someone to connect to a server socket
// 
// Passed: timeout_ms = # of milliseconds to wait for incoming connection.  -1 = Wait forever
//         dest_sock  = The optional address of a destination NetSock object to attach the connection to
//
// Returns: true if a client connected to our server, otherwise false
//==========================================================================================================
bool NetSock::accept(int timeout_ms, NetSock* dest_sock)
{
    // If the socket isn't created yet, don't even try 
    if (!m_is_created) throw std::runtime_error("accept called on non existent socket");

    // Wait for a client to connect to our socket
    if (!wait_for_data(timeout_ms)) return false;

    // Accept the connection
    int new_sd = ::accept(m_sd, NULL, 0);

    // If accept() failed, tell the caller
    if (new_sd < 0) throw std::runtime_error("failure on accept()");

    // If the caller passed us a socket object to clone ourselves into...
    if (dest_sock)
    {
        *dest_sock = *this;
        dest_sock->m_sd = new_sd;
    }

    // Otherwise, the new socket-descriptor is the one we'll read and write on
    else
    {
        ::close(m_sd);
        m_sd = new_sd;        
    }

    // Tell the caller that all is well
    return true;
}
//==========================================================================================================



//==========================================================================================================
// listen_and_accept() - Convience method for waiting for a single incoming connection
// 
// Passed: timeout_ms = # of milliseconds to wait for incoming connection.  -1 = Wait forever
//
// Returns: true if a client connected to our server, otherwise false
//==========================================================================================================
bool NetSock::listen_and_accept(int timeout_ms)
{
    listen(0);
    return accept(timeout_ms);
}
//==========================================================================================================


//==========================================================================================================
// wait_for_data() - Waits for the specified amount of time for data to be available for reading
//
// Passed: timeout_ms = timeout in milliseconds.  -1 = Wait forever
//
// Returns: true if data is available for reading, else false
//==========================================================================================================
bool NetSock::wait_for_data(int timeout_ms)
{
    return NetUtil::wait_for_data(timeout_ms, m_sd);
}
//==========================================================================================================


//==========================================================================================================
// bytes_available() - Returns the number of bytes available for reading
//==========================================================================================================
int NetSock::bytes_available()
{
    int count = 0;
    ioctl(m_sd, FIONREAD, &count);
    return count;
}
//==========================================================================================================



//==========================================================================================================
// receive() - Receives data from the socket
//
// Passed:  buffer = Pointer to the place to store the received data
//          length = The number of bytes to read in
//          peek   = If true, the bytes will be returned but not removed from the buffer
//
// Returns: The number of bytes that were read
//             -- or -- -1 = An error occured
//             -- or --  0 = The socket was closed (possibly by the other side)
//==========================================================================================================
int NetSock::receive(void* buffer, int length, bool peek)
{
    // Don't attempt to recv zero bytes
    if (length == 0) return 0;

    // This is the set of flags that we're going to pass to recv
    int flags = peek ?  MSG_PEEK : 0;

    // Get a byte-pointer to the caller's buffer
    unsigned char* ptr = (unsigned char*)buffer;

    // Keep track of how many bytes we have left to read
    int bytes_remaining = length;

    // Loop until there are no more bytes to read...
    while (bytes_remaining)
    {
        // Fetch some bytes from the socket
        int bytes_rcvd = recv(m_sd, ptr, bytes_remaining, flags);

        // If the read failed, tell the caller
        if (bytes_rcvd < 0) return -1;

        // If the socket is closed, tell the caller
        if (bytes_rcvd == 0) return 0;

        // Adjust our pointer and the number of bytes remaining to be read
        ptr             += bytes_rcvd;
        bytes_remaining -= bytes_rcvd;
    }

    // Tell the caller that we received all of the data they wanted
    return length;
}
//==========================================================================================================


//==========================================================================================================
// send() - Sends a buffer to the other side of a connected socket
//
// Returns either : -1 = An error occured
//                  Anything else = the number of bytes actually sent.  The entire string will always be
//                  sent unless the socket was closed by the other side
//==========================================================================================================
int NetSock::send(const void* buffer, int length)
{
    // Don't attempt to send zero bytes
    if (length == 0) return 0;

    // If the socket descriptor isn't open, don't try to send anything
    if (m_sd < 0) return -1;

    // Get a byte pointer to the caller's buffer
    unsigned char* ptr = (unsigned char*)buffer;

    // Keep track of how many bytes remain to be sent
    int bytes_remaining = length;

    // Loop until there are no more bytes to send...
    while (bytes_remaining)
    {
        // Attempt to send all of the bytes
        int sent = ::send(m_sd, ptr, bytes_remaining, MSG_NOSIGNAL);

        // If an error occured, tell the caller
        if (sent < 0) return -1;

        // If the socket is closed, we're done
        if (sent == 0) break;

        // Adjust the pointer and the count of bytes remaining to be sent
        ptr             += sent;
        bytes_remaining -= sent;
    }

    // Tell the caller how many bytes we sent
    return (length - bytes_remaining);
}
//==========================================================================================================


//==========================================================================================================
// copy_object() - Copies another object of the same type
//==========================================================================================================
void NetSock::copy_object(const NetSock& rhs)
{
    m_sd         = rhs.m_sd;
    m_is_created = rhs.m_is_created;
    m_error      = rhs.m_error;
    m_error_str  = rhs.m_error_str;
}
//==========================================================================================================
