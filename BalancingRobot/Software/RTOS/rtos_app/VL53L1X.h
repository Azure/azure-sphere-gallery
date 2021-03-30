#pragma once
#include <stdint.h>
#include <stdbool.h>

#include "VL53L1X_defines.h"


    struct RangingData ranging_data;

    uint8_t last_status; // status of last I2C transmission

    void VL53L1X_setAddress(uint8_t new_addr);
    uint8_t VL53L1X_getAddress(void);

    bool VL53L1X_init(bool io_2v8); // io_2v8 defaulted to true in the sample.
    void VL53L1X_setActiveLaser(uint8_t laserNum);  // 0 - front, 1 - back.

    //void VL53L1X_writeReg(uint16_t reg, uint8_t value);
    //void VL53L1X_writeReg16Bit(uint16_t reg, uint16_t value);
    //void VL53L1X_writeReg32Bit(uint16_t reg, uint32_t value);
    //uint8_t VL53L1X_readReg(enum regAddr reg);
    //uint16_t VL53L1X_readReg16Bit(uint16_t reg);
    //uint32_t VL53L1X_readReg32Bit(uint16_t reg);

    bool VL53L1X_setDistanceMode(enum DistanceMode mode);
    enum DistanceMode VL53L1X_getDistanceMode(void);

    bool VL53L1X_setMeasurementTimingBudget(uint32_t budget_us);
    uint32_t VL53L1X_getMeasurementTimingBudget();

    void VL53L1X_startContinuous(uint32_t period_ms);
    void VL53L1X_stopContinuous();
    uint16_t VL53L1X_read(bool blocking);   // defaults to  = true
    uint16_t VL53L1X_readRangeContinuousMillimeters(bool blocking);

    // check if sensor has new reading available
    // assumes interrupt is active low (GPIO_HV_MUX__CTRL bit 4 is 1)
    bool VL53L1X_dataReady(void);

    //static const char * VL53L1X_rangeStatusToString(enum RangeStatus status);

    void VL53L1X_setTimeout(uint16_t timeout);
    uint16_t VL53L1X_getTimeout(void);
    bool VL53L1X_timeoutOccurred(void);

