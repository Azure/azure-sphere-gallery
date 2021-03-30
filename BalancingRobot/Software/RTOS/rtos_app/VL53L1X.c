// Most of the functionality of this library is based on the VL53L1X API
// provided by ST (STSW-IMG007), and some of the explanatory comments are quoted
// or paraphrased from the API source code, API user manual (UM2356), and
// VL53L1X datasheet.

#include "VL53L1X_defines.h"
#include "VL53L1X.h"
#include "utils.h"
#include "i2c.h"
#include "tx_api.h"


    // The Arduino two-wire interface uses a 7-bit number for the address,
    // and sets the last bit correctly based on reads and writes
static const uint8_t AddressDefault = 0x29; // 0b0101001;

// value used in measurement timing budget calculations
// assumes PresetMode is LOWPOWER_AUTONOMOUS
//
// vhv = LOWPOWER_AUTO_VHV_LOOP_DURATION_US + LOWPOWERAUTO_VHV_LOOP_BOUND
//       (tuning parm default) * LOWPOWER_AUTO_VHV_LOOP_DURATION_US
//     = 245 + 3 * 245 = 980
// TimingGuard = LOWPOWER_AUTO_OVERHEAD_BEFORE_A_RANGING +
//               LOWPOWER_AUTO_OVERHEAD_BETWEEN_A_B_RANGING + vhv
//             = 1448 + 2100 + 980 = 4528
static const uint32_t TimingGuard = 4528;

// value in DSS_CONFIG__TARGET_TOTAL_RATE_MCPS register, used in DSS
// calculations
static const uint16_t TargetRate = 0x0A00;

// for storing values read from RESULT__RANGE_STATUS (0x0089)
// through RESULT__PEAK_SIGNAL_COUNT_RATE_CROSSTALK_CORRECTED_MCPS_SD0_LOW
// (0x0099)
struct ResultBuffer
{
    uint8_t range_status;
    // uint8_t report_status: not used
    uint8_t stream_count;
    uint16_t dss_actual_effective_spads_sd0;
    // uint16_t peak_signal_count_rate_mcps_sd0: not used
    uint16_t ambient_count_rate_mcps_sd0;
    // uint16_t sigma_sd0: not used
    // uint16_t phase_sd0: not used
    uint16_t final_crosstalk_corrected_range_mm_sd0;
    uint16_t peak_signal_count_rate_crosstalk_corrected_mcps_sd0;
};

// making this static would save RAM for multiple instances as long as there
// aren't multiple sensors being read at the same time (e.g. on separate
// I2C buses)
struct ResultBuffer results;

uint8_t address;

uint8_t activeLaser = 0;    // 0 - front, 1 - back
bool laserCalibrated[2] = { false, false };

uint16_t io_timeout;
bool did_timeout;
uint16_t timeout_start_ms;

uint16_t fast_osc_frequency;
uint16_t osc_calibrate_val;

// bool calibrated;
uint8_t saved_vhv_init;
uint8_t saved_vhv_timeout;

enum DistanceMode distance_mode;

// Record the current time to check an upcoming timeout against
void VL53L1X_startTimeout() { timeout_start_ms = millis(); }

// Check if timeout is enabled (set to nonzero value) and has expired
bool VL53L1X_checkTimeoutExpired() { return (io_timeout > 0) && ((uint16_t)(millis() - timeout_start_ms) > io_timeout); }

void VL53L1X_setupManualCalibration();
void VL53L1X_readResults();
void VL53L1X_updateDSS();
void VL53L1X_getRangingData();

static uint32_t VL53L1X_decodeTimeout(uint16_t reg_val);
static uint16_t VL53L1X_encodeTimeout(uint32_t timeout_mclks);
static uint32_t VL53L1X_timeoutMclksToMicroseconds(uint32_t timeout_mclks, uint32_t macro_period_us);
static uint32_t VL53L1X_timeoutMicrosecondsToMclks(uint32_t timeout_us, uint32_t macro_period_us);
uint32_t VL53L1X_calcMacroPeriod(uint8_t vcsel_period);

// Convert count rate from fixed point 9.7 format to float
float VL53L1X_countRateFixedToFloat(uint16_t count_rate_fixed) { return (float)count_rate_fixed / (1 << 7); }

// Public Methods //////////////////////////////////////////////////////////////

void VL53L1X_setAddress(uint8_t new_addr)
{
  writeRegister(I2C_SLAVE__DEVICE_ADDRESS, new_addr & 0x7F);
  address = new_addr;
}

