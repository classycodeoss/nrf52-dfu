FWU_LIB_PATH := ../03_Fwu_Library

all: $(FWU_LIB_PATH)/fwu.h
	gcc -I$(FWU_LIB_PATH) main.c $(FWU_LIB_PATH)/fwu.c -o fwu

run:
	./fwu /dev/tty.usbmodem0004830646701 57600

