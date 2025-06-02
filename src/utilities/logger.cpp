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


//==============================================================================
// logger.cpp - Implements a convenient logger for apps to log to a file
//==============================================================================

#include <ctime>
#include <iomanip>
#include <fstream>
#include <unistd.h>
#include <iostream>
#include <cstring>

#include "logger.h"

//==============================================================================
// init() - Initialize the logger with an app name and filename
//==============================================================================
void CLogger::init(const char* app_name, const char* filename)
{
    // If init has been called previously, do nothing
    if(m_is_initialized) return;

    // Set the app name and filename
    this->m_app_name = app_name;
    this->m_filename = filename;

    // Check if user wants to save to syslog
    if (strcmp(filename, "syslog") == 0) m_is_syslog = true;

    // Set the init flag
    m_is_initialized = true;
}
//==============================================================================



//==============================================================================
// log() - Save a message to log file with a log level
//==============================================================================
void CLogger::log(int log_level, const char* message)
{
    // Check if user wants to save to syslog
    if (m_is_syslog)
    {
        openlog(m_app_name, LOG_PID, LOG_USER);
        syslog(log_level, "%s", message);
        closelog();
    }

    // Otherwise, save to specified filename
    else
    {
        // Create a format for datatime
        const char* dt_format = "%b %d %H:%M:%S";

        // Get current datetime
        time_t now = time(NULL);
        tm* localTime = localtime(&now);
        
        // Get current process ID
        int pid = getpid();

        // Open the log file in append mode
        std::ofstream log_file(m_filename, std::ios::app);

        // Write to file when it is ready
        if (log_file.is_open())
        {
            // Format and write datetime to log
            char datetime_str[100];
            strftime(datetime_str, sizeof(datetime_str), dt_format, localTime);
            log_file << datetime_str << " ";
            
            // Write the app name and process ID to log
            log_file << m_app_name << "[" << pid << "]: ";


            // Write log level in color to log
            switch (log_level)
            {
                case LOG_INFO:
                    log_file << color.white << "[INFO]  ";
                    break;
                case LOG_DEBUG:
                    log_file << color.yellow << "[DEBUG] ";
                    break;
                case LOG_WARNING:
                    log_file << color.bold_yellow << "[WARN]  ";
                    break;
                case LOG_ERR:
                    log_file << color.bold_red << "[ERROR] ";
                    break;
                default:
                    log_file << color.white << "[INFO]  ";
                    break;
            }

            // Reset color to default
            log_file << color.reset;

            // Write the log message
            log_file << message << std::endl;

            // Close the file
            log_file.close();
        }
        
        else
        {
            // Handle file opening failure
            std::cerr << "Error opening history.log for writing." << std::endl;
        }
    }
}
//==============================================================================