// Initialize sensor using settings taken mostly from VL53L1_DataInit() and
// VL53L1_StaticInit().
// If io_2v8 (optional) is true or not given, the sensor is configured for 2V8
// mode.

bool VL53L1X_init(bool io_2v8)
{
    address = AddressDefault;
    io_timeout = 0; // no timeout
    did_timeout = false;
    // calibrated = false;
    saved_vhv_init = 0;
    saved_vhv_timeout = 0;
    distance_mode = Unknown;

    laserCalibrated[activeLaser] = false;

  // check model ID and module type registers (values specified in datasheet)
  if (readRegister16(IDENTIFICATION__MODEL_ID) != 0xEACC) { return false; }

  // VL53L1_software_reset() begin

  writeRegister(SOFT_RESET, 0x00);
  delay(1); // delayMicroseconds(100);
  writeRegister(SOFT_RESET, 0x01);

  // give it some time to boot; otherwise the sensor NACKs during the readReg()
  // call below and the Arduino 101 doesn't seem to handle that well
  delay(1);

  // VL53L1_poll_for_boot_completion() begin

  VL53L1X_startTimeout();

  // check last_status in case we still get a NACK to try to deal with it correctly
  while ((readRegister(FIRMWARE__SYSTEM_STATUS) & 0x01) == 0 || last_status != 0)
  {
    if (VL53L1X_checkTimeoutExpired())
    {
      did_timeout = true;
      return false;
    }
  }
  // VL53L1_poll_for_boot_completion() end

  // VL53L1_software_reset() end

  // VL53L1_DataInit() begin

  // sensor uses 1V8 mode for I/O by default; switch to 2V8 mode if necessary
  if (io_2v8)
  {
    writeRegister(PAD_I2C_HV__EXTSUP_CONFIG,
    readRegister(PAD_I2C_HV__EXTSUP_CONFIG) | 0x01);
  }

  // store oscillator info for later use
  fast_osc_frequency = readRegister16(OSC_MEASURED__FAST_OSC__FREQUENCY);
  osc_calibrate_val = readRegister16(RESULT__OSC_CALIBRATE_VAL);

  // VL53L1_DataInit() end

  // VL53L1_StaticInit() begin

  // Note that the API does not actually apply the configuration settings below
  // when VL53L1_StaticInit() is called: it keeps a copy of the sensor's
  // register contents in memory and doesn't actually write them until a
  // measurement is started. Writing the configuration here means we don't have
  // to keep it all in memory and avoids a lot of redundant writes later.

  // the API sets the preset mode to LOWPOWER_AUTONOMOUS here:
  // VL53L1_set_preset_mode() begin

  // VL53L1_preset_mode_standard_ranging() begin

  // values labeled "tuning parm default" are from vl53l1_tuning_parm_defaults.h
  // (API uses these in VL53L1_init_tuning_parm_storage_struct())

  // static config
  // API resets PAD_I2C_HV__EXTSUP_CONFIG here, but maybe we don't want to do
  // that? (seems like it would disable 2V8 mode)
  writeRegister16(DSS_CONFIG__TARGET_TOTAL_RATE_MCPS, TargetRate); // should already be this value after reset
  writeRegister(GPIO__TIO_HV_STATUS, 0x02);
  writeRegister(SIGMA_ESTIMATOR__EFFECTIVE_PULSE_WIDTH_NS, 8); // tuning parm default
  writeRegister(SIGMA_ESTIMATOR__EFFECTIVE_AMBIENT_WIDTH_NS, 16); // tuning parm default
  writeRegister(ALGO__CROSSTALK_COMPENSATION_VALID_HEIGHT_MM, 0x01);
  writeRegister(ALGO__RANGE_IGNORE_VALID_HEIGHT_MM, 0xFF);
  writeRegister(ALGO__RANGE_MIN_CLIP, 0); // tuning parm default
  writeRegister(ALGO__CONSISTENCY_CHECK__TOLERANCE, 2); // tuning parm default

  // general config
  writeRegister16(SYSTEM__THRESH_RATE_HIGH, 0x0000);
  writeRegister16(SYSTEM__THRESH_RATE_LOW, 0x0000);
  writeRegister(DSS_CONFIG__APERTURE_ATTENUATION, 0x38);

  // timing config
  // most of these settings will be determined later by distance and timing
  // budget configuration
  writeRegister16(RANGE_CONFIG__SIGMA_THRESH, 360); // tuning parm default
  writeRegister16(RANGE_CONFIG__MIN_COUNT_RATE_RTN_LIMIT_MCPS, 192); // tuning parm default

  // dynamic config

  writeRegister(SYSTEM__GROUPED_PARAMETER_HOLD_0, 0x01);
  writeRegister(SYSTEM__GROUPED_PARAMETER_HOLD_1, 0x01);
  writeRegister(SD_CONFIG__QUANTIFIER, 2); // tuning parm default

  // VL53L1_preset_mode_standard_ranging() end

  // from VL53L1_preset_mode_timed_ranging_*
  // GPH is 0 after reset, but writing GPH0 and GPH1 above seem to set GPH to 1,
  // and things don't seem to work if we don't set GPH back to 0 (which the API
  // does here).
  writeRegister(SYSTEM__GROUPED_PARAMETER_HOLD, 0x00);
  writeRegister(SYSTEM__SEED_CONFIG, 1); // tuning parm default

  // from VL53L1_config_low_power_auto_mode
  writeRegister(SYSTEM__SEQUENCE_CONFIG, 0x8B); // VHV, PHASECAL, DSS1, RANGE
  writeRegister16(DSS_CONFIG__MANUAL_EFFECTIVE_SPADS_SELECT, 200 << 8);
  writeRegister(DSS_CONFIG__ROI_MODE_CONTROL, 2); // REQUESTED_EFFFECTIVE_SPADS

  // VL53L1_set_preset_mode() end

  // default to long range, 50 ms timing budget
  // note that this is different than what the API defaults to
  VL53L1X_setDistanceMode(Long);
  VL53L1X_setMeasurementTimingBudget(50000);

  // VL53L1_StaticInit() end

  // the API triggers this change in VL53L1_init_and_start_range() once a
  // measurement is started; assumes MM1 and MM2 are disabled
  writeRegister16(ALGO__PART_TO_PART_RANGE_OFFSET_MM,
    readRegister16(MM_CONFIG__OUTER_OFFSET_MM) * 4);

  return true;
}

