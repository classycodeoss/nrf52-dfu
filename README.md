# nrf52-dfu
Nordic nRF52 firmware update example


step by step instructions
-------------------------

create bootloader:

$ cd 02_Bootloader
$ make clean
$ make
$ make hex
$ make flash_hex

-> a serial console connected to the nrf52 should now display @@BOOTLOADER


create application v1:

$ cd ..
$ cd 01_Demo_App
$ make clean
$ make v1


create DFU package, send it to bootloader:

$ cd dfu_zip
$ make clean
$ make v1
$ make dfu


create application v2:

$ cd ..
$ cd 01_Demo_App
$ make clean
$ make v2


create DFU package:

$ cd dfu_zip
$ make clean
$ make v2


create DFU blobs:

$ cp app_dfu_package.zip /tmp
$ pushd /tmp
$ unzip app_dfu_package.zip
$ popd

$ cd ../..
$ cd 05_Firmware_Converter
$ gcc fwconvert.c
$ ./a.out /tmp/nrf52832_xxaa.bin dfu_firmware_bin.h gFirmwareBin
$ ./a.out /tmp/nrf52832_xxaa.dat dfu_firmware_dat.h gFirmwareDat


perform DFU with demo host application:

$ cd ..
$ cd 04_Demo_Host_Application
$ cp ../05_Firmware_Converter/dfu_firmware_bin.h .
$ cp ../05_Firmware_Converter/dfu_firmware_dat.h .
$ make

-> press the button on the target board to trigger the DFU process

$ ./fwu "/dev/tty.usbserial-DN009NQG" 57600  <-- configure for your serial device
