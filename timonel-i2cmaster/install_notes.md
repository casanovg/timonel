Timonel-I2CMaster Quick Install Notes
=====================================
1) Install Python on your computer (http://www.python.org/downloads)

2) Verify Python version

   __$ python --version__

3) Install PIP packet manager

   __$ pip --version__

4) Install esptool.py
  
   __$ pip install --upgrade esptool__

5) Check which serial ports are available to use the serial USB adapter, COM14 in this example
   
   __$ python -m serial.tools.list_ports__
   
   COM1
   
   __COM14__
   
   2 ports found

6) Optionally, backup the current ESP8266 firmware
   
   __$ esptool.py --port COM14 --baud 115200 read_flash 0x00000 0x100000 current_firmware.bin__

7) Erase the ESP8266 flash memory

   __$ esptool.py --port com14 --baud 115200 erase_flash__

8) Flash timonel-i2cmaster to the ESP8266

   __$ esptool.py --port COM14 --baud 115200 write_flash -fs detect -fm qio 0x00000 Release/timonel-i2cmaster.bin__
   
