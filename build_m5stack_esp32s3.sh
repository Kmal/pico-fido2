#!/bin/bash
set -euo pipefail

VERSION_MAJOR="6"
VERSION_MINOR="6"
SUFFIX="${VERSION_MAJOR}.${VERSION_MINOR}"
all_boards=(m5stack_cardputer_adv m5stack_stick_s3)

if [[ $# -gt 0 ]]; then
    boards=("$@")
else
    boards=("${all_boards[@]}")
fi

mkdir -p release

for board in "${boards[@]}"; do
    case " ${all_boards[*]} " in
        *" ${board} "*) ;;
        *)
            echo "Unsupported M5Stack board: ${board}" >&2
            echo "Supported boards: ${all_boards[*]}" >&2
            exit 2
            ;;
    esac

    build_dir="build_${board}"
    idf.py -B "${build_dir}" \
        -D PICO_BOARD="${board}" \
        -D SDKCONFIG="sdkconfig.${board}" \
        -D SDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.defaults.esp32s3;sdkconfig.defaults.${board}" \
        set-target esp32s3 \
        build
    (cd "${build_dir}" && esptool.py --chip ESP32-S3 merge_bin \
        -o "../release/pico_fido2_${board}-${SUFFIX}.bin" @flash_args)
done