// Write an 8-bit register
void VL53L1X_writeRegister(uint16_t reg, uint8_t value)
{
    writeRegister(reg, value);
}

// Write a 16-bit register
void VL53L1X_writeRegister16(uint16_t reg, uint16_t value)
{
    writeRegister16(reg, value);
}

// Write a 32-bit register
void VL53L1X_writeReg32Bit(uint16_t reg, uint32_t value)
{
    writeRegister32(reg, value);
}

// Read an 8-bit register
uint8_t VL53L1X_readReg(enum regAddr reg)
{
  uint8_t value;

  value = readRegister(reg);
  return value;
}

// Read a 16-bit register
uint16_t VL53L1X_readRegister16(uint16_t reg)
{
  uint16_t value;

  value = readRegister16(reg);

  return value;
}

// Read a 32-bit register
uint32_t VL53L1X_readReg32Bit(uint16_t reg)
{
    uint32_t value;
    value = readRegister32(reg);

    return value;
}

// set distance mode to Short, Medium, or Long
// based on VL53L1_SetDistanceMode()
bool VL53L1X_setDistanceMode(enum DistanceMode mode)
{
  // save existing timing budget
  uint32_t budget_us = VL53L1X_getMeasurementTimingBudget();

  switch (mode)
  {
    case Short:
      // from VL53L1_preset_mode_standard_ranging_short_range()

      // timing config
      writeRegister(RANGE_CONFIG__VCSEL_PERIOD_A, 0x07);
      writeRegister(RANGE_CONFIG__VCSEL_PERIOD_B, 0x05);
      writeRegister(RANGE_CONFIG__VALID_PHASE_HIGH, 0x38);

      // dynamic config
      writeRegister(SD_CONFIG__WOI_SD0, 0x07);
      writeRegister(SD_CONFIG__WOI_SD1, 0x05);
      writeRegister(SD_CONFIG__INITIAL_PHASE_SD0, 6); // tuning parm default
      writeRegister(SD_CONFIG__INITIAL_PHASE_SD1, 6); // tuning parm default

      break;

    case Medium:
      // from VL53L1_preset_mode_standard_ranging()

      // timing config
      writeRegister(RANGE_CONFIG__VCSEL_PERIOD_A, 0x0B);
      writeRegister(RANGE_CONFIG__VCSEL_PERIOD_B, 0x09);
      writeRegister(RANGE_CONFIG__VALID_PHASE_HIGH, 0x78);

      // dynamic config
      writeRegister(SD_CONFIG__WOI_SD0, 0x0B);
      writeRegister(SD_CONFIG__WOI_SD1, 0x09);
      writeRegister(SD_CONFIG__INITIAL_PHASE_SD0, 10); // tuning parm default
      writeRegister(SD_CONFIG__INITIAL_PHASE_SD1, 10); // tuning parm default

      break;

    case Long: // long
      // from VL53L1_preset_mode_standard_ranging_long_range()

      // timing config
      writeRegister(RANGE_CONFIG__VCSEL_PERIOD_A, 0x0F);
      writeRegister(RANGE_CONFIG__VCSEL_PERIOD_B, 0x0D);
      writeRegister(RANGE_CONFIG__VALID_PHASE_HIGH, 0xB8);

      // dynamic config
      writeRegister(SD_CONFIG__WOI_SD0, 0x0F);
      writeRegister(SD_CONFIG__WOI_SD1, 0x0D);
      writeRegister(SD_CONFIG__INITIAL_PHASE_SD0, 14); // tuning parm default
      writeRegister(SD_CONFIG__INITIAL_PHASE_SD1, 14); // tuning parm default

      break;

    default:
      // unrecognized mode - do nothing
      return false;
  }

  // reapply timing budget
  VL53L1X_setMeasurementTimingBudget(budget_us);

  // save mode so it can be returned by getDistanceMode()
  distance_mode = mode;

  return true;
}

