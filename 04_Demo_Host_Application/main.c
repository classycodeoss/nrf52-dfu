//
//  main.c
//  nrf52-dfu
//
//  Sample host application to demonstrate the usage of our C library for the Nordic
//  firmware update protocol.
//
//  Created by Andreas Schweizer on 30.11.2018.
//  Copyright © 2018-2019 Classy Code GmbH
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of this
// software and associated documentation files (the "Software"), to deal in the Software
// without restriction, including without limitation the rights to use, copy, modify,
// merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to the following
// conditions:
//
// The above copyright notice and this permission notice shall be included in all copies
// or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
// INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
// PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
// HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
// CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
// OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>  // UNIX standard function definitions
#include <fcntl.h>   // File control definitions
#include <errno.h>   // Error number definitions
#include <termios.h> // POSIX terminal control definitions
#include <time.h>
#include "fwu.h"

// Input objects
#include "dfu_firmware_dat.h" // blob
#include "dfu_firmware_bin.h" // blob


static char *sSerialDevice;
static int sBaudrate;
static int sFd;
static int sTicks;
static int sSubticks;
static int sBytesSent;

static TFwu sFwu;

void txFunction(struct SFwu *fwu, uint8_t *buf, uint8_t len);
static uint8_t readData(uint8_t *data, int maxLen);
static void openSerialDevice();
static void configureSerialDevice();
static void printResponseStatus();

int main(int argc, char *argv[])
{
    if (argc != 3) {
        fprintf(stderr, "usage: %s <serial-port> <baudrate>\n", argv[0]);
        fprintf(stderr, "Small driver demo program to load a new firmware into an nRF52 device.\n");
        return -1;
    }
    
    sSerialDevice = argv[1];
    sBaudrate = atoi(argv[2]);
    
    openSerialDevice();
    configureSerialDevice();

    sFwu.commandObject = gFirmwareDat;
    sFwu.commandObjectLen = sizeof(gFirmwareDat);
    sFwu.dataObject = gFirmwareBin;
    sFwu.dataObjectLen = sizeof(gFirmwareBin);
    sFwu.txFunction = txFunction;
    sFwu.responseTimeoutMillisec = 5000;
    
    // Prepare the firmware update process.
    fwuInit(&sFwu);
    
    // Start the firmware update process.
    fwuExec(&sFwu);
    
    while (1) {
        struct timespec tsleep;
        tsleep.tv_sec = 0;
        tsleep.tv_nsec = 500000; // .5ms
        int s = nanosleep(&tsleep, NULL);
        sTicks += 500;
        
        // Can send 4 chars...
        // (On a microcontroller, you'd use the TX Empty interrupt or test a register.)
        fwuCanSendData(&sFwu, 4);

        // Data available? Get up to 4 bytes...
        // (On a microcontroller, you'd use the RX Available interrupt or test a register.)
        uint8_t rxBuf[4];
        uint8_t rxLen = readData(rxBuf, 4);
        if (rxLen > 0) {
            fwuDidReceiveData(&sFwu, rxBuf, rxLen);
        }

        // Give the firmware update module a timeslot to continue the process.
        EFwuProcessStatus status = fwuYield(&sFwu, ++sSubticks / 2);
        if (sSubticks >= 2) {
            sSubticks = 0;
        }

        if (status == FWU_STATUS_COMPLETION) {
            printf("\n***** Success! *****\n");
            return 0;
        }
        
        if (status == FWU_STATUS_FAILURE) {
            printf("\n***** Failed! Response Status = %d (", sFwu.responseStatus);
            printResponseStatus();
            printf(") *****\n");
            return -1;
        }
    }
}

void txFunction(struct SFwu *fwu, uint8_t *buf, uint8_t len)
{
    while (len--) {
        uint8_t c = *buf++;
        uint8_t n = write(sFd, &c, 1);
        if (++sBytesSent % 1000 == 0) {
            printf(".");
            fflush(stdout);
        }
    }
}

