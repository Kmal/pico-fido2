#!/bin/bash
set -euo pipefail

git submodule update --init --recursive
sudo apt update

install_esp_idf() {
    if [[ ! -d esp-idf ]]; then
        git clone --depth 1 --branch v5.5 --recursive --shallow-submodules \
            https://github.com/espressif/esp-idf.git
    fi
    cd esp-idf
    git checkout tags/v5.5
    git submodule update --init --recursive --depth 1
    ./install.sh "$@"
    . ./export.sh
    cd ..
}

if [[ $1 == "pico" ]]; then
sudo apt install -y cmake gcc-arm-none-eabi libnewlib-arm-none-eabi libstdc++-arm-none-eabi-newlib
git clone https://github.com/raspberrypi/pico-sdk
cd pico-sdk
git checkout tags/2.1.1
git submodule update --init
cd ..
git clone https://github.com/raspberrypi/picotool
cd picotool
git submodule update --init
mkdir build
cd build
cmake -DPICO_SDK_PATH=../../pico-sdk ..
make -j`nproc`
sudo make install
cd ../..
mkdir build_pico
cd build_pico
cmake -DPICO_SDK_PATH=../pico-sdk ..
make
cd ..
elif [[ $1 == "esp32" ]]; then
sudo apt install -y git wget flex bison gperf python3 python3-pip python3-venv cmake ninja-build ccache libffi-dev libssl-dev dfu-util libusb-1.0-0
install_esp_idf esp32s3
idf.py set-target esp32s3
idf.py all
mkdir -p release
cd build
esptool.py --chip ESP32-S3 merge_bin -o ../release/pico_fido_esp32-s3.bin @flash_args
cd ..
cd esp-idf
./install.sh esp32s2
. ./export.sh
cd ..
idf.py set-target esp32s2
idf.py all
mkdir -p release
cd build
esptool.py --chip ESP32-S2 merge_bin -o ../release/pico_fido_esp32-s2.bin @flash_args
cd ..
elif [[ $1 == "m5stack" ]]; then
sudo apt install -y git wget flex bison gperf python3 python3-pip python3-venv cmake ninja-build ccache libffi-dev libssl-dev dfu-util libusb-1.0-0
install_esp_idf esp32s3
if [[ $# -gt 1 ]]; then
./build_m5stack_esp32s3.sh "$2"
else
./build_m5stack_esp32s3.sh
fi
else
mkdir build
cd build
cmake -DENABLE_EMULATION=1 ..
make
fi
