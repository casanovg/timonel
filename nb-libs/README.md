# NB Libraries v1.3 #

cmd folder:
-----------
Contains the NB command set definition, common for both, the TWI (I2C) master and slave devices.

It contains the definition of the NB command set, common for the slave and master TWI (I2C) devices. Note that any change in the "nb-twi-cmd.h" file implies the need to recompile both firmware. If only one of the parties is compiled with a new revision, the communication will surely fail. This is especially important when sending a bootloader update to a slave device. 

The correct sequence for those cases is:

1. Compile the bootloader with a new command set revision

2. Convert it into a compilable payload for the TWI Master by using "timonel-updater".

3. Compile and flash the TWI master with the bootloader updater payload taking care of using the old command set. The objective of this compilation is adding the updater payload to the master, not to update the command set running on it (yet). Up to this moment, the old command set is still in use in both sides.

4. Use the master's "UploadApplication" function to load the updater payload to the TWI slave device, if the upload is successful, run it with the "RunApplication" method. From this point on, the TWI slave is already running the new command set, while the TWI master still runs the old one.

5. Compile and flash the TWI master with the new command set and reinitialize it.

If all went well, now both TWI devices share the new command set, the communication is reestablished.

twim:
-----
It has two TWI master libraries compatible with Arduino: "NbMicro" and "TimonelTWIM".

The NbMicro library is in charge of basic communication with slave devices that implement the NB command set. The NbMicro class is the one from which any higher level application should inherit.

The TimonelTWIM library inherits from NbMicro and implements functions to control a TWI slave device that runs Timonel bootloader.

twis:
-----
TWI slave driver libraries: "nb-usitwisl" is a USI-based I2C driver for AVR devices which uses hardware interrupts for better precision working. "nb-usitwisl-if" an interrupt-free version of the same driver necessary for bootloaders running on AVR devices without dedicated I2C hardware (like ATtiny85/45/25).

Nevertheless, note that in this v1.3 release, the driver is merged as inline functions in the Timonel bootloader "C" source code, so the interrupt-free driver sources are left here for reference only.