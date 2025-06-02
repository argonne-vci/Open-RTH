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
// json.h - All JSON parsing related functions and variables defined here
//==========================================================================================================

#pragma once

#include <string>

#include "../lib/jsoncpp/json.h"

// Initialize all JSON documents and add default key-value pairs
extern void init_json();

// Function to update J1772 status member values
extern void update_J1772_status();

// Parses J1772 status into a struct
extern int parse_J1772_status(const std::string& J1772_status);

// Variable to hold JSON data represented in strings
extern std::string J1772_status_str;

//==========================================================================================================