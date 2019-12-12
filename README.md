# nrf52-dfu
Nordic nRF52 firmware update example


step by step instructions
-------------------------

Provide the SDK and toolchain in the root directory:

sdk/nRF5_SDK_15.2.0_9412b96
toolchain/gcc-arm-none-eabi-7-2018-q2-update


Create the bootloader:

$ cd 02_Bootloader
$ make clean
$ make
$ make hex
$ make flash_hex

-> a serial console connected to the nrf52 should now display @@BOOTLOADER


Create application v1:

$ cd ..
$ cd 01_Demo_App
$ make clean
$ make v1


Create the DFU package, send it to the bootloader:

$ cd dfu_zip
$ make clean
$ make v1
$ make dfu


Create application v2:

$ cd ..
$ cd 01_Demo_App
$ make clean
$ make v2


Create the DFU package for v2:

$ cd dfu_zip
$ make clean
$ make v2


Create v2 DFU blobs:

$ cp app_dfu_package.zip /tmp
$ pushd /tmp
$ unzip app_dfu_package.zip
$ popd

$ cd ../..
$ cd 05_Firmware_Converter
$ gcc fwconvert.c
$ ./a.out /tmp/nrf52832_xxaa.bin dfu_firmware_bin.h gFirmwareBin
$ ./a.out /tmp/nrf52832_xxaa.dat dfu_firmware_dat.h gFirmwareDat


Perform DFU with the demo host application:

$ cd ..
$ cd 04_Demo_Host_Application
$ cp ../05_Firmware_Converter/dfu_firmware_bin.h .
$ cp ../05_Firmware_Converter/dfu_firmware_dat.h .
$ make

-> press the button on the target board to trigger the DFU process

$ ./fwu "/dev/tty.usbserial-DN009NQG" 57600  <-- configure for your serial device
