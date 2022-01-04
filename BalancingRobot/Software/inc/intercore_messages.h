#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

//
// Inter-core messages
//   First byte is the message ID
//   Remainder of payload is specific to each message ID
//

//
// Messages from high-level app to bare-metal app
//

// Debug Message being passed from the M4 to the A7
// Payload is 3x float (Pitch, Yaw, Roll)
#define MSG_DEBUG_YPR 0x01
struct DEBUG_YPR {
    uint8_t id; // MSG_DEBUG_YPR
    float pitch;
    float yaw;
    float roll;
    float heading;
    double input;
    double output;
    unsigned long duty;
};


// Debug Message being passed from the M4 to the A7
// Payload is bool - Init Completed/Failed.
#define MSG_DEBUG_INIT 0x02
struct DEBUG_INIT {
    uint8_t id; // MSG_DEBUG_INIT
    bool InitCompleted;
};

// Debug Message being passed from the M4 to the A7
// Payload is ISU Number (0, 1, 2, 3), and 5x uint8_t for found addresses.
#define MSG_DEBUG_I2C_ENUM 0x03
struct DEBUG_I2C_ENUM {
    uint8_t id; // MSG_DEBUG_INIT
    uint8_t ISU_Num;
    uint8_t devices[5];
};

// Debug Message being passed from the M4 to the A7
// Initialization State.
#define MSG_DEBUG_INIT_STATE 0x04
struct DEBUG_INIT_STATE {
    uint8_t id; // MSG_DEBUG_INIT_STATE
    bool ToF_Channel1;
    bool ToF_Channel2;
    bool IMU;
    bool GPIOs;
    bool IMU_Found;
    bool MAG_Found;
};

// WiFi State Message being passed from the A7 to the M4
#define MSG_WIFI_STATE 0x05
struct WIFI_STATE {
    uint8_t id; // MSG_WIFI_STATE
    bool WiFiState;
};

// ToF Data (front == 1, back == 2), distance == mm.
#define MSG_TOF_STATE 0x06
struct TOF_STATE {
    uint8_t id; // MSG_TOF_STATE
    int ToFSensorId;
    int distance;
};

// High Level app - device status.
#define MSG_DEVICE_STATUS 0x07
struct DEVICE_STATUS {
    uint8_t id; // MSG_DEVICE_STATUS
    unsigned long timestamp;
    float setpoint;
    float pitch;
    float yaw;
    float roll;
    double output;
    unsigned long numObstaclesDetected;
    bool avoidActive;
    bool turnNorth;
};

// High Level app - IoTC Message to turn north.
#define MSG_TURN_ROBOT 0x08
struct TURN_ROBOT {
    uint8_t id; // MSG_DEVICE_STATUS
    int heading;
    bool enabled;
};

// ToF counter for obstacles found.
#define MSG_TOF_OBSTACLE 0x09
struct TOF_OBSTACLE {
    uint8_t id; // MSG_TOF_OBSTACLE
    unsigned long ToF_Count;
};

// A7 to M4 telemetry request
#define MSG_TELEMETRY_REQUEST 0x0a
struct TELEMETRY_REQUEST {
    uint8_t id; // MSG_TELEMETRY_REQUEST
};

// A7 to M4 IMU stability request
#define MSG_IMU_STABLE_REQUEST 0x0b
struct IMU_STABLE_REQUEST {
    uint8_t id; // MSG_IMU_STABLE_REQUEST
};

#define MSG_IMU_STABLE_RESULT 0x0c
struct IMU_STABLE_RESULT {
    uint8_t id; // MSG_IMU_STABLE_RESULT
    bool imuStable;
};

#define MSG_SETPOINT 0x0d
struct SETPOINT {
    uint8_t id; // MSG_SETPOINT
    float setpoint;
};

#define MSG_TURN_DETAILS 0x0e
struct TURN_DETAILS
{
    uint8_t id; // MSG_TURN_DETAILS
    float startHeading;
    float endHeading;
};

#define MSG_REMOTE_CMD 0x0f
struct REMOTE_CMD
{
    uint8_t id; // MSG_REMOTE_CMD
    uint8_t cmd;    // 0-4 (left, right, foreward, back, stop).
};

#define MSG_UPDATE_ACTIVE 0x10
struct UPDATE_ACTIVE
{
    uint8_t id; // MSG_UPDATE_ACTIVE
    bool updateActive;
};
