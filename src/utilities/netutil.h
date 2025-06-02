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
// netutil.h - Defines some helpful utility functions for networking
//==========================================================================================================
#pragma once
#include <netinet/in.h>
#include <string>
#include <netdb.h>


// This is the equivalent of a 'struct addrinfo', but with no pointers
struct addrinfo_t
{
    addrinfo_t& operator=(addrinfo& rhs) {from_ai(rhs); return *this;}
    operator sockaddr*() const {return (sockaddr*)&addr;}
    void      from_ai(addrinfo& rhs);
    sockaddr_storage addr;
    socklen_t        addrlen;
    int              family;
    int              socktype;
    int              protocol;
};

struct NetUtil
{

    // Returns addrinfo about the local machine
    static addrinfo_t get_local_addrinfo(int type, int port, std::string bind_to, int family);

    // Returns addrinfo about a remove server
    static bool get_server_addrinfo(int type, std::string server, int port, int family, addrinfo_t* p_result);

    // Fetches the ASCII IP address from a sockaddr*.  
    static std::string ip_to_string(sockaddr* addr);

    // Converts a sockaddr_storage to an ASCII IP address.
    static std::string ip_to_string(sockaddr_storage& ss);

    // Call this to wait for data to arrive on anywhere from 1 to 4 descriptors
    // timeout_ms of -1 means "wait forever"
    static int wait_for_data(int timeout_ms, int fd1, int fd2 = -1, int fd3 = -1, int fd4 = -1);
};

