Timonel-I2CMaster Quick Install Notes
=====================================
1) Install Python on your computer (http://www.python.org/downloads)

2) Verify Python version

   $ python --version

3) Install PIP packet manager

   $ pip --version

4) Install esptool.py
  
   $ pip install --upgrade esptool

5) Check which serial ports are available to use the serial USB adapter, COM14 in this example
   
   $ python -m serial.tools.list_ports
   
   COM1
   
   __COM14__
   
   2 ports found

6) Optionally, backup the current ESP8266 firmware
   
   $ esptool.py --port COM14 --baud 115200 read_flash 0x00000 0x100000 current_firmware.bin

7) Erase the ESP8266 flash memory

   $ esptool.py --port com14 --baud 115200 erase_flash

8) Flash timonel-i2cmaster to the ESP8266

   $ esptool.py --port COM14 --baud 115200 write_flash -fs detect -fm qio 0x00000 Release/timonel-i2cmaster.bin
   