// Set the measurement timing budget in microseconds, which is the time allowed
// for one measurement. A longer timing budget allows for more accurate
// measurements.
// based on VL53L1_SetMeasurementTimingBudgetMicroSeconds()
bool VL53L1X_setMeasurementTimingBudget(uint32_t budget_us)
{
  // assumes PresetMode is LOWPOWER_AUTONOMOUS

  if (budget_us <= TimingGuard) { return false; }

  uint32_t range_config_timeout_us = budget_us -= TimingGuard;
  if (range_config_timeout_us > 1100000) { return false; } // FDA_MAX_TIMING_BUDGET_US * 2

  range_config_timeout_us /= 2;

  // VL53L1_calc_timeout_register_values() begin

  uint32_t macro_period_us;

  // "Update Macro Period for Range A VCSEL Period"
  macro_period_us = VL53L1X_calcMacroPeriod(readRegister(RANGE_CONFIG__VCSEL_PERIOD_A));

  // "Update Phase timeout - uses Timing A"
  // Timeout of 1000 is tuning parm default (TIMED_PHASECAL_CONFIG_TIMEOUT_US_DEFAULT)
  // via VL53L1_get_preset_mode_timing_cfg().
  uint32_t phasecal_timeout_mclks = VL53L1X_timeoutMicrosecondsToMclks(1000, macro_period_us);
  if (phasecal_timeout_mclks > 0xFF) { phasecal_timeout_mclks = 0xFF; }
  writeRegister(PHASECAL_CONFIG__TIMEOUT_MACROP, phasecal_timeout_mclks);

  // "Update MM Timing A timeout"
  // Timeout of 1 is tuning parm default (LOWPOWERAUTO_MM_CONFIG_TIMEOUT_US_DEFAULT)
  // via VL53L1_get_preset_mode_timing_cfg(). With the API, the register
  // actually ends up with a slightly different value because it gets assigned,
  // retrieved, recalculated with a different macro period, and reassigned,
  // but it probably doesn't matter because it seems like the MM ("mode
  // mitigation"?) sequence steps are disabled in low power auto mode anyway.
  writeRegister16(MM_CONFIG__TIMEOUT_MACROP_A, VL53L1X_encodeTimeout(
      VL53L1X_timeoutMicrosecondsToMclks(1, macro_period_us)));

  // "Update Range Timing A timeout"
  writeRegister16(RANGE_CONFIG__TIMEOUT_MACROP_A, VL53L1X_encodeTimeout(
      VL53L1X_timeoutMicrosecondsToMclks(range_config_timeout_us, macro_period_us)));

  // "Update Macro Period for Range B VCSEL Period"
  macro_period_us = VL53L1X_calcMacroPeriod(readRegister(RANGE_CONFIG__VCSEL_PERIOD_B));

  // "Update MM Timing B timeout"
  // (See earlier comment about MM Timing A timeout.)
  writeRegister16(MM_CONFIG__TIMEOUT_MACROP_B, VL53L1X_encodeTimeout(
      VL53L1X_timeoutMicrosecondsToMclks(1, macro_period_us)));

  // "Update Range Timing B timeout"
  writeRegister16(RANGE_CONFIG__TIMEOUT_MACROP_B, VL53L1X_encodeTimeout(
      VL53L1X_timeoutMicrosecondsToMclks(range_config_timeout_us, macro_period_us)));

  // VL53L1_calc_timeout_register_values() end

  return true;
}

