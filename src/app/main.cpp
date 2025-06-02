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

#include <iostream>
#include <signal.h>
#include <sstream>
#include <stdio.h>
#include <string>
#include <string.h>  // Add this for snprintf
#include <stdlib.h>  // For exit()
#include <sys/select.h>
#include <unistd.h>

#include "main.h"
#include "common.h"
#include "../../version.h"

// Define the config filename to read
std::string config_file = "rth.conf";

// Base filename of the pcap logfile
std::string tcpdump_filename = "logs/RTH_log";

// Create a mutex to ensure data is only published by one thread at a time
pthread_mutex_t publish_mtx = PTHREAD_MUTEX_INITIALIZER;

// Global file descriptor for serial port
int uart_fd = -1;

// Flag that holds status of signal if received
int signal_captured = 0;

// This is the signal handler when a signal is captured
void sig_handler(int) {signal_captured = 1;}

// Create a one-shot timer to keep track of timeouts when in remote mode
OneShot rth_state_timer;
OneShot j1772_publish_timer;

// -----------------------------------------------------------------------------
// exit_app() - Function that handles graceful shutdown of app
// -----------------------------------------------------------------------------
void exit_app(int)
{  
    logger.log(LOG_WARNING, "Shutting down Application");

    // Disable the PWM on the board
    if (config.device_type == "EVSE") control_pwm(0); // Disable PWM 

    // Stop tcpdump process if it wasn't already
    tcpdump.stop();

    // Disconnect from MQTT broker
    global_broker.close();

    // Close the uart device
    if(uart_fd != -1) { close(uart_fd); }

    if (config.device_type == "EVSE") Server.close(); // close the server-side socket

    // Exit the application
    printf("\nExit.\n");
    exit(0);
}
// -----------------------------------------------------------------------------



// -----------------------------------------------------------------------------
// main()
// -----------------------------------------------------------------------------
int main()
{
     // Initialize a logger
    logger.init("RTH", logfilename);

    // Fetch the settings from config file
    fetch_config(config_file);
    char start_log[30];
    snprintf(start_log, sizeof(start_log), "Starting RTH %s as %s.", SW_VERSION, config.device_type.c_str());
    logger.log(LOG_INFO, start_log);

    // Display banner
    printf(BOLD_CYAN);
    printf("\n------------------------------------\n");
    printf("Remote Test harness - %s\n", config.device_type.c_str());
    printf("v%s\n", SW_VERSION);
    printf("\xC2\xA9 2025 Argonne National Laboratory");
    printf("\n------------------------------------");
    printf(RESET);
    printf("\n\n");

    // Open the serial port
    const char* serial_port = "/dev/ttyAPP2";
    uart_fd = open_and_configure_serial_port(serial_port);
    if(uart_fd == -1)
    {
        printf("Failed to open serial port %s", serial_port);
        return -1;
    }

    // Flush the uart buffer, sometimes the first read returns garbage
    flush_read_buffer(uart_fd);

    // Register exit_app() as a signal handler so it can capture Ctrl+C and kill -15 commands
    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);
    signal(SIGPIPE, SIG_IGN); // ignore SIGPIPE

    // Start a tcpdump process to capture network traffic
    tcpdump_filename += "_" + config.device_type;
    tcpdump.start(network_interface, tcpdump_filename);

    // Initialize a sleeper
    sleeper.init();

    // Initialize J1772 document to hold status values
    init_json();

    // Set EVSE to State A to begin with
    if (config.device_type == "EVSE") 
    {
        control_pwm(1); // Enable PWM
        set_pwm(99.9);  // Set PWM to 99.9% duty cycle
        sleep(0.5);
    }

    // Get initial status of J1772
    update_J1772_status();
    sleep(0.5);
    
    // Print the first J1772 status message
    printf("J1772 pilot state: %s\n", J1772.pilot_state_name.c_str());

    // Start a timer to publish the RTH state to the MQTT broker
    rth_state_timer.start(250); 
    j1772_publish_timer.start(250);

    // Loop forever
    while (1)
    {
        // Continuously take samples
        sample_J1772();
        
        // If a state change we care about is detected
        
        if(j1772_publish_timer.is_expired())
        {
            // Create a JSON payload with updated J1772 status values
            update_J1772_status();

            // Publish J1772 values to MQTT broker
            if(config.device_type == "EV")
            {    send_message(mqtt.ev_J1772_status_topic.c_str(), J1772_status_str.c_str()); }
            else if(config.device_type == "EVSE")
            {    send_message(mqtt.evse_J1772_status_topic.c_str(), J1772_status_str.c_str()); }

            j1772_publish_timer.start(250);
        }

        // Save the current value to compare with later
        old_J1772 = J1772;

        std::string rth_state_str = "No state found.";
        if (rth_state_timer.is_expired())
        {
            // Publish rth state to broker
            switch(rth_state) {
                case INIT:              rth_state_str = "INIT";             break;
                case NETWORK_CHECK:     rth_state_str = "NETWORK_CHECK";    break;
                case BROKER_CHECK:      rth_state_str = "BROKER_CHECK";     break;
                case RTH_HANDSHAKE:     rth_state_str = "RTH_HANDSHAKE";    break;
                case UNPLUGGED_WAIT:    rth_state_str = "UNPLUGGED_WAIT";   break;
                case PLUGGED_IN:        rth_state_str = "PLUGGED_IN";       break;
                case SDP:               rth_state_str = "SDP";              break;
                case TCP_NETWORK:       rth_state_str = "TCP_NETWORK";      break;
                case REMOTE:            rth_state_str = "REMOTE";           break;
                case ERROR:             rth_state_str = "ERROR";            break;
            };

            if(config.device_type == "EV")
            {    send_message(mqtt.ev_state.c_str(), rth_state_str.c_str()); }
            else if(config.device_type == "EVSE")
            {    send_message(mqtt.evse_state.c_str(), rth_state_str.c_str()); }
                    

            rth_state_timer.start(250);
        }

        // Execute RTH state machine here
        if (rth_state_machine() != 0)
        {
            char log_error[30];
            snprintf(log_error, sizeof(log_error), "Error occured in state %d\n", rth_state);
            logger.log(LOG_INFO, log_error);
            exit_app(0);
        }

        // Check if coupler got unplugged halfway through the session
        if (rth_state >= PLUGGED_IN && (J1772.pilot_state_name == "A1" || J1772.pilot_state_name == "A2" || J1772.pilot_state_name == "F"))
        {
            printf(BOLD_RED "\nCoupler removed!! Stopping session." RESET "\n\n");
            
            // Turn oscillator off
            if (config.device_type == "EVSE")
                control_pwm(0);
            
            // log it
            char log_error[30];
            snprintf(log_error, sizeof(log_error), "Coupler removed in state %d\n", rth_state);
            logger.log(LOG_INFO, log_error);
            
            // then exit the app
            exit_app(0);
        }

        // Sleep a bit to not bog down the CPU
        sleeper.sleep(50);

        // If application interrupted by a signal, exit it
        if (signal_captured) exit_app(0);
    }

    printf("done.\n");
    exit_app(0);
}
// -----------------------------------------------------------------------------
