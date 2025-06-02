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
// wolfMQTT_cpp.h - Defines an MQTT client as a wrapper around wolfMQTT library
//==========================================================================================================

#pragma once

#include <string>

#include "io.h"
#include "wolfmqtt/mqtt_client.h"

class CWolfMQTTBase
{
public:

    // Constructor
    CWolfMQTTBase();
   
    // Destructor
    ~CWolfMQTTBase() {close();}

    // Call this function before calling connect to change default MQTT settings if needed
    void init(
        MqttQoS _MQTT_QOS = MQTT_QOS_0,
        int _MQTT_KEEP_ALIVE_SEC = 60,
        int _MQTT_CMD_TIMEOUT_MS = 30000,
        int _MQTT_CON_TIMEOUT_MS = 5000,
        int _MQTT_USE_TLS = false,
        int _PRINT_BUFFER_SIZE = 80,
        int _MQTT_MAX_PACKET_SIZE = 1024
    );

    // Call this to connect to an MQTT broker
    int connect(std::string ip, int port, std::string username, std::string password, std::string client_id);

    // Call this to subscribe to an MQTT topic on the broker
    int subscribe(std::string topic);

    // Call tghis to unsubscribe from an MQTT topic on the broker
    int unsubscribe(std::string topic);

    // Publish a message on the given topic
    int publish(std::string topic, std::string message);

    // Graceful shutdown of session
    void close();

    // Application-specific message handler. Application should probably never call this directly
    // Needs to be explicitly defined for all objects of derived CWolfMQTTBase
    virtual void handle_message(const std::string& topic, const std::string& message) = 0;

protected:
    // This task executes the wolfMQTT state machine
    void wolfMQTT_state_machine();

    // Pipe to pass messages between main code and task
    int m_pipe[2];

    // Socket descriptor for the connection
    int m_sd;

    // MQTT settings variables
    MqttQoS MQTT_QOS;
    int MQTT_KEEP_ALIVE_SEC;
    int MQTT_CMD_TIMEOUT_MS;
    int MQTT_CON_TIMEOUT_MS;
    int MQTT_USE_TLS;
    int PRINT_BUFFER_SIZE;
    int MQTT_MAX_PACKET_SIZE;

    // wolfMQTT variables
    MqttNet m_network;
    MqttObject mqttObj;
    MqttClient mClient;
};
//==========================================================================================================