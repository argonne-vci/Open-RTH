## Static IPv4 Address
The chargebyte boards have a factory preset static ipv4 address for the qca0. This address is set in in `/etc/network/interfaces`. This could potentially cause an issue, so on one of the boards, in `/etc/network/interfaces`, change the line `address 192.168.66.2` to a different address, like `192.168.66.3`

You can also temporarily change the ip address manually by running the command `
```
sudo ifconfig qca0 192.168.66.3 netmask 255.255.255.0
```

## Flashing PLC
The EVAcharge SE comes with a script to flash the PLCs pre-installed. It is found at `/root/flash_qca7000_temporarily`. The script will flash the PLCs until they are power cycled. The qca0 interface of the EVSE must be force flashed in order to work properly. This can be done by using the -FF flag at the end of the `flash_qca7000_temporarily.sh` script. 
```
/usr/local/bin/plcboot -N "$FW" -P "$PIB" -S "$BL" -i qca0 -FF
```
This requires open-plc-utils to be installed on the board. 

There is an evse.pib file located in the `/root` directory of the board by default, but in testing it did not work correctly. In the `scripts_and_files` directory of this repository there is a `new_evse.pib` file. It is suggested that you replace the default evse.pib file with this one.

- Get the MAC address of your qca0 interface
```
sudo ifconfig qca0
```

The MAC address is the six hex bytes that followed `HWaddr`, i.e.: `HWaddr 00:01:87:32:05:77`

- Edit the MAC Address stored in the PIB file using the command:
```
sudo modpib -M 00:01:87:32:05:77 -v evse.pib
```
Replace _00:01:87:32:05:77_ with the MAC address of your PLC found in the previous step

- If desired, edit the `flash_qca7000_temporarily.sh` script to permanently flash the PLC using the -FF flag in the final line of the script. This is seems to be required for a board which will be setup as an EVSE, but not required for a board which will be setup as a PEV. If only a temporary flash is desired, ensure that the script is not using the -FF flag.
```
/usr/local/bin/plcboot -N "$FW" -P "$PIB" -S "$BL" -i qca0 -FF
```

- Power cycle the board. Leave it powered off for at least 10 minutes before powering back on
- **EVSE only:** After booting the board, verify that the board was flashed in non-volatile memory using the command
```
sudo plctool -i qca0 -v -P new_evse.pib 00:01:87:32:05:77
```
Replace _00:01:87:32:05:77_ with the MAC address for your board
