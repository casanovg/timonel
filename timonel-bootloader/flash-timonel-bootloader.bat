:echo ""
:echo "***********************************************************************"
:echo "*                                                                     *"
:echo "* Please use USBasp for flashing Timonel to the ATtiny device         *"
:echo "* =================================================================== *"
:echo "*                                                                     *"
:echo "***********************************************************************"
:echo ""

avrdude -c USBasp -p attiny85 -U flash:w:.\\releases\\tml-bootloader.hex:i -B 20
