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


 #pragma once

class CSLACify
{

public:

    // Call this to initialize SLAC based on device type and SLAC settings
    int init(int device_type, signed slac_setting = 'n', int slac_limit = 40, int timeout = 20, int retries = 3);

    // Call this to attempt SLAC and creating a logical network
    int connect();

    // Call this to disconnect an established AVLN
    void close();

protected:
    int m_device_type;
    int m_setting;
};

enum device_type_t
{
    EVSE = 0,
    PEV  = 1
};

//==========================================================================================================