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
// Common objects used in the app
//==========================================================================================================

#include "common.h"
#include <bitset>
#include <iomanip>

// Define all global objects here
CLogger logger;
CSLACify SLAC;
CSleeper sleeper;
CWolfMQTT global_broker;
UDPSock evcc_udp, secc_udp;
NetSock secc_client, secc_server;
TCPDump tcpdump;
CServer Server;

// -----------------------------------------------------------------------------
// send_message() - Handy function to publish a message on the global MQTT broker in a thread-safe manner
// -----------------------------------------------------------------------------
void send_message(std::string topic, std::string message)
{
    pthread_mutex_lock(&publish_mtx);
    global_broker.publish(topic.c_str(), message);
    pthread_mutex_unlock(&publish_mtx);
}
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// convert_hex_to_binary() - Function to convert a hexadecimal string to binary data
//                           and return the actual length of binary data
// -----------------------------------------------------------------------------
size_t convert_hex_to_binary(const char* hex_data, char* buffer)
{
    size_t data_length = strlen(hex_data) / 2;  // Length of the binary data
    for (size_t i = 0; i < data_length; ++i)
        sscanf(&hex_data[i * 2], "%2hhx", &buffer[i]);
    
    return data_length;
}
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// convert_binary_to_hex() - Function to convert binary data to a hexadecimal string
// -----------------------------------------------------------------------------
void convert_binary_to_hex(const char* buffer, size_t length, char* hex_output)
{
    for (size_t i = 0; i < length; ++i)
        sprintf(&hex_output[i * 2], "%02x", (unsigned char)buffer[i]);

    hex_output[length * 2] = '\0';  // Null-terminate the string
}
// -----------------------------------------------------------------------------


//==========================================================================================================
