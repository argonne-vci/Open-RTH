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


// =============================================================================
// Measure J1772 pilot and prox voltages and get states
// =============================================================================

#include <array>
#include <iostream>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <vector>

#include "common.h"
#include "main.h"

#define CH_PILOT           0
#define CH_PROX            1

// Initialize the J1772 struct with default values
struct J1772_t J1772 =
{
    0,
    0,
    0,
    0,
    0,
    A1,
    "A1",
#ifdef EVCC
    PROX_UNKNOWN,
#else
    PROX_NC,
#endif
    false
};

// Initialize the pwm setting struct with default values
struct pwm_setting_t pwm_setting = 
{
    "off",
    "5.0"
};

// Save the contents of the J1772 struct to compare to later
struct J1772_t old_J1772 = J1772;

// Define the state names
const char* pilot_state_names[] =
{"A1", "A2", "B1", "B2", "C1", "C2", "D1", "D2", "E", "F"};


// Store last pilot state (hack)
int8_t last_pilot_state;

// -----------------------------------------------------------------------------
// decimal_to_little_endian_hex - Converts an int value to 4 bytes of little endian hex
// -----------------------------------------------------------------------------
std::vector<unsigned char> decimal_to_little_endian_hex(int value)
{
    // Create a vector to store the 2 bytes (little-endian format)
    std::vector<unsigned char> little_endian_bytes(2);

    // Convert the integer to little-endian format by extracting each byte
    little_endian_bytes[0] = static_cast<unsigned char>(value & 0xFF); // LSB
    little_endian_bytes[1] = static_cast<unsigned char>((value >> 8) & 0xFF); // MSB

    return little_endian_bytes;
}
// -----------------------------------------------------------------------------

