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

#include <stdint.h>
#include <string>

#ifndef J1772_H_
#define J1772_H_

// According to the EVAcharge SE documentation, the resolution for control pilot voltage is 29 mV/bit
const double pilot_voltage_resolution = 0.029;

// Struct that defines PWM settings
struct pwm_setting_t
{
    std::string enable;
    std::string duty_cycle;
};

extern struct pwm_setting_t pwm_setting;

// Struct that defines all J1772 parameters
struct J1772_t
{
    double  Vpilot;                   // V
    double  Vpilot_min;               // V
    double  Vprox;                    // V
    double  pilot_duty_cycle;         // %
    int     pilot_freq;               // Hz
    int     pilot_state;              
    std::string    pilot_state_name;
    int     prox_state;
    int     pwm_comm_state;
};

extern struct J1772_t J1772, old_J1772;


enum prox_states
{
    UNPLUGGED         = 0,
    PLUGGED_S3_CLOSED = 1,
    PLUGGED_S3_OPENED = 2,
    PROX_UNKNOWN      = 3,
    PROX_NC           = 4
};

enum pilot_states
{
    A1 = 0,
    A2 = 1,
    B1 = 2,
    B2 = 3,
    C1 = 4,
    C2 = 5,
    D1 = 6,
    D2 = 7,
    E  = 8,
    F  = 9,
    UNKNOWN = 10
};

extern const char* pilot_state_names[];

enum pilot_min_states
{
    OK      = 0,
    FAIL    = 1,
    PWM_OFF = 2
};

// Pilot PWM communication states
enum pwm_comm_states
{
    DIGITAL = 0,
    ANALOG  = 1,
    INVALID = 2
};

// Function to measure pilot and prox pins on J1772 connector       
extern void sample_J1772();

// Function to enable or disable PWM. Pass a 1 to enable, pass a 0 to disable
extern void control_pwm(int on_or_off);

// Function to set the duty cycle of the PWM
extern void set_pwm(double duty_cycle);


#endif
// -----------------------------------------------------------------------------