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
// logger.h - Defines a convenient logger for apps to log to a file
//==============================================================================
#pragma once

#include <syslog.h>

class CLogger
{
public:

    // Call this at startup to initialize logger settings
    void init(const char* app_name, const char* filename);

    // Save a message to log file with a log level
    void log(int log_level, const char* message);

protected:

    // Variables to store app and logfile name
    const char* m_app_name;
    const char* m_filename;

    // Flag to track whether init has already been called
    bool m_is_initialized, m_is_syslog;

    // Define a struct for text color and formatting
    struct color_t
    {
        const char* reset;
        const char* black;
        const char* red;
        const char* green;
        const char* yellow;
        const char* blue;
        const char* magenta;
        const char* cyan;
        const char* white;

        const char* bold_black;
        const char* bold_red;
        const char* bold_green;
        const char* bold_yellow;
        const char* bold_blue;
        const char* bold_magenta;
        const char* bold_cyan;
        const char* bold_white;

        const char* bold;
        const char* dim;
        const char* underline;
        const char* blink;
        const char* reverse;
        const char* hidden;

        // Constructor for color_t to initialize color codes
        color_t()
            : reset("\033[0m"),
              black("\033[30m"),
              red("\033[31m"),
              green("\033[32m"),
              yellow("\033[33m"),
              blue("\033[34m"),
              magenta("\033[35m"),
              cyan("\033[36m"),
              white("\033[37m"),
              bold_black("\033[1;30m"),
              bold_red("\033[1;31m"),
              bold_green("\033[1;32m"),
              bold_yellow("\033[1;33m"),
              bold_blue("\033[1;34m"),
              bold_magenta("\033[1;35m"),
              bold_cyan("\033[1;36m"),
              bold_white("\033[1;37m"),
              bold("\033[1m"),
              dim("\033[2m"),
              underline("\033[4m"),
              blink("\033[5m"),
              reverse("\033[7m"),
              hidden("\033[8m")
        {}
    } color;
};
//==============================================================================