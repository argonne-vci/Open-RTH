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
// config.h - Define a parser for configuration files
//==========================================================================================================

#pragma once

#include <string>

// A place to hold MQTT configuration settings extracted from the config file
extern struct mqtt_config_t
{
    // MQTT broker settings
    std::string broker_ip;
    int broker_port;
    int tls_setting;

    // MQTT client settings
    std::string username, password, client_id;

    // Global MQTT topics
    std::string ev_message, evse_message, ev_J1772_status_topic, evse_J1772_status_topic, ev_state, evse_state;
} mqtt;

// A place to hold general configuration settings extracted from the config file
extern struct config_t
{
    // Enable or disable logging
    std::string log_setting;

    // Whether the device will be emulating an EV or EVSE
    std::string device_type;

    // The amount of time in ms which the program will wait between sending a message over UART and reading its response
    int response_delay_ms;
} config;

// This function reads in the configuration file and saves values in memory
void fetch_config(std::string filename);

//==========================================================================================================