// Get the measurement timing budget in microseconds
// based on VL53L1_SetMeasurementTimingBudgetMicroSeconds()
uint32_t VL53L1X_getMeasurementTimingBudget()
{
  // assumes PresetMode is LOWPOWER_AUTONOMOUS and these sequence steps are
  // enabled: VHV, PHASECAL, DSS1, RANGE

  // VL53L1_get_timeouts_us() begin

  // "Update Macro Period for Range A VCSEL Period"
  uint32_t macro_period_us = VL53L1X_calcMacroPeriod(readRegister(RANGE_CONFIG__VCSEL_PERIOD_A));

  // "Get Range Timing A timeout"

  uint32_t range_config_timeout_us = VL53L1X_timeoutMclksToMicroseconds(VL53L1X_decodeTimeout(
    readRegister16(RANGE_CONFIG__TIMEOUT_MACROP_A)), macro_period_us);

  // VL53L1_get_timeouts_us() end

  return  2 * range_config_timeout_us + TimingGuard;
}

// Start continuous ranging measurements, with the given inter-measurement
// period in milliseconds determining how often the sensor takes a measurement.
void VL53L1X_startContinuous(uint32_t period_ms)
{
  // from VL53L1_set_inter_measurement_period_ms()
  writeRegister32(SYSTEM__INTERMEASUREMENT_PERIOD, period_ms * osc_calibrate_val);

  writeRegister(SYSTEM__INTERRUPT_CLEAR, 0x01); // sys_interrupt_clear_range
  writeRegister(SYSTEM__MODE_START, 0x40); // mode_range__timed
}

// Stop continuous measurements
// based on VL53L1_stop_range()
void VL53L1X_stopContinuous()
{
  writeRegister(SYSTEM__MODE_START, 0x80); // mode_range__abort

  // VL53L1_low_power_auto_data_stop_range() begin

  laserCalibrated[activeLaser] = false;
  // calibrated = false;

  // "restore vhv configs"
  if (saved_vhv_init != 0)
  {
    writeRegister(VHV_CONFIG__INIT, saved_vhv_init);
  }
  if (saved_vhv_timeout != 0)
  {
     writeRegister(VHV_CONFIG__TIMEOUT_MACROP_LOOP_BOUND, saved_vhv_timeout);
  }

  // "remove phasecal override"
  writeRegister(PHASECAL_CONFIG__OVERRIDE, 0x00);

  // VL53L1_low_power_auto_data_stop_range() end
}

// Returns a range reading in millimeters when continuous mode is active
// (readRangeSingleMillimeters() also calls this function after starting a
// single-shot range measurement)
uint16_t VL53L1X_read(bool blocking)
{
  if (blocking)
  {
      VL53L1X_startTimeout();
    while (!VL53L1X_dataReady())
    {
      if (VL53L1X_checkTimeoutExpired())
      {
        did_timeout = true;
        ranging_data.range_status = None;
        ranging_data.range_mm = 0;
        ranging_data.peak_signal_count_rate_MCPS = 0;
        ranging_data.ambient_count_rate_MCPS = 0;
        return ranging_data.range_mm;
      }
    }
  }

  VL53L1X_readResults();
  if (!laserCalibrated[activeLaser])
  {
      VL53L1X_setupManualCalibration();
      laserCalibrated[activeLaser] = true;
  }

  VL53L1X_updateDSS();

  VL53L1X_getRangingData();

  writeRegister(SYSTEM__INTERRUPT_CLEAR, 0x01); // sys_interrupt_clear_range

  return ranging_data.range_mm;
}

// convert a RangeStatus to a readable string
// Note that on an AVR, these strings are stored in RAM (dynamic memory), which
// makes working with them easier but uses up 200+ bytes of RAM (many AVR-based
// Arduinos only have about 2000 bytes of RAM). You can avoid this memory usage
// if you do not call this function in your sketch.
//const char * VL53L1X_rangeStatusToString(enum RangeStatus status)
//{
//  switch (status)
//  {
//    case RangeValid:
//      return "range valid";
//
//    case SigmaFail:
//      return "sigma fail";
//
//    case SignalFail:
//      return "signal fail";
//
//    case RangeValidMinRangeClipped:
//      return "range valid, min range clipped";
//
//    case OutOfBoundsFail:
//      return "out of bounds fail";
//
//    case HardwareFail:
//      return "hardware fail";
//
//    case RangeValidNoWrapCheckFail:
//      return "range valid, no wrap check fail";
//
//    case WrapTargetFail:
//      return "wrap target fail";
//
//    case XtalkSignalFail:
//      return "xtalk signal fail";
//
//    case SynchronizationInt:
//      return "synchronization int";
//
//    case MinRangeFail:
//      return "min range fail";
//
//    case None:
//      return "no update";
//
//    default:
//      return "unknown status";
//  }
//}

