# hubitat-arduino-relay
Arduino sketch for cheap arduino + Ethernet shield + relay board to be controlled via TCP

Currently the arduino sketch broadcasts its IP address every 10s on UDP port 8981.
Eventually it will use SSDP (which is supported by the Hubitat driver framework).
The default MAC address should give the arduino an address WizNetA1DE07. To run multiple
arduino relays on the same network it will be necessary to assign different MAC
addresses.

The RelayControlTest is a simple cross-platform Qt GUI application that listens
for the UDP broadcast and communicates with the Arduino relay via TCP.  Both
momentary-contact and close / open methods are available for each relay (for
example, garage door switch might use 2 for a 250ms closure).
