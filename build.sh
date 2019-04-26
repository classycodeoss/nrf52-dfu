#!/bin/bash

#
#  build.sh
#  nrf52-dfu
#
#  nRF52 firmware update from second CPU
#
#  BSD 3-Clause License
# 
#  Created by Andreas Schweizer on 26.04.2019.
#  Copyright (c) 2019, Classy Code GmbH
#  All rights reserved.
#  
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions are met:
#  
#  1. Redistributions of source code must retain the above copyright notice, this
#     list of conditions and the following disclaimer.
#  
#  2. Redistributions in binary form must reproduce the above copyright notice,
#     this list of conditions and the following disclaimer in the documentation
#     and/or other materials provided with the distribution.
#  
#  3. Neither the name of the copyright holder nor the names of its
#     contributors may be used to endorse or promote products derived from
#     this software without specific prior written permission.
#  
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
#  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
#  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
#  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
#  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
#  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
#  SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
#  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
#  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
#  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#  

#set -x
set -e

export APP_VERSION="1"

export TOOLCHAIN_NAME="gcc-arm-none-eabi-7-2018-q2-update"
export SDK_NAME="nRF5_SDK_15.2.0_9412b96"
export CMDLINETOOLS_NAME="nRF-Command-Line-Tools_9_8_1_OSX"
export SOFTDEVICE_NAME="s132_nrf52_6.1.0_softdevice.hex"

export SOFTDEVICE=sdk/$SDK_NAME/components/softdevice/s132/hex/$SOFTDEVICE_NAME

export GNU_VERSION="7.3.1"
export GNU_PREFIX="arm-none-eabi"

# ------------------------------ Verify Prerequisites ------------------------------

echo "Checking prerequisites (toolchain, SDK, command line tools)..."

if [ ! -d toolchain/$TOOLCHAIN_NAME ]; then
  echo "*****************************************************************************************"
  echo "The toolchain is missing in the toolchain directory ($TOOLCHAIN_NAME)."
  echo "Please download and extract the toolchain first."
  echo "URL: https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm/downloads"
  echo "******************************************************************************************"
  exit -1
fi
echo "Toolchain found."

if [ ! -d sdk/$SDK_NAME ]; then
  echo "*****************************************************************************************"
  echo "The SDK is missing in the sdk directory ($SDK_NAME)."
  echo "Please download and extract the SDK first."
  echo "URL: https://developer.nordicsemi.com/nRF5_SDK/nRF5_SDK_v15.x.x/nRF5_SDK_15.2.0_9412b96.zip"
  echo "******************************************************************************************"
  exit -1
fi
echo "SDK found."

if [ ! -d sdk/$CMDLINETOOLS_NAME ]; then
  echo "*****************************************************************************************"
  echo "The command line tools are missing in the sdk directory ($CMDLINETOOLS_NAME)."
  echo "Please download and extract the command line tools first."
  echo "URL: https://www.nordicsemi.com/Software-and-Tools/Development-Tools/nRF5-Command-Line-Tools"
  echo "******************************************************************************************"
  exit -1
fi
echo "Command line tools found."

echo "Prerequisites OK!"

# ------------------------------ Apply Patches  ------------------------------

# Patch an issue in the 15.2.0 SDK (https://devzone.nordicsemi.com/f/nordic-q-a/37120/nrfx_uart-bug)
if [ ! -f sdk/patched ]; then
  echo "Patching nrfx_uart..."
  patch -p0 < nrfx_uart.patch
  echo "patch_done" > sdk/patched
  echo "Patch OK."
fi

# ------------------------------ Compile externals ------------------------------

export GNU_INSTALL_ROOT="../../../../../../toolchain/$TOOLCHAIN_NAME/bin/"

# The build script for micro-ecc doesn't work, so we build micro-ecc manually here.
pushd sdk/$SDK_NAME/external/micro-ecc
if [ ! -f micro-ecc/uECC.c ]; then
    echo "micro-ecc not found! Let's pull it from HEAD."
    git clone https://github.com/kmackay/micro-ecc.git
fi
make -C nrf52hf_armgcc/armgcc &&
make -C nrf52nf_armgcc/armgcc
popd

# ------------------------------ Compile bootloader ------------------------------

# The resulting hex file can be installed on the SoC like this:
# $ nrfjprog -f nrf52 --eraseall
# $ nrfjprog -f nrf52 --program bootloader_sd_mbr_uicr.hex --sectoranduicrerase
# $ nrfjprog -f nrf52 --reset

export GNU_INSTALL_ROOT="../../toolchain/$TOOLCHAIN_NAME/bin/"
export MERGEHEX_DIR="../../sdk/$CMDLINETOOLS_NAME/mergehex"
export PATH=$PATH:$MERGEHEX_DIR

pushd src/bootloader

make clean
make
make hex
cp _build/bootloader_softdevice.hex ../../assets/bootloader_sd_mbr_uicr.hex
make clean

popd

# ------------------------------ Compile firmware ------------------------------

export GNU_INSTALL_ROOT="../../toolchain/$TOOLCHAIN_NAME/bin/"

pushd src/firmware

make clean
make
cp _build/nrf52832_xxaa.hex ../../assets/firmware.hex
make clean

popd

# ------------------------------ Create DFU package ------------------------------

cp assets/firmware.hex dfu_zip

# Install pip in ~/Library/Python/2.7/bin/pip
rm -rf /tmp/pip
mkdir /tmp/pip
curl https://bootstrap.pypa.io/get-pip.py -o /tmp/get-pip.py
python /tmp/get-pip.py --user
PATH=$PATH:~/Library/Python/2.7/bin
pip install --user --ignore-installed six nrfutil

# Possible SoftDevices...
# Specify the list in the nrfutil call, e.g. "--sd-req 0x87,0x8C,0xA7,0xA8,0xA9".
# For SD+BL+App updates, include "--sd-id".
nrfutil pkg generate --help

pushd dfu_zip

nrfutil pkg generate --hw-version 52 --application-version $APP_VERSION \
  --application firmware.hex --sd-req 0xAF --key-file private.key \
  dfu_firmware.zip

# (Programming with nrfutil would look like this:)
# nrfutil -v dfu serial --package app_dfu_package.zip --port "/dev/tty.usbserial-AM017YOO" --flow-control 0 --baud-rate 57600

cp dfu_firmware.zip ../assets
rm -f *.hex *.zip

popd

echo "DONE!"

