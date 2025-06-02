import time
import serial
import sys

# Method to calculate the BCC and append it to hex_data
def calc_BCC(hex_data):
    BCC = 0
    for byte in hex_data:
        BCC ^= byte
    return BCC
######################################################

# Method to send HEX data accross UART
def send_data(hex_data):
    print("Sending: ", end='')
    for byte in hex_data:
        print('{:02X}'.format(byte), end=' ')
    print()

    # Send the data to serial
    ser.write(bytes(hex_data))
    return
#############################################

def read_data():
    # Read data from the serial port
    raw_out = ser.readline()
    # Convert the raw data into hex values
    hex_out = ' '.join(['{:02X}'.format(byte) for byte in raw_out])
    print(hex_out)
    return
#############################################

def decimal_to_little_endian_hex(value):
    # Convert the float to an integer
    value = int(value)
    # Convert the decimal value to bytes, using little-endian byte order
    byte_value = value.to_bytes(2, byteorder='little', signed=False)

    # Convert to list of byte values and return it
    return list(byte_value)
############################################
############################################
############################################
############################################

# configure the serial connections (the parameters differs on the device you are connecting to)
ser = serial.Serial(
    port='/dev/ttyAPP2', #'/dev/ttyAPP0',
    baudrate=57600, #115200,
    parity=serial.PARITY_NONE,
    stopbits=serial.STOPBITS_ONE,
    bytesize=serial.EIGHTBITS,
    timeout=1
)

ser.isOpen()
ser.flushInput()


# Command to enable or disable the proximity pilot resistor
enable_prox_resistor = [0x02, 0x04, 0x00, 0x50, 0x00] # set to 2700 Ohms
disable_prox_resistor = [0x02, 0x04, 0x00, 0x50, 0x07]

# Command to enable the pullup resistor at the proximity signal
enable_prox_pullup = [0x02, 0x04, 0x00, 0x51, 0x01]
disable_prox_pullup = [0x02, 0x04, 0x00, 0x51, 0x00]

# Command to enable to disable the pilot
enable_pilot = [0x02, 0x04, 0x00, 0x12, 0x01]
disable_pilot = [0x02, 0x04, 0x00, 0x12, 0x00]

# Command to set the EVCC pilot state (different than datasheet)
StateA = [0x02, 0x04, 0x00, 0x15, 0x00] #no resistors
StateB = [0x02, 0x04, 0x00, 0x15, 0x01] #2.7k
StateC = [0x02, 0x04, 0x00, 0x15, 0x03] # 2.7k ohm || 1.3kohm = 888 ohm

#SECC command to enable StateA (12V 100% dutycycle)
secc_StateA = [0x02, 0x07, 0x00, 0x11, 0xE8, 0x03, 0xE8, 0x03]


# Read the argument that was passed with the program call
#    If there was not one, print out the usage
if len(sys.argv) > 1:   # Get the command-line argument
    mode = sys.argv[1].strip().upper()
else:
    print("Usage: {} <mode> where the mode options are SECC, EVCC, and OFF".format(sys.argv[0]))

# If mode is SECC enable the pilot, disable prox
if mode == "SECC":
    # Disable the prox
    disable_prox_resistor.append(calc_BCC(disable_prox_resistor))
    disable_prox_pullup.append(calc_BCC(disable_prox_pullup))
    send_data(disable_prox_pullup)
    read_data()
    send_data(disable_prox_resistor)
    read_data()

    # Enable the pilot
    enable_pilot.append(calc_BCC(enable_pilot))
    send_data(enable_pilot)
    read_data()    

    # Enable State A
    secc_StateA.append(calc_BCC(secc_StateA))
    send_data(secc_StateA)
    read_data()
    exit()


elif mode == "EVCC": 
    # Enable the prox
    enable_prox_resistor.append(calc_BCC(enable_prox_resistor))
    enable_prox_pullup.append(calc_BCC(enable_prox_pullup))    
    send_data(enable_prox_pullup)
    read_data()

    # Disable pilot
    disable_pilot.append(calc_BCC(disable_pilot))
    send_data(disable_pilot)
    read_data()

    #Set Pilot State
    StateB.append(calc_BCC(StateB))
    send_data(StateB)
    read_data()

    exit()
elif mode == "OFF":    # Disable prox and pilot
    # Disable the prox
    disable_prox_resistor.append(calc_BCC(disable_prox_resistor))
    disable_prox_pullup.append(calc_BCC(disable_prox_pullup))
    send_data(disable_prox_pullup)
    read_data()
    send_data(disable_prox_resistor)
    read_data()

    # Disable the pilot
    disable_pilot.append(calc_BCC(disable_pilot))
    send_data(disable_pilot)
    read_data()

    # Disable EVCC resistors
    StateA.append(calc_BCC(StateA))
    send_data(StateA)
    read_data()
    exit()
else:
    print("Usage: {} <mode> where the mode options are SECC, EVCC, and OFF".format(sys.argv[0]))
    exit()

 
