Timonel - ATtiny85 I2C Bootloader (i2c-m)
=========================================
__i2c-m__: this branch is to develop __timonel-i2cmaster__ improvements ...

For a general description of this project, please open the master branch [readme](../master/README.md) ...

Contributuions are welcome!

Development Environment Setup (Windows) / Master: ESP8266, Slave: ATTiny85
--------------------------------------------------------------------------
1-Install WinAVR in "C:\WinAVR-20100110"
2-Replace WinAVR subdirectories with the newer version from (http://blog.zakkemble.net/avr-gcc-builds)
  (http://blog.zakkemble.net/download/avr-gcc-8.2.0-x64-mingw.zip) at the time of this writing.
  Open a Window command box and run: "avr-gcc --version" to check that the newest version is in your path.
3-Install Git for Windows (https://git-scm.com/download).
4-Install Visual Studio Code (https://code.visualstudio.com/download).
5-Install Platformio (From VS Code --> Extensions --> search: Platformio IDE.
6-Install Notepad++ (https://notepad-plus-plus.org/download).
7-Install Python 3.7.2 or newer (https://www.python.org/downloads/release).
  (https://www.python.org/ftp/python/3.7.2/python-3.7.2-amd64.exe) for Windows x64 systems.
  Make sure to check "Add Python to PATH".
8-Open a Windows command prompt (or GIT Bash terminal) and run "pip install esptool"
  to install the ESP8266 utilities.
9-Install a mingw-w64 package (http://repo.msys2.org/distrib/x86_64/msys2-x86_64-20180531.exe).


