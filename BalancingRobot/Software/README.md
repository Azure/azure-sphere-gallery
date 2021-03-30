# Azure Sphere Balancing Robot

The balancing robot uses the high level core for internet communications and deferred update, and one of the real time cores running Azure RTOS for real-time control.

## Contents

| File/folder | Description |
|-------------|-------------|
| `Azure_IoT_Central_Template`       | Azure IoT Central Device Template |
| `HighLevelApp`       | High level application |
| `inc`       | Inter core interface definition |
| `RTOS`       | Real time application |
| `README.md` | This README file. |

## Prerequisites and Setup

The Robot project has two Azure Sphere CMake based applications, one for the high level core, and one that's deployed to one of the real time cores.

The projects are configured to build for tools revision 21.01, and Target API set 8.

For this code to actually work, you will need to build out the robot electronics and enclosure as per the other subdirectories.  

However, you can build and run this software out using any Azure Sphere development board.  If you do this, it will communicate with IoT Central showing default/zero data for most telemetry.

Setup instructions for the demo environment are in the [Documentation/Balancing_Robot_Software_Setup_Guide.docx](../Documentation/Balancing_Robot_Software_Setup_Guide.docx)

## High level application

The high level application is responsible for sending telemetry to Azure IoT Central, handling Azure IoT Central Device Twin messages, and inter-core communications with the real time application.

The [high level application README](./HighLevelApp/README.md) provides more information.

## Real time application

The real time application is responsible for reading the ICM-20948 IMU (accelerometer, gyroscope, magnetometer), Time of Flight sensors, and controlling the motors.

The [real time application README](./RTOS/README.md) provides more information.

## Running the code

Instructions for running the project can be found in the [Documentation/Balancing_Robot_Demo_Guide.docx](../Documentation/Balancing_Robot_Demo_Guide.docx)
