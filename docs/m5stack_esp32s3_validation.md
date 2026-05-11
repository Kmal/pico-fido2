# M5Stack ESP32-S3 validation checklist

This checklist covers manual validation for the M5Stack Cardputer-Adv and M5Stack StickS3 ESP-IDF profiles. `PICO_BOARD` selection controls which M5Stack board descriptor and user-presence bridge are compiled. The unselected board macro must compile as `0` so warning-clean builds can enable `-Wundef`.

## Cardputer-Adv

- `idf.py -D PICO_BOARD=m5stack_cardputer_adv -D SDKCONFIG="sdkconfig.m5stack_cardputer_adv" -D SDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.defaults.esp32s3;sdkconfig.defaults.m5stack_cardputer_adv" set-target esp32s3 build` succeeds.
- USB HID/CCID interfaces enumerate with the configured VID/PID.
- USB product string matches `Pico FIDO2 Cardputer-Adv` unless PHY data already provisions a product string.
- FIDO registration requires a fresh key press on the TCA8418 keyboard through the board-aware `button_wait()` adapter.
- OATH/OpenPGP flows continue to work when those applications are enabled.
- Battery ADC on GPIO10 reads a plausible voltage after board-specific ADC calibration is applied.
- LCD status/message output works only when `CONFIG_PICO_FIDO2_M5_DISPLAY_STATUS=y` is enabled, and display updates are handled by the display task rather than inline FIDO request processing.
- During FIDO user-presence prompts, the optional LCD shows boot, prompt, accepted, and timeout/failure messages.

## StickS3

- `idf.py -D PICO_BOARD=m5stack_stick_s3 -D SDKCONFIG="sdkconfig.m5stack_stick_s3" -D SDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.defaults.esp32s3;sdkconfig.defaults.m5stack_stick_s3" set-target esp32s3 build` succeeds.
- The build reports the ESP32-S3 target, 8 MB flash, and PSRAM enabled.
- USB HID/CCID interfaces enumerate with the configured VID/PID.
- USB product string matches `Pico FIDO2 StickS3` unless PHY data already provisions a product string.
- FIDO registration requires KEY1 or KEY2 through the board-aware `button_wait()` adapter.
- M5PM1 initializes at I2C address `0x6e`.
- L3B power is enabled during runtime.
- The speaker amplifier remains disabled unless audio status is explicitly enabled.
- IR receive is not claimed unless it is implemented through RMT.

## CI guidance

Only compile validation should be automated in CI unless dedicated hardware runners are available. Hardware behavior, USB enumeration, PMIC behavior, battery ADC readings, and FIDO user-presence flows remain manual validation steps.