// Did a timeout occur in one of the read functions since the last call to
// timeoutOccurred()?
bool VL53L1X_timeoutOccurred()
{
  bool tmp = did_timeout;
  did_timeout = false;
  return tmp;
}

// Private Methods /////////////////////////////////////////////////////////////

// "Setup ranges after the first one in low power auto mode by turning off
// FW calibration steps and programming static values"
// based on VL53L1_low_power_auto_setup_manual_calibration()
void VL53L1X_setupManualCalibration()
{
  // "save original vhv configs"
  saved_vhv_init = readRegister(VHV_CONFIG__INIT);
  saved_vhv_timeout = readRegister(VHV_CONFIG__TIMEOUT_MACROP_LOOP_BOUND);

  // "disable VHV init"
  writeRegister(VHV_CONFIG__INIT, saved_vhv_init & 0x7F);

  // "set loop bound to tuning param"
  writeRegister(VHV_CONFIG__TIMEOUT_MACROP_LOOP_BOUND,
    (saved_vhv_timeout & 0x03) + (3 << 2)); // tuning parm default (LOWPOWERAUTO_VHV_LOOP_BOUND_DEFAULT)

  // "override phasecal"
  writeRegister(PHASECAL_CONFIG__OVERRIDE, 0x01);
  writeRegister(CAL_CONFIG__VCSEL_START, readRegister(PHASECAL_RESULT__VCSEL_START));
}

// read measurement results into buffer
void VL53L1X_readResults()
{
    uint8_t txBuffer[20];
    uint8_t rxBuffer[20];

    txBuffer[0] = (RESULT__RANGE_STATUS >> 8) & 0xFF;
    txBuffer[1] = RESULT__RANGE_STATUS & 0xFF;

    int result = readBlockData(txBuffer, rxBuffer, 2, 17);

    if (result == -1)
    {
        printf("VL53L1X_readResults - failed\n");
        return;
    }

  //Wire.beginTransmission(address);
  //Wire.write((RESULT__RANGE_STATUS >> 8) & 0xFF); // reg high byte
  //Wire.write( RESULT__RANGE_STATUS       & 0xFF); // reg low byte
  //last_status = Wire.endTransmission();

    int offset = 0;

  //Wire.requestFrom(address, (uint8_t)17);

  //results.range_status = Wire.read();
    results.range_status = rxBuffer[offset++];

  //Wire.read(); // report_status: not used
    offset++;

  //results.stream_count = Wire.read();
    results.stream_count = rxBuffer[offset++];

  //results.dss_actual_effective_spads_sd0  = (uint16_t)Wire.read() << 8; // high byte
  //results.dss_actual_effective_spads_sd0 |=           Wire.read();      // low byte
    results.dss_actual_effective_spads_sd0 = (uint16_t)rxBuffer[offset++] << 8; // high byte
    results.dss_actual_effective_spads_sd0 |= rxBuffer[offset++]; // low byte

  //Wire.read(); // peak_signal_count_rate_mcps_sd0: not used
    offset++;
    //Wire.read();
    offset++;

  //results.ambient_count_rate_mcps_sd0  = (uint16_t)Wire.read() << 8; // high byte
  //results.ambient_count_rate_mcps_sd0 |=           Wire.read();      // low byte
  results.ambient_count_rate_mcps_sd0  = (uint16_t)rxBuffer[offset++] << 8; // high byte
  results.ambient_count_rate_mcps_sd0 |= rxBuffer[offset++];      // low byte

  //Wire.read(); // sigma_sd0: not used
  offset++;
  //Wire.read();
  offset++;

  //Wire.read(); // phase_sd0: not used
  offset++;
  //Wire.read();
  offset++;

  //results.final_crosstalk_corrected_range_mm_sd0  = (uint16_t)Wire.read() << 8; // high byte
  //results.final_crosstalk_corrected_range_mm_sd0 |=           Wire.read();      // low byte
  results.final_crosstalk_corrected_range_mm_sd0  = (uint16_t)rxBuffer[offset++] << 8; // high byte
  results.final_crosstalk_corrected_range_mm_sd0 |= rxBuffer[offset++];      // low byte

  //results.peak_signal_count_rate_crosstalk_corrected_mcps_sd0  = (uint16_t)Wire.read() << 8; // high byte
  //results.peak_signal_count_rate_crosstalk_corrected_mcps_sd0 |=           Wire.read();      // low byte
  results.peak_signal_count_rate_crosstalk_corrected_mcps_sd0  = (uint16_t)rxBuffer[offset++] << 8; // high byte
  results.peak_signal_count_rate_crosstalk_corrected_mcps_sd0 |= rxBuffer[offset++];      // low byte
}

