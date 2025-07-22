# Zephyr I2C Example App

## Overview
This is a basic Zephyr application demonstrating I2C communication on the ESP32-S3 development board. The app initializes the I2C bus, communicates with an I2C slave device, and prints debug output to the serial console.

## Board
- ESP32-S3 DevKitC

## Build and Flash
1. Setup Zephyr environemnt (west, Zephyr SDK, etc.)
2. Clone git repo ://git.epam.com/tomislav_radanovic/sabrina-internship.git
3. Run:  west update
4. Build app for board: west build -b esp32s3_devkitc \app\i2c_test
5. Flash board: west flash

## View Output
- To view serial output use: west espressif monitor


