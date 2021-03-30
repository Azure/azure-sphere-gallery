## Real time app 
The real time application is responsible for reading sensors (Time of Flight lasers for distance sensing, ICM-20948 TDK/Invensense Accelerometer, Gyroscope, Magnetometer), generating Yaw, Pitch, and Roll (from the IMU using the Digital Motion Processor or software algorithms), and controlling the motors (direction and speed).

Note that there are two Time of Flight sensors, both have the same I2C address, the design uses an I2C 'fan out' chip to control which of the Time of Flight sensors is currently selected. 

The real-time software uses two [PID controllers](https://en.wikipedia.org/wiki/PID_controller#:~:text=A%20proportional%E2%80%93integral%E2%80%93derivative%20controller%20(PID%20controller%20or%20three-term%20controller),A%20PID%20controller%20continuously%20calculates%20an%20error%20value) (Proportional, Integral, Derivative), sometimes referred to as cascading PID controllers, one controls speed, the other controls the balance point of the robot. This [YouTube video](https://www.youtube.com/watch?v=uyHdyF0_BFo) shows the impact of tuning the P, I, and D values. 

# Build instructions
The real time application uses git submodules to pull in versions of [Azure RTOS (threadx)](https://azure.microsoft.com/services/rtos/) and [MTK libraries for the M4 on the MT3620](https://github.com/MediaTek-Labs/mt3620_m4_software).  To build the app, you must ensure to pull in the submodules:

```
git submodule init
git pull --recurse-submodules
```

The real time application uses a [TDK/Invensense ICM-20948](https://invensense.tdk.com/products/motion-tracking/9-axis/icm-20948/) 9-axis [IMU](https://en.wikipedia.org/wiki/Inertial_measurement_unit). The real time application does not include the software to support the ICM-29048 IMU directly. TDK have an SDK/sample port for their IMU that includes support for the Digital Motion Processor (DMP) which we have ported for integration, but the license for that code does not allow us to share the port. You will need to either port the TDK IMU sample to Azure Sphere, or potentially use other sample code that supports the ICM-20948 IMU.

There are two locations in rtos_app.c that show where you will need to initialize your IMU code, and where you need to read the IMU and convert the DMP generated [quaternions](https://en.wikipedia.org/wiki/Quaternion) to Yaw, Pitch, and Roll - these are:

`Line 207: // TODO: Read IMU, calculate Yaw, Pitch, Roll`

`Line 457: // TODO: Initialize the IMU here`

The real time application uses a 1ms tick, the default Azure RTOS timer tick is 10ms, you will need to make a change to the Azure RTOS tx_api.h file in threadx/common/inc - change line 204 to read:

`#define TX_TIMER_TICKS_PER_SECOND       ((ULONG) 1000)`

Note that the file `rtos_app/tx_initialize_low_level.S` will also need to be modified if you need to change the timer tick, look for the line `SYSTICK_CYCLES` this has a default value of 100 to give 10ms tick.


## Header/C files

The application uses several header/c pairs, these are:

| Header/C files | Description |
|-------------|-------------|
| FanOut | Select the front/rear facing Time of Flight laser |
| i2c | Functions for reading/writing to I2C devices |
| PID and PID_v1 | PID Controller implementation |
| utils | Contains functions to: get the current millisecond tick, dump buffer contents in Hex/Ascii, and function prototypes |
| VL53L1X | Code for setting up and reading values from the VL53L1X Time of Flight sensors |
