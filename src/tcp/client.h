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


#pragma once
#include <string>
#include "netsock.h"

class CClient
{
public:

    CClient() {m_psock = NULL; m_connected = false;}

    void    launch(std::string remote_ip, int remote_port);

    void    send(void* buffer, int byte_count);

    void    task();

protected:
    
    bool        m_connected;
    std::string m_remote_ip;
    int         m_remote_port;
    NetSock*    m_psock;
};
