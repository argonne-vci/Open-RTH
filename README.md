# Open-RTH

The Remote Test Harness is an SECC/EVCC emulator that allows testing high level communication protocol between EVSE and EV remotely. This includes DIN SPEC 70121 and all current and future versions of ISO 15118-2/-20 standards. It effectively "virtualizes" the CCS/NACS cable so that application layer changes can be tested in real-time even when the EV and EVSE are physically separated anywhere in the world, as long as internet connection is present.
This software requires two boards: one acting as an emulated EV that is connected to the EVSE DUT (device under test), and one acting as an emulated EVSE that is connected to the EV DUT. The two boards plug in to their respective DUTs via a CCS cable and inlet, establish a handshake with each other, perform SLAC to setup a logical network and relay all messages coming over the TCP port to each other via MQTT. The RTH can capture network data in PCAP files and save them for each session.

There are two caveats to using this device:

- It will always stop at or before a PreCharge message since the EV and EVSE are not physically connected to each other and no power transfer is actually taking place.
- It will only work on non-TLS encrypted sessions, since it is currently not designed to be a MitM (Man-in-the-Middle) device.

---
#### Notes
- This software in this repo is designed to work with the EVAcharge SE, and portions of this code are hardware specific; primarily the reading and setting of the PWM and Prox. This software may be adapted to work with other hardware devices by changing the appropriate sections of code.
- The software in this repo is designed to run on the default image which comes standard on the EVAcharge SE. As a result, this code is C++03 compatiable and requires a C++03 compatible compiler. There is a file in the docs/ folder which describes how to install the compiler which was used in the development of this software.
- There is a second file in the doc/ folder which describes how to properly setup the PLCs on the board. 
- This repo contains a [scripts_and_files/](scripts_and_files/) folder which contains the following items:
    - [__new_evse.pib__](scripts_and_files/new_evse.pib): The EVAcharge SE comes with an evse.pib file which may not properly configured for this software. This new_evse.pib file may be used instead to flash the PLC on the board which will act as an EVSE.
    - [__set_mode.py__](scripts_and_files/set_mode.py): This is a script which will properly configure the boards as an EVCC or an SECC. The EVAcharge SE will automatically revert the Pilot and Prox to their default settings after several minutes of not receiving a message via its onboard co-processor. As a result, this script has been added to the repository and may be run every time before starting Open-RTH to ensure each board is properly configured. It is run with an argument of "SECC", "EVCC", or "OFF".
- This software utilizes the following external libraries:
    - A custom implementation of [open-plc-utils](https://github.com/qca/open-plc-utils).
    - [jsoncpp](https://github.com/open-source-parsers/jsoncpp)
    - [wolfMQTT](https://github.com/wolfSSL/wolfMQTT)
    - [wolfSSL](https://github.com/wolfSSL/wolfssl)
- The PLC on the board acting as the EVSE must be force flashed. A quirk of the EVAcharge SE has been discovered which will not allow PLC communication between the EV and the board acting as the EVSE unless the PIB information of the EVSE is stored in non-volatile memory.

---
#### Dependencies

- This application requires the presence of an internet connection (via ethernet on the EVAcharge SE).

- _wolfMQTT_ and _wolfSSL_ libraries must be installed on the EVAcharge SE. All relavant static libraries (.a), shared objects (.so) are included [here](lib). A guide for how to correctly install these libraries onto the EVAcharge board can be found [here](docs/Transfer_Shared_Libraries.md).

- This application requires an MQTT broker which both EVAcharge SE boards can connect to.

---
#### Build

- To build, run `make` in a terminal from the source directory. This will build the application for the EVAcharge SE board. Run `make upload` to upload the app to the EVAcharge SE via SCP if the hostname and username are properly configured in the makefile.

---

#### Configure 

- A configuration file is necessary in order for the application to run. A sample configuration file is provided [here](src/config/rth.conf).

- The same application can be used for both boards, but they need to be separately configured as an EV or EVSE in the configuration file.

- The global MQTT broker and client configuration settings are also defined in the `rth.conf` configuration and should be modified as needed. The application publishes and subscribes to the topics listed in the config file. Do not alter these topics unless they are also updated in the application itself.

- The EVAcharge SE boards are placed inside their respective enclosures. Ensure that the RTH J1772 harness is properly connected to the EVSE enclosure, and the CCS inlet box is connected to the EV enclosure via BNC cables. These must include the control pilot, proximity pilot, and the ground lines for both.

- Ensure that the PLC chips on each board have their respective MAC addresses set and PIB files properly configured to be either an EVCC or an SECC by following [this guide](docs/PLC_Setup.md).

- Before starting the application, ensure that both EVAcharge SE modules are prepared and in the proper state by running the `set_mode.py` script located [here](scripts_and_files/set_mode.py). The EVAcharge SE will disable the PWM and Prox pins after a certain period of inactivity, so this script should be run again before starting the application if more than a few minutes have passed.

---

#### Setup and Execute

The application can run like any other executable by calling `sudo ./EVAcharge_rth_1.0.0.chb`. Ensure it is run using `sudo` as shown, as the application requires elevated permissions to function properly. The software utilizes a state machine to ensure proper sequence of events; when running, the output on the screen will indicate the current state of the application.

The procedure to run the application properly follows:

1. Ensure both devices are connected to the internet and unplugged from their respect DUTs.
2. Start the application on both devices simultaneously.
3. The apps will perform a network check, connect to the global broker and perform a handshake to ensure that both devices can communicate with each other over MQTT.
4. First, plug the RTH-EV into the EVSE DUT. The RTH should undergo SLAC with the EVSE and establish a logical network with the EVSE. They will then perform Service Discovery over UDP, and the RTH will connect to the EVSE's TCP server.
5. Next, plug the RTH-EVSE into the EV DUT. The RTH should undergo SLAC with the EV and establish a logical network with the EV. They will then perform Service Discovery over UDP, and the RTH will connect to the EVSE's TCP server.
6. Once the SDP is complete on both devices they will go into "remote" mode, where every _request_ message from the EV DUT that appears over the TCP socket of the RTH EVSE is relayed to the RTH EV over MQTT. The RTH EV will then transmit this message over its TCP socket to the EVSE DUT. The _resposne_ messages from the EVSE are transferred back to the EV DUT in the same manner.
7. The relaying process takes about 1-3 seconds and then stops. Once the test is complete, the application can exist by pressing `Ctrl + C`.
8. A `.pcap` file is saved localled within the timestamp and appended to it for reference. This file can then be transferred over to a host PC via SCP and opened using [Wireshark](https://www.wireshark.org/) for analysis. A plugin called [wireshark-v2g](https://github.com/ChargePoint/wireshark-v2g) by ChargePoint can be used to interpret the V2G messages captured in the `.pcap` file.

> **Note:** Ensure that only one instance of the application is running at a time. If the application crashes or hangs, make sure it is properly killed by running `sudo kill -9 $(ps aux | grep '[E]VAcharge_rth_0.10.0.chb' | awk '{print $2}')` where the square brackets `[E]` prevents grep from matching itself in the process.

---
