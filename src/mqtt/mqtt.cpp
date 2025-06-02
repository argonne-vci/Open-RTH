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
// mqtt.cpp - All MQTT related functions and variables implemented here
//==========================================================================================================

#include <cstdlib>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include "common.h"
#include "main.h"

// Flag that indicates if RTH data is received
int rth_data_received;

// Flag that indicates if a J1772 status message is received
int J1772_status_received;

// A J1772 status message
std::string J1772_status_msg;

// An RTH TCP/IP data packet
std::string rth_datapacket;


// -----------------------------------------------------------------------------
// handle_message()
// -----------------------------------------------------------------------------
void CWolfMQTT::handle_message(const std::string& topic, const std::string& message)
{
    // Make sure the message arrives on the correct topic

    /** Global topics **/
    if (topic == mqtt.ev_message)
    {
        // Handle handshake messages here
        if (message == "1")
        {
            // Reply handshake message received from EV
            rth_hs = BOTH_HS;
            return;
        }

        // handle all other messages here
        else
        {
            // Save the message received
            rth_data_received = 1;
            rth_datapacket = message;
            return;
        }
    }
    else if (topic == mqtt.evse_message)
    {
        // Handle handshake messages here
        if (message == "1")
        {
            // First handshake message received from EVSE
            rth_hs = FIRST_HS;
            return;
        }

        // handle all other messages here
        else
        {
            // Save the message received
            rth_data_received = 1;
            rth_datapacket = message;
            return;
        }
    }
    else
        logger.log(LOG_WARNING, "Message arrived on invalid topic");
}
// -----------------------------------------------------------------------------

