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
// State machine for Remote Test Harness
//==========================================================================================================

#pragma once

// All possible states for the RTH
enum rth_state_t
{
    INIT           = 0,
    NETWORK_CHECK  = 1,
    BROKER_CHECK   = 2,
    RTH_HANDSHAKE  = 3,
    UNPLUGGED_WAIT = 4,
    PLUGGED_IN     = 5,
    SDP            = 6,
    TCP_NETWORK    = 7,
    REMOTE         = 8,
    ERROR          = 9
};

// All possible handshake types
enum rth_handshake_t
{
    NO_HS    = 0,
    FIRST_HS = 1,
    BOTH_HS  = 2
};

// This function handles all states of the RTH and dispatches appropriate functions accordingly
int rth_state_machine();

//==========================================================================================================
