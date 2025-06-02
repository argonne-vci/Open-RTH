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
#include "netsock.h"

class CServer
{
public:
    CServer() {m_psock = NULL; m_connected = false;}

    void    launch(int local_port);

    void    send(void* buffer, int byte_count);

    void    task();

    void    close();

protected:

    bool        m_connected;
    int         m_local_port;
    NetSock*    m_psock;
};
