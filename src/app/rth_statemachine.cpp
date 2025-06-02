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
// State machine for Remote Test Harness
//==========================================================================================================

#include "common.h"

// This is the current state of RTH
rth_state_t rth_state;

// This is the current state of handshake. Start with assuming no handshake yet
rth_handshake_t rth_hs = NO_HS;

// Keep status of SLAC
int SLAC_init = false;

// SLAC settings
int slac_attn_limit = 70; // in dB. Use 40 for CCS cable, 70 for direct BNC connection
int slac_settle_time = 1; // in seconds. Can increase if more time is needed after AVLN is established
int slac_timeout = 20; // timeout for SLAC waiting to connect
int slac_retries = 10; // number of retries for SLAC

// TCP/IP server objects
CClient Client;
int TCP_port = 65535;
int server_flag = 0;

// Create a one-shot timer to keep track of timeouts when in remote mode
OneShot remote_timer;

// -----------------------------------------------------------------------------
// network_check() - This will perform a ping test to google servers to see if we have network connectivity
// -----------------------------------------------------------------------------
int network_check()
{
    printf(BOLD_YELLOW "\nPerforming Network check .. " RESET "\n\n");
    // const char* hostname = "google.com";
    const char* hostname = "8.8.8.8";
    std::string command = "ping -c 1 ";
    command.append(hostname);
    int rc = system(command.c_str());
    return rc;
}
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// broker_check() - This will attempt to connect to global MQTT broker
// -----------------------------------------------------------------------------
int broker_check()
{
    printf(BOLD_YELLOW "\nConnecting to global Broker .. \n\n" RESET);

    // Configure and connect to global MQTT broker using TLS (refer to wolfMQTT_cpp.h for init() argument details)
    global_broker.init(MQTT_QOS_0, 60, 30000, 5000, true, 80, 1024);
    int rc = global_broker.connect(mqtt.broker_ip, mqtt.broker_port, mqtt.username, mqtt.password, mqtt.client_id);
    printf("connect rc: %d\n", rc);

    // If connection to broker failed
    if (rc != 0)
    {
        char error_msg[50];
        snprintf(error_msg, sizeof(error_msg), "Error: Connection to MQTT broker failed. RC: %d\n", rc);
        logger.log(LOG_ERR, error_msg);
        printf(BOLD_RED "\nConnecting to global broker failed.\n\n" RESET);
        return rc;
    }

    // Otherwise we're good; Subscribe to relavant topic based on device type
    if (config.device_type == "EV")
    {
        global_broker.subscribe(mqtt.evse_message);
    }
    else if (config.device_type == "EVSE")
    {
        global_broker.subscribe(mqtt.ev_message);
    }

    // If we get here, we're configured and ready to run the app properly
    logger.log(LOG_INFO, "Initialized, connected to brokers and ready for comms");
    return rc;
}
// -----------------------------------------------------------------------------



// -----------------------------------------------------------------------------
// rth_handshake() - This will attempt to perform a handshake with both RTH devices
// -----------------------------------------------------------------------------
int rth_handshake()
{
    printf(BOLD_YELLOW "\nPerforming RTH Handshake .. \n\n" RESET);

    // Keep track of number of retries
    int num_retries = 0;

    while (1)
    {   
        // EVSE will issue a handshake command first
        if (rth_hs == NO_HS && config.device_type == "EVSE")
        {
            send_message(mqtt.evse_message, "1");
        }
            

        // EV will reply back with its handshake command
        if (rth_hs == FIRST_HS && config.device_type == "EV")
        {
            send_message(mqtt.ev_message, "1");

            // EV will send handshake message 3 times before considering it a success
            if (num_retries >= 3)
                rth_hs = BOTH_HS;
        }

        // EVSE received handshake response from EV
        if (rth_hs == BOTH_HS)
        {
            logger.log(LOG_INFO, "RTH Handshake established with EV");
            break;
        }

        // Timeout after trying a few times
        if (num_retries > 10)
        {
            logger.log(LOG_ERR, "Error: Could not establish handshake with other RTH device");
            printf(BOLD_RED "\nRTH Handshake failed.\n\n" RESET);

            return -1;
        }
        else num_retries++;

        // Sleep a bit to see if handshake message received before trying again
        sleep(3);

        // If application interrupted by a signal, exit it
        if (signal_captured) exit_app(0);
    }
    
    printf("RTH Handshake established! \n");
    // If we get here, we're good to go
    return 0;
}
// -----------------------------------------------------------------------------



// -----------------------------------------------------------------------------
// perform_SLAC() - Initialize settings and perform SLAC to form AVLN
// -----------------------------------------------------------------------------
int perform_SLAC()
{
    printf(BOLD_YELLOW "\nPerforming SLAC .. " RESET "\n\n");

    // Initialize SLAC if it hasn't been done so already
    if (SLAC_init == 0)
    {
        // Initialize SLAC settings based on device type
        if (config.device_type == "EV")
        {
            if (!SLAC.init(PEV, 'l', slac_attn_limit, slac_timeout, slac_retries))
            {
                printf("SLAC init failed.\n");
                SLAC_init = 0;
                return -1;
            }
            SLAC_init = 1;
        }
        else if (config.device_type == "EVSE")
        {
            if (!SLAC.init(EVSE))
            {
                printf("SLAC init failed.\n");
                SLAC_init = 0;
                return -1;
            }
            SLAC_init = 1;
        }
    }

    // Attempt to perform SLAC here
    if (SLAC.connect() <= 0)
    {
        printf(BOLD_RED "\nSLAC failed.\n\n" RESET);
        logger.log(LOG_ERR, "SLAC failed.");
        return -1;
    }

    // If we get here, AVLN successfully established
    return 0;
}
// -----------------------------------------------------------------------------