static uint8_t readData(uint8_t *data, int maxLen)
{
    uint8_t n = 0;
    uint8_t byte;
    
    fd_set rfds;
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 100;
    
    FD_ZERO(&rfds);
    FD_SET(sFd, &rfds);
    
    do {
        int x = select(sFd + 1, &rfds, NULL, NULL, &tv);
        if (!x) {
            return n;
        }
        
        int v = read(sFd, &byte, 1);
        if (v == 1) {
            maxLen--;
            n++;
            *data++ = byte;
        }
        
    } while (byte != 0xc0 && maxLen > 0);
    
    return n;
}

static void openSerialDevice()
{
    //  O_NOCTTY: the program doesn't want to be the "controlling terminal" for the port.
    //  O_NDELAY: ignore the DCD signal line.
    sFd = open(sSerialDevice, O_RDWR | O_NOCTTY | /*O_NDELAY | */ O_NONBLOCK);
    if (sFd < 0) {
        fprintf(stderr, "opening serial device '%s' failed, error %d\n", sSerialDevice, sFd);
        exit(-1);
    }
    printf("serial device successfully opened: '%s'\n", sSerialDevice);
}

static void configureSerialDevice()
{
    int res;
    struct termios settings;
    
    // would use FNDELAY in the arguments for non-blocking reads
    res = fcntl(sFd, F_SETFL, 0);
    if (res < 0) {
        perror("fcntl(F_SETFL)");
        exit(-1);
    }
    
    // save current port settings
    res = tcgetattr(sFd, &settings);
    if (res < 0) {
        perror("tcgetattr 2");
        exit(-1);
    }
    
    settings.c_cflag &= ~(CSIZE | CSTOPB | HUPCL);
    settings.c_cflag |= (CLOCAL | CREAD | CS8);
    settings.c_iflag = IGNPAR;
    settings.c_oflag = 0;
    settings.c_lflag = 0;
    
    settings.c_cc[VMIN] = 1;
    settings.c_cc[VTIME] = 0;
    
    printf("B115200 = %d, sBaudrate = %d\n", B115200, sBaudrate);
    cfsetispeed(&settings, sBaudrate);
    cfsetospeed(&settings, sBaudrate);
    
    tcflush(sFd, TCIFLUSH);
    tcsetattr(sFd, TCSANOW, &settings);
}

static void printResponseStatus()
{
    switch (sFwu.responseStatus) {
        case FWU_RSP_OK:                        printf("FWU_RSP_OK"); break;
        case FWU_RSP_TOO_SHORT:                 printf("FWU_RSP_TOO_SHORT"); break;
        case FWU_RSP_START_MARKER_MISSING:      printf("FWU_RSP_START_MARKER_MISSING"); break;
        case FWU_RSP_END_MARKER_MISSING:        printf("FWU_RSP_END_MARKER_MISSING"); break;
        case FWU_RSP_REQUEST_REFERENCE_INVALID: printf("FWU_RSP_REQUEST_REFERENCE_INVALID"); break;
        case FWU_RSP_ERROR_RESPONSE:            printf("FWU_RSP_ERROR_RESPONSE"); break;
        case FWU_RSP_TIMEOUT:                   printf("FWU_RSP_TIMEOUT"); break;
        case FWU_RSP_PING_ID_MISMATCH:          printf("FWU_RSP_PING_ID_MISMATCH"); break;
        case FWU_RSP_RX_OVERFLOW:               printf("FWU_RSP_RX_OVERFLOW"); break;
        case FWU_RSP_INIT_COMMAND_TOO_LARGE:    printf("FWU_RSP_INIT_COMMAND_TOO_LARGE"); break;
        case FWU_RSP_CHECKSUM_ERROR:            printf("FWU_RSP_CHECKSUM_ERROR"); break;
        case FWU_RSP_DATA_OBJECT_TOO_LARGE:     printf("FWU_RSP_DATA_OBJECT_TOO_LARGE"); break;
        case FWU_RSP_RX_INVALID_ESCAPE_SEQ:     printf("FWU_RSP_RX_INVALID_ESCAPE_SEQ"); break;
        default: printf("unknown response status: %d", sFwu.responseStatus); break;
    }
}
