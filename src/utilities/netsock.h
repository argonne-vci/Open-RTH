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
// netsock.h - Defines a network socket
//
// Presume that most of the functions in this class can throw runtime_error exceptions.
//==========================================================================================================
#pragma once
#include <netinet/in.h>
#include <string>
#include <stdexcept>

class NetSock
{
public:

    // The are the codes that can be returned by get_error()
    enum
    {
        BIND_FAILED,
        NO_SUCH_SERVER,
        CANT_CONNECT
    };


    // Constructor and Destructor
    NetSock();
    ~NetSock() {close();}

    // Copy constructor
    NetSock(const NetSock& rhs) {copy_object(rhs);}
    
    // Assignment 
    NetSock& operator=(const NetSock& rhs) {copy_object(rhs); return *this;}

    // Call this to create a server socket
    bool    create_server(int port, std::string bind_to = "", int family = AF_UNSPEC, std::string interface = "");

    // Call this to start listening for connections.  Can throw runtime_error
    void    listen(int concurrent_connections = 0);

    // Call this to wait for someone to connect to a server socket
    bool    accept(int timeout_ms = -1, NetSock* new_sock = NULL);

    // Convenience call for waiting for a single incoming connection
    bool    listen_and_accept(int timeout_ms = -1);

    // Call this to connect to a server
    bool    connect(std::string server_name, int port, int family = AF_INET, std::string interface = "");

    // Waits for data to arrive.  Returns 'true' if data became available before the timeout expires
    bool    wait_for_data(int milliseconds);

    // Returns the number of bytes available for reading 
    int     bytes_available();

    // Call this to receive a fixed amount of data from the socket
    int     receive(void* buffer, int length, bool peek = false);

    // Call these to send a string or buffer full of data
    int     send(const void* buffer, int length);

    // Call this to close this socket.  Safe to call if socket isn't open
    void    close();

protected:

    // Copy another object of this type
    void    copy_object(const NetSock& rhs);

    // Most recent error
    std::string m_error_str;
    int     m_error;

    // This will be true on a socket for which create_server() or connect() has been called
    bool    m_is_created;

    // The socket descriptor of our socket
    int     m_sd;
};