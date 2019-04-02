SL14 => SL33 (2A0) pre-error address
SL22 => SL33 (2C0)
SL31 => SL33 (Can't operate)

What happen when a slave dev reads an address that's not its own? Does it send NACK???
 V
 V
 V
CHECK WITH DIODES !!!
 V
 V
 V
Outcome:
--------
When the TWI master requests a memory dump from a specific slave device, this slave starts transmitting its memory contents to the TWI bus. During this transmission, apparently, a non-addressed slave interprets a false TWI START condition due to a particular data pattern sent by the addressed slave. When this occurs, as soon as the addressed slave transmits a data pattern that matches the non-addressed slave address, this one sends wrongly a TWI ACKNOWLEDGE, corrupting the addressed slave transmission and forcing the master to abort the running command.

Analyzer capture:
-----------------
"SL33(direct)-SL14(behind-diodes_not_initialized)-cmd-DUMPFLSH-Focus_Addr_2A8.sr"

------------------------------------------------------------------------------------------ 
 
Test 1:
======

SL14 (direct) FC 130 / 0x1900 initialized
SL33 (direct) FC 130 / 0x1980 initialized
 
SL33 Command: Dump Memory (READFLSH 0x87 n-times starting from position 0) output:
------------------------------------------------------------------------------------------ 
Addr 02A0: [TwiCmdXmit] >> Multi: 0x87 --> making actual TWI transmission ...
FF FF FF FF FF FF FF FF [TwiCmdXmit] >> Multi: 0x87 --> making actual TWI transmission ...
[TwiCmdXmit] Error parsing 0x87 multibyte command <<< 255
[DumpMemory] DumpFlashMem Error parsing 135 command <<< 255
[~NbMicro] Freeing TWI address 33 ...
 
SL14 Command: Dump Memory (READFLSH 0x87 n-times starting from position 0) output:
------------------------------------------------------------------------------------------ 
Addr 06E0: [TwiCmdXmit] >> Multi: 0x87 --> making actual TWI transmission ...
FF FF FF FF FF FF FF FF [TwiCmdXmit] >> Multi: 0x87 --> making actual TWI transmission ...
FF FF FF FF FF FF FF FF [TwiCmdXmit] >> Multi: 0x87 --> making actual TWI transmission ...
[TwiCmdXmit] Error parsing 0x87 multibyte command <<< 255
[DumpMemory] DumpFlashMem Error parsing 135 command <<< 255
[~NbMicro] Freeing TWI address 14 ...

or 

Addr 0000: [TwiCmdXmit] >> Multi: 0x87 --> making actual TWI transmission ...
FF FF FF FF FF FF FF FF [TwiCmdXmit] >> Multi: 0x87 --> making actual TWI transmission ...
[TwiCmdXmit] Error parsing 0x87 multibyte command <<< 255
[DumpMemory] DumpFlashMem Error parsing 135 command <<< 255
[~NbMicro] Freeing TWI address 14 ...
+
SL33 Freezed and holding the TWI bus !!!







 
 