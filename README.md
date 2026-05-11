# Pico FIDO2

This project transforms your Raspberry RP235x or ESP32 microcontroller into an integrated FIDO Passkey **and** OpenPGP smartcard, functioning like a standard USB Passkey for authentication and as a smartcard for cryptographic operations.

This is a fork of the last community edition version of the pico-fido2 firmware, from December 9th 2025, that was available on https://github.com/polhenarejos/pico-fido2 before that repository was deleted and replaced by something else.

---

## Supported platforms

| | RP2040 | RP2350 | ESP32-S2 | ESP32-S3 |
|---|---|---|---|---|
| CPU | 2x Cortex-M0+ | 2x Cortex-M33 | 1x Xtensa | 2x Xtensa |
| Core pinning | Yes  | Yes | No  | Yes |
| RTOS | No (Pico SDK) | No (Pico SDK) | FreeRTOS | FreeRTOS |
| MCU ID | `0` | `1` | `2` | `2` |

### Security

Currently most secure features are supported and implemented only for RP2350.

| | RP2350 | RP2040 | ESP32-S2 | ESP32-S3 |
|---|---|---|---|---|
| Secure Boot | Full (boot key hash, CRIT1 flags, debug disable, glitch detector) | No HW support | No (`// TODO`) | No (`// TODO`) |
| Secure Lock | Yes (key invalidation, page locking) | No | No | No |
| MKEK in OTP/eFuse | Yes (OTP rows with ECC, chaff, page locking) | No (plaintext flash) | Yes (eFuse `BLK_KEY3`, write-locked) | Yes (eFuse `BLK_KEY3`, write-locked) |
| Device key in OTP/eFuse | Yes (OTP + chaff + migration) | No | Yes (eFuse `BLK_KEY4`) | Yes (eFuse `BLK_KEY4`) |
| `cmd_secure` APDU | Available | Not available | Available | Available |
| Firmware signing | Yes (`pico_sign_binary`) | No | No | No |
| Rollback protection | Yes | No | No | No |
| HW crypto | SHA-256	| No | SHA-256 + AES-GCM + ECDSA/ECDH | SHA-256 + AES-GCM + ECDSA/ECDH |

## Features

Pico FIDO2 includes the following features:

### FIDO2 / U2F / WebAuthn

- CTAP 2.1 / CTAP 1
- WebAuthn
- U2F
- HMAC-Secret extension
- CredProtect extension
- User presence enforcement through physical button
- User verification with PIN
- Discoverable credentials (resident keys)
- Credential management
- ECDSA and EDDSA authentication
- Support for SECP256R1, SECP384R1, SECP521R1, SECP256K1 and Ed25519 curves
- App registration and login
- Device selection
- Support for vendor configuration
- Backup with 24 words
- Secure lock to protect the device from flash dumps
- Permissions support (MC, GA, CM, ACFG, LBW)
- Authenticator configuration
- minPinLength extension
- Self attestation
- Enterprise attestation
- credBlobs extension
- largeBlobKey extension
- Large blobs support (2048 bytes max)
- OATH (based on YKOATH protocol specification)
- TOTP / HOTP
- Yubikey One Time Password
- Challenge-response generation
- Emulated keyboard interface
- Button press generates an OTP that is directly typed
- Yubico YKMAN compatible
- Nitrokey nitropy and nitroapp compatible
- Secure Boot and Secure Lock in RP2350 and ESP32-S3 MCUs
- One Time Programming to store the master key that encrypts all resident keys and seeds.
- Rescue interface to allow recovery of the device if it becomes unresponsive or undetectable.
- LED customization with Pico Commissioner.

### OpenPGP Smartcard

- OpenPGP card specification v3.4
- 3 key slots (Signature, Encryption, Authentication)
- RSA (2048, 3072, 4096), Ed25519, Curve25519, ECDSA (NIST P-256, P-384, P-521)
- Key generation on device
- Key import/export
- PIN and Admin PIN protection
- Reset and Unblock functions
- Works with GnuPG, SSH, S/MIME, and compatible tools
- CCID over USB
- Compatible with major OS (Linux, Windows, macOS)
- Touch button for user presence confirmation (optional)
- Open source

---

## Security Considerations

Microcontrollers RP2350 and ESP32-S3 are designed to support secure environments when Secure Boot is enabled, and optionally, Secure Lock. These features allow a master key encryption key (MKEK) to be stored in a one-time programmable (OTP) memory region, which is inaccessible from outside secure code. This master key is then used to encrypt all private and secret keys on the device, protecting sensitive data from potential flash memory dumps.

**However**, the RP2040 microcontroller lacks this level of security hardware, meaning that it cannot provide the same protection. Data stored on its flash memory, including private or master keys, can be easily accessed or dumped, as encryption of the master key itself is not feasible. Consequently, if an RP2040 device is stolen, any stored private or secret keys may be exposed.

---

## Build for Raspberry Pico

Before building, ensure you have installed the toolchain for the Pico and that the Pico SDK is properly located on your drive.

```
git clone https://github.com/youruser/pico-fido2
git submodule update --init --recursive
cd pico-fido2
mkdir build
cd build
PICO_SDK_PATH=/path/to/pico-sdk cmake .. -DPICO_BOARD=board_type -DUSB_VID=0x1D50 -DUSB_PID=0x619B
make
```

Note that `PICO_BOARD`, `USB_VID` and `USB_PID` are optional. If not provided, `pico` board and VID/PID `1D50:619B` will be used.

