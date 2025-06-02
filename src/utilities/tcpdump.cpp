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
//==============================================================================

#include "tcpdump.h"
#include <cstdlib>
#include <ctime>

// This will be the file's extension
std::string file_extension = ".pcap";

//==============================================================================
// start() - Starts tcpdump with the given network interface and filename.
//           The function appends the current date and time, as well as .pcap extension
//==============================================================================
int TCPDump::start(const std::string& interface, const std::string& filename)
{
    // Get the current datetime
    time_t now = time(NULL);

    // Format the datetime properly
    char dt[100];
    std::strftime(dt, sizeof(dt), "%Y%m%d-%H%M%S", localtime(&now));
    
    // Create the filename with datetime and extension
    std::string full_filename = filename + "_" + dt + file_extension;

    // Check if both interface and filename are not empty
    if (!interface.empty() && !filename.empty())
    {
        // Construct the tcpdump command to run in the background
        std::string command = "tcpdump -i " + interface + " -w " + full_filename + " &";

        // Execute the command to start tcpdump process
        system(command.c_str());
        return 1;
    }

    // Names are empty, tell the caller
    else return 0;
}
//==============================================================================



//==============================================================================
// stop() - Stops the tcpdump process
//==============================================================================
void TCPDump::stop()
{
    // Construct the command to kill all tcpdump processes gracefully
    std::string command = "pkill -15 tcpdump";

    // Execute the command to stop tcpdump
    system(command.c_str());
}
//==============================================================================
