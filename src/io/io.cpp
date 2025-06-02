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


#include <algorithm>
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <cstring>

#include "io.h"
#include "common.h"
#include "main.h"


// -----------------------------------------------------------------------------
// calculateBBC - Receives a vector of hex values and calculates the BCC
// -----------------------------------------------------------------------------
unsigned char calculate_bcc(const std::vector<unsigned char>& data)
{
    unsigned char bcc = 0x00;  // Initialize BCC to 0
    // Traditional for loop (C++98 compatible)
    for (size_t i = 0; i < data.size(); ++i) {
        bcc ^= data[i];  // XOR each byte with the current BCC value
    }
    return bcc;
}
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// read_serial_data - a function to read incoming serial data
// -----------------------------------------------------------------------------
void read_serial_data(int fd, std::vector<std::vector<unsigned char> >& messages, bool mute)
{
    unsigned char buffer[256];  // Temporary buffer for reading data
    ssize_t bytesRead;

    // Persistent buffer for accumulating data
    static std::vector<unsigned char> incomingBuffer;

    // Read data from serial port
    bytesRead = read(fd, buffer, sizeof(buffer));
    
    if (bytesRead > 0)
    {
        // Append new data to the static buffer
        incomingBuffer.insert(incomingBuffer.end(), buffer, buffer + bytesRead);

        // Process complete messages
        while(incomingBuffer.size() >= 2)
        {
            // Look for the start byte (0x02)
            std::vector<unsigned char>::iterator start = std::find(incomingBuffer.begin(), incomingBuffer.end(), static_cast<unsigned char>(0x02));
            if(start == incomingBuffer.end())   // No valid start; discard buffer
            {
                incomingBuffer.clear();
                return;
            }

            // Ensure there's enough data to check the the length byte
            if(incomingBuffer.end() - start < 2)    // Wait for more data
                break;

            // Extract the message length
            size_t messageLength = *(start + 1) + 2; // Length byte + start byte and length byte
            if(incomingBuffer.end() - start < static_cast<ptrdiff_t>(messageLength))
                break;  // Wait for the complete message

            // Extract the complete message
            std::vector<unsigned char> message(start, start + messageLength);
            messages.push_back(message);

            // Remove the processed message from the buffer
            incomingBuffer.erase(incomingBuffer.begin(), start + messageLength);

            // Debug print
            if(!mute)
            {
                std::cout << "Complete message: ";
                for(size_t i = 0; i < message.size(); i++)
                    std::cout << "0x" << std::hex << (int)message[i] << " ";
                std::cout << std::endl;
            }
        }
    } 
    else if(bytesRead < 0) 
    {
        std::cerr << "Error reading from port: " <<strerror(errno) << std::endl;
    }
}

// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// flush_read_buffer - discard data in the serial input buffer
// -----------------------------------------------------------------------------
void flush_read_buffer(int fd)
{
    // Use tcflush to discard data in the input buffer, report any error
    if(tcflush(fd, TCIFLUSH) != 0) 
    { 
        std::cerr << "Error flushing read buffer" << std::endl; 
    }     
}
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// write_to_serial - read data from serial port
// -----------------------------------------------------------------------------
int write_to_serial(int fd, const std::vector<unsigned char>& data, bool mute)
{
    // Send the data (hex values)
    ssize_t bytesWritten = write(fd, &data[0], data.size()); 
    if (bytesWritten == -1) {
        std::cerr << "Error writing to port" << std::endl;
        close(fd);
        return -1;
    }

    // If mute != 0, print out the data that was sent
    if(!mute)
    {
        std::cout << "Data sent successfully: ";
        for (size_t i = 0; i < data.size(); ++i) {
            std::cout << "0x" << std::hex << (int)data[i] << " ";
        }
        std::cout << std::endl;
    }

    return 0;
}
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// open_and_configure_serial_port - Function to open and configure the serial port
// -----------------------------------------------------------------------------
int open_and_configure_serial_port(const char* serial_port) {
    int fd = open(serial_port, O_RDWR | O_NOCTTY | O_SYNC);
    if (fd == -1) {
        std::cerr << "Failed to open port: " << serial_port << std::endl;
        return -1;
    }

    // Set up the serial port settings
    struct termios tty;
    memset(&tty, 0, sizeof tty);
    if (tcgetattr(fd, &tty) != 0) {
        std::cerr << "Error getting port attributes" << std::endl;
        close(fd);
        return -1;
    }

    // Set baud rate
    cfsetospeed(&tty, B57600); // Output baud rate
    cfsetispeed(&tty, B57600); // Input baud rate

    // Configure serial port settings
    tty.c_cflag |= (CLOCAL | CREAD);  // Enable receiver, set local mode
    tty.c_cflag &= ~CSIZE;            // Clear data size mask
    tty.c_cflag |= CS8;               // 8 data bits
    tty.c_cflag &= ~PARENB;           // No parity
    tty.c_cflag &= ~CSTOPB;           // 1 stop bit
    tty.c_cflag &= ~CRTSCTS;          // No hardware flow control

    // Set input modes
    tty.c_iflag &= ~(IXON | IXOFF | IXANY);  // Disable software flow control
    tty.c_iflag &= ~(ICANON | ECHO | ECHOE | ISIG); // Raw input

    // Set output modes
    tty.c_oflag &= ~OPOST;  // Raw output

    // Set control modes
    tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);  // Raw mode

    // Apply the settings
    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        std::cerr << "Error setting port attributes" << std::endl;
        close(fd);
        return -1;
    }

    return fd;  // Return the file descriptor for the opened and configured serial port
}