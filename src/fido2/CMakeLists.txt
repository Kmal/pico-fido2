set(FIDO2_INCLUDE_DIRS
    .
    ../../pico-keys-sdk/src
    ../../pico-keys-sdk/src/fs
    ../../pico-keys-sdk/src/rng
    ../../pico-keys-sdk/src/usb
    ../../pico-keys-sdk/tinycbor/src
    ../../pico-fido/src
    ../../pico-openpgp/src
)
set(FIDO2_REQUIRES bootloader_support esp_partition esp_tinyusb mbedtls efuse)

if(PICO_FIDO2_M5STACK_BOARD)
    list(APPEND FIDO2_INCLUDE_DIRS ../../pico-keys-sdk/src/led ../m5boards/include)
    list(APPEND FIDO2_REQUIRES m5boards)
endif()

idf_component_register(
    SRCS ${SOURCES}
    INCLUDE_DIRS ${FIDO2_INCLUDE_DIRS}
    REQUIRES ${FIDO2_REQUIRES}
)
idf_component_set_property(${COMPONENT_NAME} WHOLE_ARCHIVE ON)
