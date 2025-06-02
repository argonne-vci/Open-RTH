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
// tcpdump.h - Defines a class for capturing network activity using tcpdump
//==============================================================================
#pragma once

#include <string>

class TCPDump
{
public:
    
    // Destructor ensures that tcpdump is stopped when the object is destroyed
    ~TCPDump() {stop();}

    // Starts tcpdump with the given network interface and filename
    int start(const std::string& interface, const std::string& filename);

    // Stops the tcpdump process
    void stop();
};
//==============================================================================