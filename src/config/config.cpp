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
// config.cpp - Implement a parser for configuration files
//==========================================================================================================

#include <string.h>
#include <stdexcept>

#include "main.h"
#include "common.h"
#include "config_file.h"

// Configuration settings are stored here
mqtt_config_t mqtt;
config_t config;

//==========================================================================================================
// fetch_config() - Reads in the configuration file
//==========================================================================================================
void fetch_config(std::string filename)
{
    // Create a config_file object
    CConfigFile conf;

    // Read the configuration file and quit if we can't
    if (!conf.read(filename))
    {
        logger.log(LOG_ERR, "Failed to open config file. Exiting application");
        exit(1);
    }

    try
    {
        // Get general settings from config file
        conf.set_current_section("General");
        conf.get("logging", &config.log_setting);
        conf.get("response_delay_ms", &config.response_delay_ms);
        conf.get("device_type", &config.device_type);

        // Get MQTT settings from config file
        conf.set_current_section("MQTT");
        conf.get("broker_ip", &mqtt.broker_ip);
        conf.get("broker_port", &mqtt.broker_port);
        conf.get("username", &mqtt.username);
        conf.get("password", &mqtt.password);
        conf.get("client_id", &mqtt.client_id);

        // Get global topics to publish and subscribe to
        conf.get("ev_message", &mqtt.ev_message);
        conf.get("evse_message", &mqtt.evse_message);
        conf.get("ev_J1772_status_topic", &mqtt.ev_J1772_status_topic);
        conf.get("evse_J1772_status_topic", &mqtt.evse_J1772_status_topic);
        conf.get("ev_state", &mqtt.ev_state);
        conf.get("evse_state", &mqtt.evse_state);
    }

    // If any configuration setting is missing, it's fatal error
    catch(std::runtime_error& ex)
    {
        const char* err = ex.what();
        char err_msg[100] = {'\0'};
        strcat(err_msg, "Config parameter missing or incorrect: ");
        strcat(err_msg, err);
        logger.log(LOG_ERR, err_msg);
        exit(1);
    }
}
//==========================================================================================================