// perform Dynamic SPAD Selection calculation/update
// based on VL53L1_low_power_auto_update_DSS()
void VL53L1X_updateDSS()
{
  uint16_t spadCount = results.dss_actual_effective_spads_sd0;

  if (spadCount != 0)
  {
    // "Calc total rate per spad"

    uint32_t totalRatePerSpad =
      (uint32_t)results.peak_signal_count_rate_crosstalk_corrected_mcps_sd0 +
      results.ambient_count_rate_mcps_sd0;

    // "clip to 16 bits"
    if (totalRatePerSpad > 0xFFFF) { totalRatePerSpad = 0xFFFF; }

    // "shift up to take advantage of 32 bits"
    totalRatePerSpad <<= 16;

    totalRatePerSpad /= spadCount;

    if (totalRatePerSpad != 0)
    {
      // "get the target rate and shift up by 16"
      uint32_t requiredSpads = ((uint32_t)TargetRate << 16) / totalRatePerSpad;

      // "clip to 16 bit"
      if (requiredSpads > 0xFFFF) { requiredSpads = 0xFFFF; }

      // "override DSS config"
      writeRegister16(DSS_CONFIG__MANUAL_EFFECTIVE_SPADS_SELECT, requiredSpads);
      // DSS_CONFIG__ROI_MODE_CONTROL should already be set to REQUESTED_EFFFECTIVE_SPADS

      return;
    }
  }

  // If we reached this point, it means something above would have resulted in a
  // divide by zero.
  // "We want to gracefully set a spad target, not just exit with an error"

   // "set target to mid point"
   writeRegister16(DSS_CONFIG__MANUAL_EFFECTIVE_SPADS_SELECT, 0x8000);
}

// get range, status, rates from results buffer
// based on VL53L1_GetRangingMeasurementData()
void VL53L1X_getRangingData()
{
  // VL53L1_copy_sys_and_core_results_to_range_results() begin

  uint16_t range = results.final_crosstalk_corrected_range_mm_sd0;

  // "apply correction gain"
  // gain factor of 2011 is tuning parm default (VL53L1_TUNINGPARM_LITE_RANGING_GAIN_FACTOR_DEFAULT)
  // Basically, this appears to scale the result by 2011/2048, or about 98%
  // (with the 1024 added for proper rounding).
  ranging_data.range_mm = ((uint32_t)range * 2011 + 0x0400) / 0x0800;

  // VL53L1_copy_sys_and_core_results_to_range_results() end

  // set range_status in ranging_data based on value of RESULT__RANGE_STATUS register
  // mostly based on ConvertStatusLite()
  switch(results.range_status)
  {
    case 17: // MULTCLIPFAIL
    case 2: // VCSELWATCHDOGTESTFAILURE
    case 1: // VCSELCONTINUITYTESTFAILURE
    case 3: // NOVHVVALUEFOUND
      // from SetSimpleData()
      ranging_data.range_status = HardwareFail;
      break;

    case 13: // USERROICLIP
     // from SetSimpleData()
      ranging_data.range_status = MinRangeFail;
      break;

    case 18: // GPHSTREAMCOUNT0READY
      ranging_data.range_status = SynchronizationInt;
      break;

    case 5: // RANGEPHASECHECK
      ranging_data.range_status =  OutOfBoundsFail;
      break;

    case 4: // MSRCNOTARGET
      ranging_data.range_status = SignalFail;
      break;

    case 6: // SIGMATHRESHOLDCHECK
      ranging_data.range_status = SignalFail;
      break;

    case 7: // PHASECONSISTENCY
      ranging_data.range_status = WrapTargetFail;
      break;

    case 12: // RANGEIGNORETHRESHOLD
      ranging_data.range_status = XtalkSignalFail;
      break;

    case 8: // MINCLIP
      ranging_data.range_status = RangeValidMinRangeClipped;
      break;

    case 9: // RANGECOMPLETE
      // from VL53L1_copy_sys_and_core_results_to_range_results()
      if (results.stream_count == 0)
      {
        ranging_data.range_status = RangeValidNoWrapCheckFail;
      }
      else
      {
        ranging_data.range_status = RangeValid;
      }
      break;

    default:
      ranging_data.range_status = None;
  }

  // from SetSimpleData()
  ranging_data.peak_signal_count_rate_MCPS =
      VL53L1X_countRateFixedToFloat(results.peak_signal_count_rate_crosstalk_corrected_mcps_sd0);
  ranging_data.ambient_count_rate_MCPS =
      VL53L1X_countRateFixedToFloat(results.ambient_count_rate_mcps_sd0);
}