Additionally, you can pass the `VIDPID=value` parameter to build the firmware with a known VID/PID. The supported values are:

- `NitroHSM`
- `NitroFIDO2`
- `NitroStart`
- `NitroPro`
- `Nitro3`
- `Yubikey5`
- `YubikeyNeo`
- `YubiHSM`
- `Gnuk`
- `GnuPG`

You can use whatever VID/PID for your own personal use. **But remember that you are not authorized to distribute the binary with a VID/PID that you do not own.**
The VID/PID `1D50:619B` is provided to the project by [OpenMoko](https://wiki.openmoko.org/wiki/USB_Product_IDs). It can only be used for builds distributed under a free and open source license.

After running `make`, the binary file `pico_fido2.uf2` will be generated. To load this onto your Pico board:

1. Put the Pico board into loading mode by holding the `BOOTSEL` button while plugging it in.
2. Copy the `pico_fido2.uf2` file to the new USB mass storage device that appears.
3. Once the file is copied, the Pico mass storage device will automatically disconnect, and the Pico board will reset with the new firmware.
4. A blinking LED will indicate that the device is ready to work.

To configure your device you can use the [picoforge desktop application ](https://github.com/librekeys/picoforge).

## Drivers

Pico FIDO2 uses the `HID` driver for FIDO and `CCID` for OpenPGP, both present in all major operating systems. It should be detected by all OS and browser/applications just like normal USB FIDO keys and smartcards.

## License

This project is released under the GNU Affero General Public License v3 (AGPLv3).
A copy of the AGPLv3 license is available in the `LICENSE` file.

## Credits
This project uses libraries and portion of code from other projects that are detailed in the `LICENSE` file.

## M5Stack ESP32-S3 boards

The ESP-IDF build supports two M5Stack ESP32-S3 devices in addition to the RP2040/Pico SDK builds:

- `m5stack-cardputer-adv` — M5Stack Cardputer-Adv, ESP32-S3FN8, 8 MB flash, ST7789V2 LCD, TCA8418 keyboard, ES8311 audio, BMI270 IMU, and battery ADC on GPIO10.
- `m5stack-stick-s3` — M5Stack StickS3, ESP32-S3-PICO-1-N8R8, 8 MB flash, 8 MB PSRAM, ST7789P3 LCD, ES8311 audio, BMI270 IMU, M5PM1 power management, IR TX/RX, and KEY1/KEY2 buttons.

The board data is selected at build time with `PICO_BOARD`, matching the Raspberry Pico build pattern. The M5Stack ESP-IDF builds also use board-specific sdkconfig fragments for ESP32-S3 flash, PSRAM, and optional display settings.

### M5Stack ESP32-S3 build examples

Build Cardputer-Adv:

```bash
idf.py -D PICO_BOARD=m5stack_cardputer_adv -D SDKCONFIG="sdkconfig.m5stack_cardputer_adv" -D SDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.defaults.esp32s3;sdkconfig.defaults.m5stack_cardputer_adv" set-target esp32s3 build
```

Build, flash, and monitor Cardputer-Adv:

```bash
idf.py -D PICO_BOARD=m5stack_cardputer_adv -D SDKCONFIG="sdkconfig.m5stack_cardputer_adv" -D SDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.defaults.esp32s3;sdkconfig.defaults.m5stack_cardputer_adv" set-target esp32s3 build flash monitor
```

Build StickS3:

```bash
idf.py -D PICO_BOARD=m5stack_stick_s3 -D SDKCONFIG="sdkconfig.m5stack_stick_s3" -D SDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.defaults.esp32s3;sdkconfig.defaults.m5stack_stick_s3" set-target esp32s3 build
```

Build, flash, and monitor StickS3:

```bash
idf.py -D PICO_BOARD=m5stack_stick_s3 -D SDKCONFIG="sdkconfig.m5stack_stick_s3" -D SDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.defaults.esp32s3;sdkconfig.defaults.m5stack_stick_s3" set-target esp32s3 build flash monitor
```

To build both M5Stack ESP32-S3 profiles and write board-named merged binaries into `release/`, run:

```bash
./build_m5stack_esp32s3.sh
```

### M5Stack download mode

- Cardputer-Adv: switch power OFF, hold `G0`, apply USB power, then release `G0` after the host detects the bootloader.
- StickS3: connect USB and hold the reset button until the internal green LED flashes.

### User-presence behavior

The ESP-IDF M5Stack profiles replace the default ESP32 BOOT/GPIO0 button path with the board-aware user-presence adapter and enable a default 15-second user-presence timeout when no provisioned timeout exists in PHY data.

- Cardputer-Adv: any fresh TCA8418 keyboard key-press event confirms FIDO user presence.
- StickS3: a transition from released to pressed on either KEY1 or KEY2 confirms FIDO user presence.

### Hardware caveats

- StickS3 IR receive is an RMT-style peripheral use case and should not be treated as plain GPIO polling.
- StickS3 speaker amplifier power stays disabled by default because the M5Stack examples note that speaker and IR receive should not be active together.
- StickS3 external 5 V output must not be enabled blindly; follow M5Stack EXT_5V_EN warnings before adding support.
- Cardputer-Adv battery measurement is available through ADC on GPIO10.
- The optional LCD status API is compiled as a no-op unless `CONFIG_PICO_FIDO2_M5_DISPLAY_STATUS=y` is enabled; when enabled, runtime status and two-line messages are queued to a display task so FIDO USB work is not blocked by LCD drawing.
