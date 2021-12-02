# Overview

Drivers for Azure Sphere MT3620 realtime cores (CM4). These drivers support
the following peripherals:

- [ADC](ADC.h)
- [GPIO](GPIO.h)
- [I2C](I2CMaster.h)
- [I2S](I2S.h)
- [SPI](SPIMaster.h)
- [Timers](GPT.h)
- [UART](UART.h)

The API should be HW agnostic; which is why the MT3620 specifics (register details)
are restricted to the register headers in the `mt3620/` directory. The implementation
in the `.c` files is obviously HW specific.

For detail and documentation on the MT3620; consult
[this Microsoft resource page](https://docs.microsoft.com/en-gb/azure-sphere/hardware/mt3620-product-status).

# Usage

[Here](https://github.com/CodethinkLabs/mt3620-m4-samples) are some example applications using these drivers.

# Documentation

The API for the drivers is documented using XMLDoc. You can use Doxygen to parse these recursively.