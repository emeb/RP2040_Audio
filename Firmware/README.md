# RP2040 Audio Firmware
This directory contains the source and build scripts for the RP2040 Audio
firmware.

## Features
* Simple graphic user interface for selecting and controlling a variety
of audio DSP algorithms.
* Single button and CV navigation through availble effects and settings.
* Non-volatile storage of effects parameters so user settings will survive
power cycling and restore to the previous configuration.
* Storage "pending" indicator.
* CPU Load meter to indicate how hard the system is working.
* Wet/Dry mix indicator.
* Independent input/output VU meters for both channels.

## Usage
* System shows splash screen with version number at power-up.
* Use the single button to select which screen parameter to edit. Current
parameter highlighted with a magenta box drawn around it.
* Use CV #1 to edit the parameter. There is hysteresis and 'snap' so that you
must move the CV to begin changing the value from the previous setting.
* CV #2 is dedicated to the Wet/Dry mix.

## Building
This project is built using the Raspberry Pi Pico SDK. 

https://github.com/raspberrypi/pico-sdk

Install the SDK and ensure it's working properly, then within this directory

```shell
mkdir build
cd build
cmake ..
make
```

Then either upload the .uf2 file to the RP2040 via USB, or use SWD to install
the .elf file.