// Function to convert a signed 16-bit hex value to a double using 2's complement
int hex_to_signed_decimal(unsigned short hex_value)
{
    // Check if the value is signed (bit 15 is set)
    if(hex_value & 0x800)
    {
        // Negative value: apply two's complement
        return static_cast<int>(hex_value - 0x10000);  // Convert to negative value using two's complement
    } else {
        // Positive value, simply cast it to double
        return static_cast<int>(hex_value);
    }
}
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// read_pilot_voltage - read the hi and lo pilot voltage values
//  this function is void but an array is passed by reference to represent the pilot values
// -----------------------------------------------------------------------------
void read_pilot_voltage()
{
    // The message to get the pilot voltage is 0x02 0x03 0x00 0x14
    // Prepare data
    std::vector<unsigned char> uart_msg;
    uart_msg.push_back(0x02);   // Start byte
    uart_msg.push_back(0x03);   // Message length
    uart_msg.push_back(0x00);
    uart_msg.push_back(0x14);

    // Calculate the BCC and append it to the message
    uart_msg.push_back(calculate_bcc(uart_msg));

    // Send the data
    write_to_serial(uart_fd, uart_msg);

    // Allow some time for the device to respond
    sleeper.sleep(config.response_delay_ms);

    // Process incoming messages
    std::vector<std::vector<unsigned char> > messages;
    read_serial_data(uart_fd, messages);

    // Parse the first valid response message
    unsigned short pos_pilot_voltage_hex = -99;
    unsigned short neg_pilot_voltage_hex = -99;
    for(size_t i = 0; i < messages.size(); ++i)
    {
        const std::vector<unsigned char>&msg = messages[i];
        // Validate header and length
        if(msg.size() >= 9 && msg[0] == 0x02 && msg[1] == 0x07 && msg[3] == 0x94)
        {
            // Parse the hex data
            pos_pilot_voltage_hex = (msg[5] << 8) | msg[4];
            neg_pilot_voltage_hex = (msg[7] << 8) | msg[6];

            // Convert the values to doubles and store in array
            J1772.Vpilot = static_cast<double>(hex_to_signed_decimal(pos_pilot_voltage_hex)) * pilot_voltage_resolution;   // positive pilot voltage
            J1772.Vpilot_min = static_cast<double>(hex_to_signed_decimal(neg_pilot_voltage_hex)) * pilot_voltage_resolution;   // negative pilot voltage

            break;
        }

    }
    
}
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// read_prox_voltage - read the hi and lo pilot voltage values
//  this function is void but an array is passed by reference to represent the pilot values
// -----------------------------------------------------------------------------
void read_prox_voltage()
{
    // The message to get the pilot voltage is 0x02 0x03 0x00 0x52
    // Prepare data
    std::vector<unsigned char> uart_msg;
    uart_msg.push_back(0x02);
    uart_msg.push_back(0x03);
    uart_msg.push_back(0x00);
    uart_msg.push_back(0x52);

    // Calculate the BCC and append it to the message
    uart_msg.push_back(calculate_bcc(uart_msg));

    // Send the data
    write_to_serial(uart_fd, uart_msg, false);

    // Allow some time for the device to respond
    sleeper.sleep(config.response_delay_ms);

    // Read response from the serial port
    std::vector<std::vector<unsigned char>> messages;

    // Read data from the serial port and extract messages
    read_serial_data(uart_fd, messages);

    // Parse the hex data
    unsigned short prox_voltage_hex = 0;
    double prox_voltage = 0;
    for(size_t i = 0; i < messages.size(); ++i)
    {
        const std::vector<unsigned char>& msg = messages[i];
        // Validate header and length
        if(msg.size() >= 7 && msg[0] == 0x02 && msg[1] == 0x05 && msg[3] == 0xD2)
        {

            // Parse the prox voltage hex data
            prox_voltage_hex = (msg[5] << 8) | msg[4];

            // Convert the value to a double and store it in the J1772 structure
            prox_voltage = static_cast<double>(hex_to_signed_decimal(prox_voltage_hex)) * pilot_voltage_resolution;
            J1772.Vprox = prox_voltage;

            // Only one message is expected, so break the loop here
            break;
        }
    }

}
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// get_actual_voltage - Read actual voltage from a given channel
// -----------------------------------------------------------------------------
void get_actual_voltage(uint8_t channel)
{       
    switch (channel)
    {	
        // If sampling Pilot pin, need to find high and low values
        case CH_PILOT:
        {   
            read_pilot_voltage();

            // Update J1772 with the new values
            // Round the readings to 3 decimal points
            J1772.Vpilot = floor(J1772.Vpilot * 1000.0) / 1000.0;
            J1772.Vpilot_min = floor(J1772.Vpilot_min * 1000.0) / 1000.0;

            break;
        }

        // If sampling Prox pin, simply measure voltage
        case CH_PROX:
        {
            // First get the raw ADC voltage
            read_prox_voltage();

            // Round the readings to 3 decimal points
            J1772.Vprox = round(J1772.Vprox * 1000.0) / 1000.0;

            break;
        }

        default:

            logger.log(LOG_WARNING, "Invalid channel provided (get_actual_voltage)");
            // return -1;
            break;
    }
}
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// get_pilot_state - This function will measure pilot high and low voltage 
//                   and calculate pilot state based on the voltages
// -----------------------------------------------------------------------------  
uint8_t get_pilot_state()
{
    // Fetch the pilot voltages
    get_actual_voltage(CH_PILOT);

    // Save voltages we just fetched
    double high = J1772.Vpilot;
    double low  = J1772.Vpilot_min;
    int state;

    // define a state for the control minimum voltage. 
    // true indicates a good negative voltage reading, false indicates a bad negative voltage reading
    bool low_ok = (low > -13 && low <= -10);

    // check control high voltage and set the control state accordingly
    if (high >= 11.40 && high <= 12.60) state = low_ok ? A2 : A1;
    else if (high >=  8.36 && high <=  9.56) state = low_ok ? B2 : B1;
    else if (high >=  5.48 && high <=  6.49) state = low_ok ? C2 : C1;
    else if (high >=  2.62 && high <=  3.25) state = low_ok ? D2 : D1;
    else if (high >=  0    && high <=  0.25) state = low_ok ? A1 : F;

    // If we get here, something is wrong; use the last known good state (hack)
    else state = last_pilot_state;

    // copy contents of the actual name into J1772 struct
    J1772.pilot_state_name = pilot_state_names[state];

    // return pilot state
    return state;
}
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// get_prox_state - This function will measure prox voltage and calculate state
// -----------------------------------------------------------------------------  
uint8_t get_prox_state()
{
    // Fetch the prox voltage
    get_actual_voltage(CH_PROX);

    // Save voltage we just fetched
    double prox = J1772.Vprox;
    int state;

    // check prox voltage and set the state accordingly
    if (prox >= 0.80 && prox <= 1.72) state = PLUGGED_S3_CLOSED;
    else if (prox >= 2.51 && prox <= 3.01) state = PLUGGED_S3_OPENED;
    else if (prox >= 4.20 && prox <= 4.65) state = UNPLUGGED;
    else if (prox >= 0.00 && prox <= 0.50) state = PROX_UNKNOWN;
    else state = PROX_UNKNOWN;

    // return prox state
    return state;
}
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// get_pwm_values - read the hi and lo pilot voltage values
//  this function is void but pass two values by reference:
//      frequency   :   this is the frequency of the PWM
//      duty cycle  :   this is the duty cycle of the PWM
// -----------------------------------------------------------------------------
void get_pwm_values()
{
    // The message to get the pilot voltage is 0x02 0x03 0x00 0x14
    // Prepare data
    std::vector<unsigned char> uart_msg;
    uart_msg.push_back(0x02);
    uart_msg.push_back(0x03);
    uart_msg.push_back(0x00);
    uart_msg.push_back(0x10);

    // Calculate the BCC and append it to the message
    uart_msg.push_back(calculate_bcc(uart_msg));

    // Send the data
    write_to_serial(uart_fd, uart_msg);

    // Allow some time for the device to respond
    sleeper.sleep(config.response_delay_ms);

    // Read response from the serial port
    std::vector<std::vector<unsigned char>> messages;
    read_serial_data(uart_fd, messages);

    // Parse and handle each message
    unsigned short frequency_hex = -9999;
    unsigned short duty_cycle_hex = -9999;
    for(size_t i = 0; i < messages.size(); ++i)
    {
        const std::vector<unsigned char>& msg = messages[i];

        // Validate header and legnth
        if(msg.size() >= 9 && msg[0] == 0x02 && msg[1] == 0x07 && msg[3] == 0x90)
        {
            frequency_hex = (msg[5] << 8) | msg[4];
            duty_cycle_hex = (msg[7] << 8) | msg[6];

            J1772.pilot_freq = static_cast<int>(hex_to_signed_decimal(frequency_hex));
            J1772.pilot_duty_cycle = static_cast<double>(hex_to_signed_decimal(duty_cycle_hex)) / 10.0;

            // We're only expecting one message, so break the loop
            break;
        }

    }
    
}
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// get_pwm_comm_state - Measure the PWM duty cycle and
//                      1. Figure out if it's analog or digital comms
//                      2. Set the pilot oscillator state
// -----------------------------------------------------------------------------  
uint8_t get_pwm_comm_state()
{
    // First measure the frequency and duty cycle
    get_pwm_values();

    // Save the duty cycle we just fetched
    int frequency = J1772.pilot_freq;
    double duty_cycle = J1772.pilot_duty_cycle;
    
    // check the duty cycle and set the PWM comm state accordingly
    int comm_state;
    if (duty_cycle >= 3 && duty_cycle < 8) comm_state = DIGITAL;
    else if (duty_cycle >= 8 && duty_cycle <= 98) comm_state = ANALOG;
    else comm_state = INVALID;
    
    // set the pilot oscillator state
    bool osc_state;
    if (comm_state == DIGITAL || comm_state == ANALOG) osc_state = true;
    else osc_state = false;

    // return pwm comm state
    return osc_state;
}
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// control_pwm - Function to enable or disable PWM. Pass a 1 to enable, pass a 0 to disable  
// ----------------------------------------------------------------------------- 
void control_pwm(int on_or_off)
{
    if( (on_or_off != 0) && (on_or_off != 1) ) // check if on_or_off is an invalid value
    {
        std::cerr << "Error: Invalid argument passed to control_pwm()" << std::endl;
        return;
    }
    // The message to control pwm is 0x02 0x07 0x00 0x12 [Control Code] [BCC]
    //  where [Control Code] is on_or_off
    std::vector<unsigned char> uart_msg;
    uart_msg.push_back(0x02);
    uart_msg.push_back(0x04);
    uart_msg.push_back(0x00);
    uart_msg.push_back(0x12);
    
    uart_msg.push_back(on_or_off);
    
    // Calculate the BCC and add it to the message
    uart_msg.push_back(calculate_bcc(uart_msg));

    // Send the data
    write_to_serial(uart_fd, uart_msg);

    // Allow some time for the device to respond
    sleeper.sleep(config.response_delay_ms);

    // Read response from the serial port
    std::vector<std::vector<unsigned char>> messages;
    read_serial_data(uart_fd, messages);
}
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// set_pwm - Set PWM frequency and duty cycle 
// ----------------------------------------------------------------------------- 
void set_pwm(double duty_cycle)
{
    // Sanity check to make sure new duty cycle is within reason
    if(duty_cycle < 0 || duty_cycle > 99.999) 
    {
        std::cerr << "Invalid duty cycle passed to set_pwm()" << std::endl;
        return;
    }

    // Multiply the duty cycle by 10 for the EVAcharge_SE
    // because a 50% duty cycle, for example, on the EVAcharge_SE,
    // is represented with "500"
    duty_cycle *= 10;

    // The message to control pwm is 0x02 0x07 0x00 0x12 [Control Code] [BCC]
    //  where [Control Code] is on_or_off
    std::vector<unsigned char> uart_msg;
    uart_msg.reserve(9);
    uart_msg.push_back(0x02);
    uart_msg.push_back(0x07);
    uart_msg.push_back(0x00);
    uart_msg.push_back(0x11);
    
    // Set the frequency at 1000 Hz. Could become a variable setting if desired
    uart_msg.push_back(0xE8);
    uart_msg.push_back(0x03);
    
    // Convert duty cycle to hex and add to uart_msg
    std::vector<unsigned char> duty_cycle_hex = decimal_to_little_endian_hex(duty_cycle);
    uart_msg.insert(uart_msg.end(), duty_cycle_hex.begin(), duty_cycle_hex.end());

    // Calculate the BCC and add it to the message
    uart_msg.push_back(calculate_bcc(uart_msg));

    // Send the data
    write_to_serial(uart_fd, uart_msg);

    // Allow some time for the device to respond
    sleeper.sleep(config.response_delay_ms);

    // Read response from the serial port
    std::vector<std::vector<unsigned char>> messages;
    read_serial_data(uart_fd, messages);
}
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// sample_J1772 - Measure pilot and prox pins on J1772 connector    
// -----------------------------------------------------------------------------   
void sample_J1772()
{
    // Measure duty cycle and get PWM state
    get_pwm_comm_state();

    // Measure Pilot pin voltage and get the state
    J1772.pilot_state    = get_pilot_state();

    // If the pilot state is different from last time, measure again to be sure
    if (J1772.pilot_state != last_pilot_state)
    {
        J1772.pilot_state = get_pilot_state();

        // Save the pilot state to compare with later (hack)
        last_pilot_state = J1772.pilot_state;
    }

    // Measure the proximity pin voltage and get the state
    #ifdef EVCC
        J1772.prox_state = get_prox_state();
    #endif
}
// -----------------------------------------------------------------------------

