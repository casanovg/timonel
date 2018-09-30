:echo ""
:echo "***********************************************************************"
:echo "*                                                                     *"
:echo "* Please use USBasp for flashing Timonel to the ATtiny device         *"
:echo "* =================================================================== *"
:echo "*                                                                     *"
:echo "***********************************************************************"
:echo ""

:del rudder-interrupt-free.* & make all & avr-size rudder-interrupt-free.elf & avr-size rudder-interrupt-free.hex & avrdude -c USBasp -p attiny85 -U flash:w:rudder-interrupt-free.hex:i -B 20

avrdude -c USBasp -p attiny85 -U flash:w:.\\releases\\tml-bootloader.hex:i -B 20