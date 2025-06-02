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


 #include <unistd.h>
#include <cstdio>
#include <thread>
#include <string>

#include "client.h"
#include "server.h"
#include "common.h"

extern CClient Client;

static void launch_task(CServer* p) {p->task();}

// -----------------------------------------------------------------------------
// launch() - This will launch the server-side thread
// -----------------------------------------------------------------------------
void CServer::launch(int local_port)
{
    m_local_port = local_port;
    std::thread th(launch_task, this);
    th.detach();
}
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// close() - This will close the client-side thread
// -----------------------------------------------------------------------------
void CServer::close()
{
    m_psock->close();
}
// -----------------------------------------------------------------------------



// -----------------------------------------------------------------------------
// task() - The server-side task that creates a server on agiven port, listens
//          for incoming connects and waits for data to arrive. It will also send
//          data back on the port when needed
// -----------------------------------------------------------------------------
void CServer::task()
{
    char buffer[0x10000];

again:

    // If application interrupted by a signal, exit it
    if (signal_captured) exit_app(0);

    // If we have a server socket already, throw it away
    if (m_psock)
    {
        m_connected = false;
        delete m_psock;
        m_psock = NULL;
    }

    // Create a new server socket
    m_psock = new NetSock;
    m_psock->create_server(m_local_port, "", AF_INET6, "qca0");

    // Wait for someone to connect
    m_psock->listen_and_accept();

    // We have a client connected to us
    m_connected = true;
    printf("Remote client connected to server\n\n");


    while (true)
    {   
        // Wait for data to arrive.  If the client closes the socket, break
        if (!m_psock->wait_for_data(-1)) break;
      
        // How many bytes are available to read?
        int bytes_ready = m_psock->bytes_available();

        // If the other side closed the connection, break
        if (bytes_ready < 1) break;

        // Don't ready more bytes than our buffer can hold!
        if (bytes_ready > sizeof(buffer)) bytes_ready = sizeof(buffer);
        
        // Fetch the data-bytes that are available
        int bytes_rcvd = m_psock->receive(buffer, bytes_ready);

        // If we didn't get all of our bytes, the other side closed the connection
        if (bytes_rcvd < bytes_ready) break;

        // Convert the buffer to a string iteratively ignoring any null terminators in the buffer
        std::string buffer_str(buffer, bytes_rcvd);

        // Convert the buffer to a hexadecimal string of proper size
        int buffer_hex_length = bytes_rcvd * 2 + 1;
        char buffer_hex[buffer_hex_length];
        convert_binary_to_hex(buffer, bytes_rcvd, buffer_hex);
        printf(BOLD_BLUE "<-- (TCP)" RESET "  Received EV req Datapacket (%d bytes): %s\n", bytes_rcvd, buffer_hex);

        // Send message to the global broker
        printf(BOLD_MAGENTA "--> (MQTT)" RESET " Sending EV req Datapacket\n\n");
        send_message(mqtt.evse_message, buffer_hex);

        // Wait here until a response is received
        while (1)
        {   
            // If RTH data is received over MQTT
            if (rth_data_received)
            {
                // Convert hexadecimal string to binary
                char rth_datapacket_bin[65536];
                size_t bin_length = convert_hex_to_binary(rth_datapacket.c_str(), rth_datapacket_bin);

                printf(BOLD_MAGENTA "<-- (MQTT)" RESET " Received EVSE res Datapacket (%d bytes): ", bin_length);
                for (size_t i = 0; i < bin_length; ++i)
                    printf("%02x", (unsigned char)rth_datapacket_bin[i]);
                printf("\n");
                
                printf(BOLD_BLUE "--> (TCP)" RESET "  Sending EVSE res Datapacket\n\n");
                CServer::send((void *)rth_datapacket_bin, bin_length);
                rth_data_received = 0;

                break;
            }

            // Wait until a new message arrives
            sleeper.sleep(50);
        }

        // else break;
    }

    // Connection is closed if we get here
    m_connected = false;
    printf("Remote client disconnected from server\n");
    goto again;
}
// -----------------------------------------------------------------------------



// -----------------------------------------------------------------------------
// send() - Sends data over the connected socket
// -----------------------------------------------------------------------------
void CServer::send(void* buffer, int byte_count)
{
    if (m_psock && m_connected)
    {
        m_psock->send(buffer, byte_count);      
    }    
}
// -----------------------------------------------------------------------------