// -----------------------------------------------------------------------------
// setup_TCP_network() - This function handles creating TCP server and connecting
//                        as client depending on device type
// -----------------------------------------------------------------------------
int setup_TCP_network()
{
    if (config.device_type == "EVSE")
    {
        // Create a TCP server
        printf("Creating SECC server on port %d\n", TCP_port);
        Server.launch(TCP_port);
    }

    else if (config.device_type == "EV")
    {
        // Connect to TCP server
        printf("\nConnecting to SECC server on port %d\n", SECC_port);
        Client.launch(ipv6_str, SECC_port);
    }

    return 0;
}
// -----------------------------------------------------------------------------



// -----------------------------------------------------------------------------
// rth_state_machine() - This function handles all states of the RTH and
//                       dispatches appropriate functions accordingly
// -----------------------------------------------------------------------------
int rth_state_machine()
{
    switch (rth_state)
    {
        // ---------------------------------------------------------------------
        case INIT:
            // If we get here the first time, we are initialized properly
            rth_state = NETWORK_CHECK;

            break;

        // ---------------------------------------------------------------------
        case NETWORK_CHECK:
            if (network_check() != 0)
            {
                return -1;
            }
            else
            {
                rth_state = BROKER_CHECK;
                break;
            }

        // ---------------------------------------------------------------------
        case BROKER_CHECK:
            if (broker_check() != 0)
            {
                return -1;
            }
            else
            {
                rth_state = RTH_HANDSHAKE;
                break;
            }

        // ---------------------------------------------------------------------
        case RTH_HANDSHAKE:
            if (rth_handshake() != 0)
            {
                return -1;
            }
            else
            {
                rth_state = UNPLUGGED_WAIT;
                printf("\nWaiting for coupler to be plugged in .. \n\n");
                break;
            }

        // ---------------------------------------------------------------------
        case UNPLUGGED_WAIT:
            if (server_flag == 0 && config.device_type == "EVSE")
            {
                if (!secc_udp.create_server(15118, "::", AF_INET6))
                {
                    fprintf(stderr, "Can't create listener on UDP port %i\n", 15118);
                    exit(0);
                }
                if (setup_TCP_network() != 0)
                {
                    printf("TCP/IP network failed");
                    return -1;
                }
                server_flag = 1;
            }
            
            // Check if coupler is plugged in
            if (J1772.pilot_state_name == "B1" && config.device_type == "EVSE")
            {
                // Turn oscillator on at 5% duty cycle
                set_pwm(5.00);

                // Wait for a bit to ensure oscillator turned on
                sleeper.sleep(100);
            }

            // Wait for a bit till status update message received
            sleeper.sleep(100);
            
            if (J1772.pilot_state_name == "B2")
            {   
                rth_state = PLUGGED_IN;
                printf("Plugged in!\n");
                break;
            }
            break;
            

        // ---------------------------------------------------------------------
        case PLUGGED_IN:
            // Try SLAC here
            printf("Performing SLAC\n");
            if (perform_SLAC() != 0)
            {
                printf("SLAC failed\n");
                return -1;
            }

            // SLAC is successful
            else
            {
                rth_state = SDP;
                printf("AVLN established!\n");
                break;
            }
        
        // ---------------------------------------------------------------------
        case SDP:
            if (perform_SDP() != 0)
            {
                printf("SDP failed\n");
                return -1;
            }
            else
            {
                rth_state = TCP_NETWORK;
                break;
            }
        
        // ---------------------------------------------------------------------
        case TCP_NETWORK:

            if (config.device_type == "EV")
            {
                if (setup_TCP_network() != 0)
                {
                    printf("TCP/IP network failed");
                    return -1;
                }
                else
                {
                    rth_state = REMOTE;
                    printf("\n\033[1;33mRemote mode now enabled.\033[0m\n\n");
                }
            }

            else if (config.device_type == "EVSE")
            {
                rth_state = REMOTE;
                printf("\n\033[1;33mRemote mode now enabled.\033[0m\n\n");
            }

            // Set a timer for 90 seconds
            remote_timer.start(90000);            
            break;

        // ---------------------------------------------------------------------
        case REMOTE:

            // If 30 seconds have passed, exit the application
            if (remote_timer.is_expired())
            {
                // Untoggle CP resistor back to 12 V
                logger.log(LOG_ERR, "Timed out in remote mode");
                printf("\nTimed out.\n");
                exit_app(0);
            }
            break;

        // ---------------------------------------------------------------------
        // If we get here, something is wrong
        default:
            return -1;
    }

    // If we get here, all is good
    return 0;
}
// -----------------------------------------------------------------------------