// Decode sequence step timeout in MCLKs from register value
// based on VL53L1_decode_timeout()
uint32_t VL53L1X_decodeTimeout(uint16_t reg_val)
{
  return ((uint32_t)(reg_val & 0xFF) << (reg_val >> 8)) + 1;
}

// Encode sequence step timeout register value from timeout in MCLKs
// based on VL53L1_encode_timeout()
uint16_t VL53L1X_encodeTimeout(uint32_t timeout_mclks)
{
  // encoded format: "(LSByte * 2^MSByte) + 1"

  uint32_t ls_byte = 0;
  uint16_t ms_byte = 0;

  if (timeout_mclks > 0)
  {
    ls_byte = timeout_mclks - 1;

    while ((ls_byte & 0xFFFFFF00) > 0)
    {
      ls_byte >>= 1;
      ms_byte++;
    }

    return (ms_byte << 8) | (ls_byte & 0xFF);
  }
  else { return 0; }
}

// Convert sequence step timeout from macro periods to microseconds with given
// macro period in microseconds (12.12 format)
// based on VL53L1_calc_timeout_us()
uint32_t VL53L1X_timeoutMclksToMicroseconds(uint32_t timeout_mclks, uint32_t macro_period_us)
{
  return ((uint64_t)timeout_mclks * macro_period_us + 0x800) >> 12;
}

// Convert sequence step timeout from microseconds to macro periods with given
// macro period in microseconds (12.12 format)
// based on VL53L1_calc_timeout_mclks()
uint32_t VL53L1X_timeoutMicrosecondsToMclks(uint32_t timeout_us, uint32_t macro_period_us)
{
  return (((uint32_t)timeout_us << 12) + (macro_period_us >> 1)) / macro_period_us;
}

// Calculate macro period in microseconds (12.12 format) with given VCSEL period
// assumes fast_osc_frequency has been read and stored
// based on VL53L1_calc_macro_period_us()
uint32_t VL53L1X_calcMacroPeriod(uint8_t vcsel_period)
{
  // from VL53L1_calc_pll_period_us()
  // fast osc frequency in 4.12 format; PLL period in 0.24 format
  uint32_t pll_period_us = ((uint32_t)0x01 << 30) / fast_osc_frequency;

  // from VL53L1_decode_vcsel_period()
  uint8_t vcsel_period_pclks = (vcsel_period + 1) << 1;

  // VL53L1_MACRO_PERIOD_VCSEL_PERIODS = 2304
  uint32_t macro_period_us = (uint32_t)2304 * pll_period_us;
  macro_period_us >>= 6;
  macro_period_us *= vcsel_period_pclks;
  macro_period_us >>= 6;

  return macro_period_us;
}

uint8_t VL53L1X_getAddress(void) 
{ 
    return address; 
}

enum DistanceMode VL53L1X_getDistanceMode() 
{ 
    return distance_mode; 
}

uint16_t VL53L1X_readRangeContinuousMillimeters(bool blocking) 
{ 
    return VL53L1X_read(blocking);
} // alias of read()

bool VL53L1X_dataReady(void) 
{ 
    return (readRegister(GPIO__TIO_HV_STATUS) & 0x01) == 0;
}


void VL53L1X_setTimeout(uint16_t timeout)
{ 
    io_timeout = timeout; 
}

uint16_t VL53L1X_getTimeout(void) 
{ 
    return io_timeout; 
}

void VL53L1X_setActiveLaser(uint8_t laserNum)
{
    if (laserNum == 0 || laserNum == 1)
    {
        activeLaser = laserNum;
    }
}