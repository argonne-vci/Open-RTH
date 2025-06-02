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
#include <cstring>
#include <cstdio>
#include <thread>
#include "client.h"
#include "server.h"
#include "common.h"

extern CServer Server;

static void launch_task(CClient* p) {p->task();}

// -----------------------------------------------------------------------------
// launch() - This will launch the client-side thread
// -----------------------------------------------------------------------------
void CClient::launch(std::string remote_ip, int remote_port)
{
    m_remote_ip = remote_ip;
    m_remote_port = remote_port;
    std::thread th(launch_task, this);
    th.detach();
}
// -----------------------------------------------------------------------------



// -----------------------------------------------------------------------------
// task() - The server-side task that connects to a server on a given port, listens
//          for incoming connects and waits for data to arrive. It will also send
//          data back on the port when needed
// -----------------------------------------------------------------------------
void CClient::task()
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

    // Create a new client socket
    m_psock = new NetSock;

    // Connect to the remote server
    while (!m_psock->connect(m_remote_ip, m_remote_port, AF_INET6, "qca0"))
    {
        printf("Failed to connect to %s:%i.  Retrying\n", m_remote_ip.c_str(), m_remote_port);
        sleep(1);        
    }

    // We have a valid connection
    m_connected = true;

    // Tell the world that we're connected
    printf("Connected to remote server at %s\n\n", m_remote_ip.c_str());

    // std::string message = "Hello!";
    // std::string message = "a0b0c0d0e0f000182336b5cd86dd6000000000600640fe80000000000000021823fffe36b5cdfe800000000000003933e74737944644c001ffff0015afc3e089f2a3501840007d96000001fe8001000000448000dbab9371d3234b71d1b981899189d191818991d26b9b3a232b30020000040401d75726e3a69736f3a31353131383a323a323031333a4d73674465660040000000080";
    //std::string message = "01fe8001000000228000dbab9371d3234b71d1b981899189d191818991d26b9b3a232b30020000040040";
    // string message = "303166653830303130303030303032323830303064626162393337316433323334623731643162393831383939313839643139313831383939316432366239623361323332623330303230303030303430303430";
    //CClient::send((void *)message.c_str(), strlen(message.c_str()));

    while (true)
    {
        // If RTH data is received over MQTT
        if (rth_data_received)
        {
            // Convert hexadecimal string to binary
            char rth_datapacket_bin[65536];
            size_t bin_length = convert_hex_to_binary(rth_datapacket.c_str(), rth_datapacket_bin);

            printf(BOLD_MAGENTA "<-- (MQTT)" RESET " Received EV req Datapacket (%d bytes): ", bin_length);
            for (size_t i = 0; i < bin_length; ++i)
                printf("%02x", (unsigned char)rth_datapacket_bin[i]);
            printf("\n");

            // Send it to the server
            // printf("Sending request message to EVSE .. \n");
            printf(BOLD_BLUE "--> (TCP)" RESET "  Sending EV req Datapacket\n\n");
            CClient::send((void *)rth_datapacket_bin, bin_length);

            // Wait for data to arrive.  If the client closes the socket, break
            if (!m_psock->wait_for_data(-1)) break;

            // How many bytes are available to read?
            int bytes_ready = m_psock->bytes_available();
            // int bytes_ready = 100;

            // If the other side closed the connection, break
            if (bytes_ready < 1) break;

            // Don't ready more bytes than our buffer can hold!
            // if (bytes_ready > sizeof(buffer)) bytes_ready = sizeof(buffer);
            
            // Fetch the data-bytes that are available
            int bytes_rcvd = m_psock->receive(buffer, bytes_ready);

            // If we didn't get all of our bytes, the other side closed the connection
            if (bytes_rcvd < bytes_ready) break;


            // Convert the buffer to a hexadecimal string of proper size
            int buffer_hex_length = bytes_rcvd * 2 + 1;
            char buffer_hex[buffer_hex_length];
            convert_binary_to_hex(buffer, bytes_rcvd, buffer_hex);
            printf(BOLD_BLUE "<-- (TCP)" RESET "  Received EVSE res Datapacket (%d bytes): %s\n", bytes_rcvd, buffer_hex);

            // Send the received data to the RTH server side
            printf(BOLD_MAGENTA "--> (MQTT)" RESET " Sending EVSE res Datapacket\n\n");
            send_message(mqtt.ev_message, buffer_hex);
            rth_data_received = 0;
        }
        // Wait until a new message arrives
        sleeper.sleep(50);
    }

    // If we get here, connection is dropped
    m_connected = false;
    printf("Remote server dropped connection\n");
    goto again;

}
// -----------------------------------------------------------------------------



// -----------------------------------------------------------------------------
// send() - Sends a buffer to the other side of a connected socket
// -----------------------------------------------------------------------------
void CClient::send(void* buffer, int byte_count)
{
    if (m_psock && m_connected)
    {
        m_psock->send(buffer, byte_count);      
    }    
}
// -----------------------------------------------------------------------------
