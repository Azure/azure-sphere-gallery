# Industrial Device Controller

The industrial device controller uses the high level application and the real-time capable application for collecting the telemetry through MODBUS protocol and ingesting into IoTHub.

## Contents

| File/folder | Description |
|-------------|-------------|
| `HighLevelApp`       | High level application |
| `MT3620_IDC_RTApp`   | Real-time capable application |
| `README.md` | This README file |

## High level application
The high level application is responsible for collecting the telemetry through MODBUS protocol and ingesting into IoTHub.
The [high level application README](./HighLevelApp/README.md) provides more information.

## Real-time capable application
The real-time application is responsible for read and write the modbus messages through UART.
The [Real-time capable application README](./MT3620_IDC_RTApp/README.md) provides more information.