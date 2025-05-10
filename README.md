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

## test connection
* connect badge to USB
* find serial port with board list
```
arduino-cli board list
```

## update serial port in load script
* edit BS_Knoxville_2025/FIRMWARE/build/esp32.esp32.esp32s3/CMD_LINE.sh
* update `port` variable

## run update firmware command
```
cd BS_Knoxville_2025/FIRMWARE/build/esp32.esp32.esp32s3
./CMD_LINE.sh
```
