timonel-i2cmms
==============

This is a demo of the Timonel TWI Master libraries for Arduino. It shows how to use the libraries to control and update firmware on several ATTiny85 connected to a TWI bus. The ATTiny85s run the Timonel bootloader.

NbTinyX5 library
----------------
Provides the functionality to communicate with an ATTiny85 that implements the NB protocol through a TWI bus.

TimonelTwiM library
-------------------
It inherits from the NbTinyX5 class. Provides specific commands to control and upload application firmware to an ATTiny85 running the Timonel bootloader.
