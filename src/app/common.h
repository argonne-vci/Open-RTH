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
// Common header files and objects used in the app
//==========================================================================================================

#pragma once

#include <mutex>
#include <pthread.h>
#include <string>

#include "client.h"
#include "config.h"
#include "io.h"
#include "J1772.h"
#include "json.h"
#include "logger.h"
#include "mqtt.h"
#include "mstimer.h"
#include "netsock.h"
#include "rth_statemachine.h"
#include "sdp.h"
#include "server.h"
#include "slacify.h"
#include "sleeper.h"
#include "tcpdump.h"
#include "udpsock.h"
#include "wolfMQTT_cpp.h"

// Global file descriptor for serial port
extern int uart_fd; // This is specific to the EVACharge_SE
            // If this program will support multiple hardwares, move this to a more appropriate place

// Declare all external objects here
extern CWolfMQTT global_broker;
extern CLogger logger;
extern CSLACify SLAC;
extern CSleeper sleeper;
extern UDPSock evcc_udp, secc_udp;
extern NetSock secc_client, secc_server;
extern TCPDump tcpdump;
extern CServer Server;

// Declare all external variables
extern rth_state_t rth_state;
extern pthread_mutex_t publish_mtx;
extern rth_handshake_t rth_hs;
extern J1772_t J1772;
extern int rth_data_received;
extern std::string rth_datapacket;
extern std::string network_interface;
extern int signal_captured;
extern int J1772_status_received;
extern std::string J1772_status_msg;

// Declare all external functions here
void send_message(std::string topic, std::string message);
size_t convert_hex_to_binary(const char* hex_data, char* buffer);
void convert_binary_to_hex(const char* buffer, size_t length, char* hex_output);
extern void exit_app(int);

// Declare global SECC variables here
extern uint16_t SECC_port;
extern char SECC_ipv6addr[INET6_ADDRSTRLEN];
extern std::string ipv6_str;
extern int TCP_port;

// Define common colors to print to screen
#define BLACK        "\033[0;30m"
#define RED          "\033[0;31m"
#define GREEN        "\033[0;32m"
#define YELLOW       "\033[0;33m"
#define BLUE         "\033[0;34m"
#define MAGENTA      "\033[0;35m"
#define CYAN         "\033[0;36m"
#define WHITE        "\033[0;37m"
#define BOLD_BLACK   "\033[1;30m"
#define BOLD_RED     "\033[1;31m"
#define BOLD_GREEN   "\033[1;32m"
#define BOLD_YELLOW  "\033[1;33m"
#define BOLD_BLUE    "\033[1;34m"
#define BOLD_MAGENTA "\033[1;35m"
#define BOLD_CYAN    "\033[1;36m"
#define BOLD_WHITE   "\033[1;37m"
#define RESET        "\033[0m"

//==========================================================================================================
