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
// json.cpp - All JSON parsing related functions and variables implemented here
//==========================================================================================================

#include <cstring>
#include <iostream>
#include <stdio.h>

#include "json.h"
#include "J1772.h"

// Create JSON object for the structs we care about
Json::Value J1772_status;

// Variable to hold the JSON data represented in strings
std::string J1772_status_str;

// -----------------------------------------------------------------------------
// init_json() - Initialize all JSON documents and add default key-value pairs
// -----------------------------------------------------------------------------
void init_json()
{    
    // Initialize the JSON object
    J1772_status.clear();  // Clear in case it's reused

    // Add member variables with default values to the JSON object
    J1772_status["Vpilot"] = 0;
    J1772_status["Vpilot_min"] = 0;
    J1772_status["Vprox"] = 0;
    J1772_status["pilot_duty_cycle"] = 0;
    J1772_status["pilot_freq"] = 0;
    J1772_status["pilot_state_name"] = "A1"; 
}
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// json_to_string() - Function to convert JSON object to string
// -----------------------------------------------------------------------------
std::string json_to_string(const Json::Value& json_obj)
{
    Json::StreamWriterBuilder writer;
    writer["indentation"] = "";  // Disable pretty-printing if necessary
    std::string json_str = Json::writeString(writer, json_obj);
    return json_str;
}
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// update_J1772_status() - Function to update J1772 status member values
// -----------------------------------------------------------------------------
void update_J1772_status()
{
    // Update the JSON object with data from the J1772 struct
    J1772_status["Vpilot"] = J1772.Vpilot;
    J1772_status["Vpilot_min"] = J1772.Vpilot_min;
    J1772_status["Vprox"] = J1772.Vprox;
    J1772_status["pilot_duty_cycle"] = J1772.pilot_duty_cycle;
    J1772_status["pilot_freq"] = J1772.pilot_freq;
    J1772_status["pilot_state_name"] = J1772.pilot_state_name;

    // Convert the JSON object to string
    J1772_status_str = json_to_string(J1772_status);
}
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// parse_J1772_status() - Parses J1772 status message into a struct
// -----------------------------------------------------------------------------
int parse_J1772_status(const std::string& J1772_status_msg)
{
    Json::Reader reader;

    // Parse the command and exit if there's an error in parsing
    if(!reader.parse(J1772_status_msg, J1772_status))
    {
        printf("Error: Failed to parse JSON\n");
        return -1;
    }

    // Helper macros for parsing numeric fields
    #define PARSE_DOUBLE_FIELD(jsonKey, structField)                      \
        if (J1772_status.isMember(jsonKey) && J1772_status[jsonKey].isNumeric()) \
            J1772.structField = J1772_status[jsonKey].asDouble();          \
        else { printf("Error: Missing or invalid %s\n", jsonKey); return -1; }

    // Helper macro for parsing integer fields
    #define PARSE_INT_FIELD(jsonKey, structField)                          \
    if (J1772_status.isMember(jsonKey) && J1772_status[jsonKey].isInt()) \
        J1772.structField = J1772_status[jsonKey].asInt();             \
    else { printf("Error: Missing or invalid %s\n", jsonKey); return -1; }

    // Helper macro for parsing string fields
    #define PARSE_STRING_FIELD(jsonKey, structField)                      \
        if (J1772_status.isMember(jsonKey) && J1772_status[jsonKey].isString()) \
            J1772.structField = J1772_status[jsonKey].asString();         \
        else { printf("Error: Missing or invalid %s\n", jsonKey); return -1; }

    // Helper macro for parsing boolean fields
    #define PARSE_BOOL_FIELD(jsonKey, structField)                        \
        if (J1772_status.isMember(jsonKey) && J1772_status[jsonKey].isBool()) \
            J1772.structField = J1772_status[jsonKey].asBool();           \
        else { printf("Error: Missing or invalid %s\n", jsonKey); return -1; }

    // Update the J1772 struct values appropriately
    PARSE_DOUBLE_FIELD("Vpilot", Vpilot);
    PARSE_DOUBLE_FIELD("Vpilot_min", Vpilot_min);
    PARSE_DOUBLE_FIELD("Vprox", Vprox);
    PARSE_DOUBLE_FIELD("pilot_duty_cycle", pilot_duty_cycle);

    // Parse integer fields
    PARSE_INT_FIELD("pilot_freq", pilot_freq);

    PARSE_STRING_FIELD("pilot_state_name", pilot_state_name);

    // Set the pilot_state variable based on the pilot_state_name
    if (J1772.pilot_state_name == "A1") {
        J1772.pilot_state = A1;
    } else if (J1772.pilot_state_name == "A2") {
        J1772.pilot_state = A2;
    } else if (J1772.pilot_state_name == "B1") {
        J1772.pilot_state = B1;
    } else if (J1772.pilot_state_name == "B2") {
        J1772.pilot_state = B2;
    } else if (J1772.pilot_state_name == "C1") {
        J1772.pilot_state = C1;
    } else if (J1772.pilot_state_name == "C2") {
        J1772.pilot_state = C2;
    } else if (J1772.pilot_state_name == "D1") {
        J1772.pilot_state = D1;
    } else if (J1772.pilot_state_name == "D2") {
        J1772.pilot_state = D2;
    } else if (J1772.pilot_state_name == "E") {
        J1772.pilot_state = E;
    } else if (J1772.pilot_state_name == "F") {
        J1772.pilot_state = F;
    } else {
        J1772.pilot_state = UNKNOWN;
    }


    // If we get here, parsing is successful
    return 0;
}
// -----------------------------------------------------------------------------


//==========================================================================================================
