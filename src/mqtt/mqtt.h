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
// mqtt.h - All MQTT related functions and variables defined here
//==========================================================================================================

#pragma once

#include "wolfMQTT_cpp.h"

// -----------------------------------------------------------------------------
// This is a wrapper around the wolf MQTT library with our application-specific message handler
// -----------------------------------------------------------------------------
class CWolfMQTT : public CWolfMQTTBase
{
    public:
        CWolfMQTT() : CWolfMQTTBase() {}

    protected:
        // Application-specific message handler. Application should probably never call this directly
        void handle_message(const std::string& topic, const std::string& message);
};
// -----------------------------------------------------------------------------

//==========================================================================================================
