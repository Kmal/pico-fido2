idf_component_register(
    SRCS
        pico_fido2_display.c
        pico_fido2_m5_board.c
        pico_fido2_m5pm1.c
        pico_fido2_presence.c
        pico_fido2_tca8418.c
    INCLUDE_DIRS include
    REQUIRES esp_driver_gpio esp_driver_i2c esp_driver_spi esp_timer freertos
)

if(PICO_BOARD STREQUAL "m5stack_cardputer_adv")
    target_compile_definitions(${COMPONENT_LIB} PRIVATE
        PICO_FIDO2_BOARD_M5STACK_CARDPUTER_ADV=1
        PICO_FIDO2_BOARD_M5STACK_STICK_S3=0
    )
elseif(PICO_BOARD STREQUAL "m5stack_stick_s3")
    target_compile_definitions(${COMPONENT_LIB} PRIVATE
        PICO_FIDO2_BOARD_M5STACK_CARDPUTER_ADV=0
        PICO_FIDO2_BOARD_M5STACK_STICK_S3=1
    )
else()
    message(FATAL_ERROR "src/m5boards requires PICO_BOARD=m5stack_cardputer_adv or PICO_BOARD=m5stack_stick_s3")
endif()
