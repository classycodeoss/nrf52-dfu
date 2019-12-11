# nrf52-dfu
Nordic nRF52 firmware update example


step by step instructions
-------------------------


create bootloader:

$ cd 02_Bootloader
$ make
$ make hex
$ make flash_hex

-> a serial console connected to the nrf52 should now display @@BOOTLOADER



create application:

$ cd 01_Demo_App
$ make



create DFU package, send it to bootloader:

$ cd dfu_zip
$ make
$ make dfu
