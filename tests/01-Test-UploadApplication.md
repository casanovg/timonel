Devices preparation
--------------------
__Device 1 setup:__

./make-timonel.sh tml-t85-full t-full-11 11 1A80; ./flash-timonel-bootloader.sh t-full-11 1; ./fuse-read.sh

__Device 2 setup:__

./make-timonel.sh tml-t85-small-dump t-smdm-33 33 1A80; ./flash-timonel-bootloader.sh t-smdm-33 1; ./fuse-read.sh

Test payload
------------
timonel/timonel-testbed/data/payloads/payload_sos_full_memory_1A80.h