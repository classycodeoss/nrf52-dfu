v1:
	cp ../_build/nrf52832_xxaa.hex .
	nrfutil pkg generate --hw-version 52 --application-version 1 --application nrf52832_xxaa.hex --sd-req 0xAF --key-file private.key app_dfu_package.zip

v2:
	cp ../_build/nrf52832_xxaa.hex .
	nrfutil pkg generate --hw-version 52 --application-version 2 --application nrf52832_xxaa.hex --sd-req 0xAF --key-file private.key app_dfu_package.zip

dfu:
	nrfutil -v dfu serial --package app_dfu_package.zip --port "/dev/tty.usbserial-DN009GRC" --flow-control 0 --baud-rate 57600

clean:
	rm -f *.hex *.zip
