-----THE BOARD NEEDS TO OFF WHEN PLUGGING IT IN TO YOUR COMPUTER-----
-----The Serial port will not come be live until the Battery is diconnected for saftey reasons--------

Arduino Config

Board ESP32 Dev Module
https://github.com/espressif/arduino-esp32

RGB LED Lib
https://github.com/MartyMacGyver/ESP32-Digital-RGB-LED-Drivers

OLED Lib
https://github.com/olikraus/u8g2


Serial Driver

Linux Driver Info
https://learn.sparkfun.com/tutorials/how-to-install-ch340-drivers/linux

Windows Driver
https://www.wch-ic.com/downloads/CH341SER_EXE.html

Micropython
https://micropython.org/download/ESP32_GENERIC_S3/

# macOS/Linux setup
## clone / fork badge repo
* https://github.com/mrogers315/BS_Knoxville_2025
```
git clone git@github.com:mrogers315/BS_Knoxville_2025.git
```

## Install arduino CLI
* https://docs.arduino.cc/arduino-cli/installation/
```
brew install arduino-cli
```

## install arduino-esp32
* https://pypi.org/project/esptool/
```
pip install esptool
```

## test connection and note serial port
* connect badge to USB
* find serial port with the command below
```
arduino-cli board list
```

## run update firmware command passing the serial port
```
cd FIRMWARE/build/esp32.esp32.esp32s3
./CMD_LINE.sh /dev/cu.usbserial-210
```
# Alternate setup with Arduino IDE (GUI)
## clone / fork badge repo
* https://github.com/mrogers315/BS_Knoxville_2025
```
git clone git@github.com:mrogers315/BS_Knoxville_2025.git
```

## Install Arduino IDE
* https://www.arduino.cc/en/software/

## Arduino IDE configuration
- Device type should be `ESP32S3 Dev Module`
- Device connection is likely `/dev/cu.usbserial-10` (may differ on your machine)
- We need to install `U8g2` and `Freenove_WS2812_Lib_for_ESP32` (screen libraries) through `Tools` -> `Manage Libraries`
- To compile: `Sketch` -> `Verify/Compile`
- If the firmware compiles without errors, do `Sketch` -> `Upload`
- If you encounter `A fatal error occurred: Unable to verify flash chip connection (No serial data received.)` when uploading, try lowering default baud rate from `921600` to one step lower `460800` via `Tools` -> `Upload Speed`
  - https://forum.arduino.cc/t/esp32-a-fatal-error-occurred-unable-to-verify-flash-chip-connection-no-serial-data-received-failed-uploading-uploading-error-exit-status-2/1250126/4
