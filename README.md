# Distance‑Display

Custom Zephyr RTOS project for distance sensing using the MB7040 ultrasonic sensor.

## Overview

This project integrates a **custom MB7040 driver** into Zephyr to measure distances and display them via a console or host interface. It’s ideal for embedded systems and RTOS-based sensor applications.

## Features

- MB7040 ultrasonic sensor support via a custom Zephyr driver  
- Real-time distance measurements  
- Configurable sampling intervals  
- Modular structure (drivers, application, config)

## Repo Structure

Distance-Display/
├── drivers/ # Custom MB7040 ultrasound sensor driver
├── app/ # Application code using the driver
├── zephyr/ # Zephyr configuration (prj.conf, overlays)
├── CMakeLists.txt # Project definition
├── Kconfig # Driver configuration options
└── west.yml # West meta for dependencies


## Requirements

- [Zephyr RTOS](https://zephyrproject.org/)
- MB7040 ultrasonic sensor
- Supported development board (e.g., nRF52, STM32)
- West tool
- CMake, Ninja, Python 3.x, etc.

## Setup & Build

1. Clone the repository:
   ```
   west init -m https://github.com/ww3ak/Distance-Display.git
   west update
   cd Distance-Display
   ```
2. Build Project:
  ```
  west build -b <your_board> -d build/
  ```
3. Flash FirmwareL
  ```
west flash
```

## Configuration Options

- CONFIG_MB7040_SAMPLING_INTERVAL   # Time between measurements (in ms)
- CONFIG_MB7040_TRIGGER_PULSE_US    # Trigger pulse duration (microseconds)
- CONFIG_MB7040_UART_OUTPUT         # Enable or disable UART output
```
