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


#ifndef IO_H
#define IO_H

#include <cstdio>
#include <cstring>
#include <vector>

// open_and_configure_serial_port - Function to open and configure the serial port
int open_and_configure_serial_port(const char* serial_port);

// Function to read data from a serial port
void read_from_serial(int fd, std::vector<unsigned char>& data, bool mute=true);

// read_serial_data - a second attempt at a function to read incoming serial data
void read_serial_data(int fd, std::vector<std::vector<unsigned char> >& messages, bool mute=true);

// flush_read_buffer - discard data in the serial input buffer
void flush_read_buffer(int fd);

// Funtion to write a series of hex characters over serial
int write_to_serial(int fd, const std::vector<unsigned char>& data, bool mute=true);

// calculateBBC - Receives a vector of hex values and calculates the BCC
unsigned char calculate_bcc(const std::vector<unsigned char>& data);

#endif // IO_H