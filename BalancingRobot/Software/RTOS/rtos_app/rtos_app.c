#include <stdbool.h>
#include <math.h>
#include <stdint.h>

#include "i2c.h"
#include "mt3620-intercore.h"
#include "intercore_messages.h"
#include "os_hal_gpio.h"
#include "os_hal_uart.h"
#include "os_hal_pwm.h"
#include "os_hal_adc.h"

#include "printf.h"
#include "tx_api.h"

#include "utils.h"
#include "PID_v1.h"
#include "PID.h"

#include "FanOut.h"
#include "VL53L1X.h"

// Show Debug Log messages for Yaw/Pitch/Roll/Roll Delta
// #define SHOW_LOG

#define GLOBAL_KICK 0
#define IO_CTRL 0
#define POLARITY_SET 0

#define DEMO_STACK_SIZE         1024
#define DEMO_BYTE_POOL_SIZE     9120
#define DEMO_BLOCK_POOL_SIZE    100
#define DEMO_QUEUE_SIZE         100

#define PWM_PERIOD 1000
#define PID_RANGE 100
#define PWM_MULTIPLIER 10		// range is -250,250 - need to get to PWM duty cycle of 0-1000.

// amount to add/remove from setpoint when ToF is active
#define TOF_ADJUST 1.5

#define SETPOINT_ADJUST 0.01
#define SETPOINT_ADJUST_ANGLE 30
#define SETPOINT_ADJUST_TRIM 0.02

// distance where obstacles are detected
// we will still get a reading outside of this range
// these 'longer' readings are ignored.
#define TOF_OBSTACLE_DISTANCE_MM 100	// 100mm == ~4 inches from ToF sensor.

// used to turn ToF off when robot is initially balanced
// count for TOF_STABILIZE_PERIOD (value can be adjusted) before enabling ToF.
static unsigned long ToF_Settle_Counter = 0;
#define TOF_STABILIZE_PERIOD 500	// 500 * 5ms tick (2,500ms, [2.5 seconds]).

struct TURN_DETAILS details;
struct DEVICE_STATUS tMsg;
struct IMU_STABLE_RESULT tImuStatusMsg;
struct TURN_ROBOT tNorth;
int turnHeading = -1;
unsigned long ToF_ObstacleCounter = 0;

static bool updating = false;		// set to true when update is accepted in the HL app.

// resources for inter core messaging
static uint8_t buf[64];
static uint32_t dataSize;
static BufferHeader* outbound, * inbound;
static uint32_t sharedBufSize = 0;
static const size_t payloadStart = 20;
bool highLevelReady = false;

static bool haveHLApp = false;		// used to determine whether we've received a message from the HL app.

static const uint8_t HighLevelAppComponentId[16] = { 0x67, 0xc3, 0x5b, 0x88, 0x59, 0xb1, 0xa5, 0x44, 0x91, 0xfa, 0x3f, 0xeb, 0x53, 0xa8, 0x23, 0x17 };

// Define the ThreadX object control blocks...
TX_THREAD				tx_hardware_init_thread;
TX_THREAD				tx_timer_test_Thread;
TX_THREAD               tx_hardware_Thread;
TX_THREAD               tx_ToF_Thread;
TX_THREAD               tx_Intercore_Thread;

TX_TIMER				msTimer;
TX_EVENT_FLAGS_GROUP    hardware_event_flags_0;
TX_EVENT_FLAGS_GROUP    ToF_event_flags_0;
TX_EVENT_FLAGS_GROUP    Intercore_event_flags_0;

TX_BYTE_POOL            byte_pool_0;
TX_BLOCK_POOL           block_pool_0;
UCHAR                   memory_area[DEMO_BYTE_POOL_SIZE];

void timerFn(ULONG input);

// Define thread prototypes.
void hardware_thread(ULONG thread_input);
void ToF_thread(ULONG thread_input);
void Intercore_thread(ULONG thread_input);
void hardware_init_thread(ULONG thread_input);

void EnqueueIntercoreMessage(void* payload, size_t payload_size);
int GetCompassDirection(float compassAngle);

static bool TurnRobotFlag = false;
static float turnStartHeading = 0.0;

//#define MOTOR_MULTIPLIER 1000/PID_RANGE
#define CUT_OFF_ANGLE 45

#define M_PI 3.14159265358979323846

// auto-calibration stuff.
#define SETPOINT_DEFAULT 90.54 

static bool AutoCalibrateSetpoint = false;
// 5ms tick, 200 ticks (1 second) for auto-adjust.
static double setpoint = SETPOINT_DEFAULT;
static double CalibratedSetpoint = SETPOINT_DEFAULT;

// setpoint adjust values.
float ToFSetpointAdjust = 0.0;
float SpeedSetpointAdjust = 0.0;
float RemoteFwdBackAdjust = 0.0;	// add/subtract to get the robot to move foreward/back.
int RemoteRotateCommand = 0;	// 0 = none, 1 == left, 2 == right.

#define PID_KP  2.2f
#define PID_KI  0.5f
#define PID_KD  0.25f

#define PID_TAU 0.05f
#define SAMPLE_TIME_S 0.05f

#define PID_LIM_MIN -1.5f      
#define PID_LIM_MAX  1.5f

PIDController pid = { PID_KP, PID_KI, -PID_KD,
					  PID_TAU,
					  PID_LIM_MIN, PID_LIM_MAX,
					  SAMPLE_TIME_S };

// used when turning, or moving in a specific direction
// 'turns off' auto-tuning of balance point.
static bool RotateClockwise = false;	// used to determine the robot rotation direction (shortest rotation).
static bool ObstacleDetected = false;
static bool Tof_Active = false;		// false if the robot is leaning more than CUT_OFF_ANGLE - don't read ToF lasers
static bool GetRotationDirection(float current, float desired);

// used to tweak the balance point.
#define SETPOINT_TWEAK_MAX 0.1
#define SETPOINT_TWEAK_MIN 0.005
bool applyAutoAdjust = true;
float setpointTweak = SETPOINT_TWEAK_MAX;

static float rollHistory[200];
static int rollHistoryPointer = 0;
static unsigned long loopCounter = 0;
static bool imuStable = false;

float oldOutput = 0.0;

// globals used in intercore-msgs.
float g_pitch, g_yaw, g_roll = 0.0;
float g_heading = 0.0;

double input, output = 0.0;

// offset to add to/subtract from motors to allow spin.
#define SPIN_OFFSET 150

double Kp = 16; 
double Ki = 120;
double Kd = .4;

#define ACCELEROMETER_SENSITIVITY 8192.0
#define GYROSCOPE_SENSITIVITY 65.536

// from accel calibration.
float    base_x_accel;
float    base_y_accel;
float    base_z_accel;

float    base_x_gyro;
float    base_y_gyro;
float    base_z_gyro;

// Use the following global variables and access functions to help store the overall
// rotation angle of the sensor
unsigned long last_read_time;
float         last_x_angle;  // These are the filtered angles
float         last_y_angle;
float         last_z_angle;
float         last_gyro_x_angle;  // Store the gyro angles to compare drift
float         last_gyro_y_angle;
float         last_gyro_z_angle;

void calibrate_sensors();
void set_last_read_angle_data(unsigned long time, float x, float y, float z, float x_gyro, float y_gyro, float z_gyro);

static inline unsigned long get_last_time() { return last_read_time; }
static inline float get_last_x_angle() { return last_x_angle; }
static inline float get_last_y_angle() { return last_y_angle; }
static inline float get_last_z_angle() { return last_z_angle; }
static inline float get_last_gyro_x_angle() { return last_gyro_x_angle; }
static inline float get_last_gyro_y_angle() { return last_gyro_y_angle; }
static inline float get_last_gyro_z_angle() { return last_gyro_z_angle; }

static volatile bool hardwareInitOK = false;

bool InitHardware(void);
unsigned long loop(void);

unsigned long loop(void)
{
	unsigned long startPeriod = millis();


	// TODO: Read IMU, calculate Yaw, Pitch, Roll
	// ICM-20948 oritentation gives Roll as lean angle.

	float _Roll = 0.0;	// 
	float g_heading = 0.0; //
	if (g_heading < 0)
		g_heading += 360;

	loopCounter++;		// counter to ensure we have 200 items in the rollHistory array.


	// roll history is used to determine how stable the IMU readings are before unlocking the motors/telemetry
	rollHistory[rollHistoryPointer] = _Roll;
	rollHistoryPointer = (rollHistoryPointer + 1) % 200;

	float min = 360.0;
	float max = 0;
	int rollHistoryCount = 0;
	for (int x = 0; x < 200; x++)
	{
		if (rollHistory[x] != -1)
		{
			rollHistoryCount++;
			if (rollHistory[x] < min)
				min = rollHistory[x];
			if (rollHistory[x] > max)
				max = rollHistory[x];
		}
	}

	// determine whether the IMU is stable.
	if (!imuStable && rollHistoryCount == 200 && fabs(max - min) < 0.02)
	{
		imuStable = true;
	}

	// if the IMU is not stable, or the system is updating then return (motor control code after this point)
	if (!imuStable || updating)
	{
		return 0;
	}

	// see if we're close to pointing north...
	if (TurnRobotFlag)
	{
		// get a value from 0 to 15 (0 == north)
		int OrientPos=GetCompassDirection(g_heading);
		int TurnPos = GetCompassDirection(turnHeading);
		if (OrientPos == TurnPos)
		{
			details.id = MSG_TURN_DETAILS;
			details.startHeading = turnStartHeading;
			details.endHeading = g_heading;
			EnqueueIntercoreMessage(&details, sizeof(details));
			TurnRobotFlag = false;
		}
	}

	// set the PID input
	input = _Roll;

	float delta = fabs(_Roll - CalibratedSetpoint);

	static unsigned long calibrationCounter = 0;

	float setpointMotorAdjust = 0.0;

	calibrationCounter++;
	if (calibrationCounter == 10)
	{
		calibrationCounter = 0;
		if (!AutoCalibrateSetpoint && !ObstacleDetected && _Roll > 80)
		{
			setpointMotorAdjust = PIDController_Update(&pid, 0, output/100.0);
			SpeedSetpointAdjust = setpointMotorAdjust * -1;
		}
	}

	setpoint = CalibratedSetpoint + ToFSetpointAdjust + SpeedSetpointAdjust + RemoteFwdBackAdjust;

	Compute();

	if (!TurnRobotFlag && !ObstacleDetected && _Roll > 80 && RemoteFwdBackAdjust == 0 && RemoteRotateCommand == 0)
	{
		if (output > 0)
		{
			CalibratedSetpoint += 0.005;
		}
		if (output < 0)
		{
			CalibratedSetpoint -= 0.005;
		}
	}


#ifdef SHOW_LOG
		printf("Calibrated Setpoint %3.2f | Setpoint %3.2f (Tweak %3.2f) [AutoCalib %c | calibStarted %c | Tick %d] | Delta %3.2f | Output %3.2f\n", CalibratedSetpoint, setpoint, setpointTweak, AutoCalibrateSetpoint == true ? 'Y' : 'N', autoCalibrateStarted == true ? 'Y' : 'N', AutoCalibrationTickCounter, delta, output);
#endif

	if (output == 0 || delta > CUT_OFF_ANGLE)
	{
		Tof_Active = false;
		// turn off 'GPIO6 LED'
		mtk_os_hal_pwm_config_freq_duty_normal(OS_HAL_PWM_GROUP1, PWM_CHANNEL2, PWM_PERIOD, 0);
		ToF_Settle_Counter = 0;

		// turn off motors.
		mtk_os_hal_gpio_set_output(OS_HAL_GPIO_0, 0);
		mtk_os_hal_gpio_set_output(OS_HAL_GPIO_1, 0);
		mtk_os_hal_gpio_set_output(OS_HAL_GPIO_2, 0);
		mtk_os_hal_gpio_set_output(OS_HAL_GPIO_12, 0);
		mtk_os_hal_gpio_set_output(OS_HAL_GPIO_13, 0);

		mtk_os_hal_pwm_config_freq_duty_normal(OS_HAL_PWM_GROUP1, PWM_CHANNEL0, PWM_PERIOD, 0);
		mtk_os_hal_pwm_config_freq_duty_normal(OS_HAL_PWM_GROUP1, PWM_CHANNEL1, PWM_PERIOD, 0);
	}
	else
	{
		if (Tof_Active == false)
		{
			ToF_Settle_Counter++;
			if (ToF_Settle_Counter == TOF_STABILIZE_PERIOD)
			{
				ToF_Settle_Counter = 0;
				Tof_Active = true;
				// turn on 'GPIO6 LED'
				// mtk_os_hal_pwm_config_freq_duty_normal(OS_HAL_PWM_GROUP1, PWM_CHANNEL2, PWM_PERIOD, 1000);
			}
		}

		unsigned long duty = abs(output)*PWM_MULTIPLIER;
		unsigned long dutyLeft = duty;
		unsigned long dutyRight = duty;

		static bool Tick = false;

		// add turning value.
		if ((TurnRobotFlag || RemoteRotateCommand != 0) && Tick && !ObstacleDetected)
		{
			if (TurnRobotFlag || RemoteRotateCommand)
			{
				if (output > 0)
				{
					if (!RotateClockwise)
					{
						dutyLeft = (duty + SPIN_OFFSET) % 1000;
					}
					else
					{
						dutyRight = (duty + SPIN_OFFSET) % 1000;
					}
				}
				if (output < 0)
				{
					if (!RotateClockwise)
					{
						dutyRight = (duty + SPIN_OFFSET) % 1000;
					}
					else
					{
						dutyLeft = (duty + SPIN_OFFSET) % 1000;
					}
				}
			}
		}

		if (output > 0)
		{
			mtk_os_hal_gpio_set_output(OS_HAL_GPIO_0, 0);
			mtk_os_hal_gpio_set_output(OS_HAL_GPIO_1, 1);
			mtk_os_hal_gpio_set_output(OS_HAL_GPIO_2, 1);
			mtk_os_hal_gpio_set_output(OS_HAL_GPIO_12, 0);
		}
		else
		{
			mtk_os_hal_gpio_set_output(OS_HAL_GPIO_0, 1);
			mtk_os_hal_gpio_set_output(OS_HAL_GPIO_1, 0);
			mtk_os_hal_gpio_set_output(OS_HAL_GPIO_2, 0);
			mtk_os_hal_gpio_set_output(OS_HAL_GPIO_12, 1);
		}

		Tick = !Tick;

		mtk_os_hal_gpio_set_output(OS_HAL_GPIO_13, 1);
		mtk_os_hal_pwm_config_freq_duty_normal(OS_HAL_PWM_GROUP1, PWM_CHANNEL0, PWM_PERIOD, dutyLeft);
		mtk_os_hal_pwm_config_freq_duty_normal(OS_HAL_PWM_GROUP1, PWM_CHANNEL1, PWM_PERIOD, dutyRight);
	}

	unsigned long timePeriod = millis() - startPeriod;

	return timePeriod;
}

bool InitHardware(void)
{
	// Initialize I2C (IMU)
	mtk_os_hal_i2c_ctrl_init(OS_HAL_I2C_ISU0);
	mtk_os_hal_i2c_speed_init(OS_HAL_I2C_ISU0, I2C_SCL_400kHz);

	// Initialize I2C (ToF Sensors)
	mtk_os_hal_i2c_ctrl_init(OS_HAL_I2C_ISU1);
	mtk_os_hal_i2c_speed_init(OS_HAL_I2C_ISU1, I2C_SCL_400kHz);

	// Motor control.
	mtk_os_hal_gpio_set_direction(OS_HAL_GPIO_0, OS_HAL_GPIO_DIR_OUTPUT);
	mtk_os_hal_gpio_set_direction(OS_HAL_GPIO_1, OS_HAL_GPIO_DIR_OUTPUT);
	mtk_os_hal_gpio_set_direction(OS_HAL_GPIO_2, OS_HAL_GPIO_DIR_OUTPUT);
	mtk_os_hal_gpio_set_direction(OS_HAL_GPIO_12, OS_HAL_GPIO_DIR_OUTPUT);

	// ToF Fanout chip
	mtk_os_hal_gpio_set_direction(OS_HAL_GPIO_16, OS_HAL_GPIO_DIR_OUTPUT);

	// Motor Driver Enable.
	mtk_os_hal_gpio_set_direction(OS_HAL_GPIO_13, OS_HAL_GPIO_DIR_OUTPUT);

	// set initial state - all low.
	mtk_os_hal_gpio_set_output(OS_HAL_GPIO_0, 0);
	mtk_os_hal_gpio_set_output(OS_HAL_GPIO_1, 0);
	mtk_os_hal_gpio_set_output(OS_HAL_GPIO_2, 0);
	mtk_os_hal_gpio_set_output(OS_HAL_GPIO_12, 0);
	// Motor Controller (initially off)
	mtk_os_hal_gpio_set_output(OS_HAL_GPIO_13, 0);
	// ToF Fanout - turned on.
	mtk_os_hal_gpio_set_output(OS_HAL_GPIO_16, 1);

	// configure PWM
	mtk_os_hal_pwm_ctlr_init(OS_HAL_PWM_GROUP1, PWM_CHANNEL0 | PWM_CHANNEL1 | PWM_CHANNEL2);

	mtk_os_hal_pwm_feature_enable(OS_HAL_PWM_GROUP1,PWM_CHANNEL0,GLOBAL_KICK,IO_CTRL,POLARITY_SET);
	mtk_os_hal_pwm_feature_enable(OS_HAL_PWM_GROUP1, PWM_CHANNEL1, GLOBAL_KICK, IO_CTRL, POLARITY_SET);
	mtk_os_hal_pwm_feature_enable(OS_HAL_PWM_GROUP1, PWM_CHANNEL2, GLOBAL_KICK, IO_CTRL, POLARITY_SET);

	mtk_os_hal_pwm_config_freq_duty_normal(OS_HAL_PWM_GROUP1,PWM_CHANNEL0, PWM_PERIOD,0);
	mtk_os_hal_pwm_config_freq_duty_normal(OS_HAL_PWM_GROUP1, PWM_CHANNEL1, PWM_PERIOD, 0);
	mtk_os_hal_pwm_config_freq_duty_normal(OS_HAL_PWM_GROUP1, PWM_CHANNEL2, PWM_PERIOD, 0);

	mtk_os_hal_pwm_start_normal(OS_HAL_PWM_GROUP1,PWM_CHANNEL0);
	mtk_os_hal_pwm_start_normal(OS_HAL_PWM_GROUP1, PWM_CHANNEL1);
	mtk_os_hal_pwm_start_normal(OS_HAL_PWM_GROUP1, PWM_CHANNEL2);

	// TODO: Initialize the IMU here

	PID(&input, &output, &setpoint, Kp, Ki, Kd, DIRECT);
	SetMode(AUTOMATIC);
	SetSampleTime(5);
	SetOutputLimits(-PID_RANGE, PID_RANGE); // (-100,100);

	return true;
}

void ToF_thread(ULONG thread_input)
{
	printf("Initialize ToF\r\n");
	SelectFanoutChannel(1);
	
	VL53L1X_setActiveLaser(1);	// used to track which has been configued.

	bool initOk = VL53L1X_init(true);

	if (!initOk)
	{
		printf("ToF Channel 1 failed\r\n");
		return;
	}

	VL53L1X_setDistanceMode(Long);
	VL53L1X_setMeasurementTimingBudget(50000);
	// Start continuous readings at a rate of one measurement every 50 ms (the
	// inter-measurement period). This period should be at least as long as the
	// timing budget.
	VL53L1X_startContinuous(50);

	delay(2);

	SelectFanoutChannel(2);

	VL53L1X_setActiveLaser(1);	// used to track which has been configued.

	initOk = VL53L1X_init(true);

	if (!initOk)
	{
		printf("ToF Channel 2 failed\r\n");
		return;
	}

	VL53L1X_setDistanceMode(Long);
	VL53L1X_setMeasurementTimingBudget(50000);
	// Start continuous readings at a rate of one measurement every 50 ms (the
	// inter-measurement period). This period should be at least as long as the
	// timing budget.
	VL53L1X_startContinuous(50);

	SelectFanoutChannel(1);

	bool useFrontToF = true;		// TODO: tick/tock on front/rear sensors.

	ULONG   actual_flags = 0;

	uint16_t distances[2];	// front and rear lasers.
	// setup last distances to be 'far'.
	uint16_t lastDistances[2] = { 2400,2400 };

	bool obstacles[2] = { false, false };

	while (true)
	{
		// wait on timer tick.
		tx_event_flags_get(&ToF_event_flags_0, 0x1, TX_OR_CLEAR, &actual_flags, TX_WAIT_FOREVER);

		if (Tof_Active)
		{
			int index = useFrontToF == true ? 0 : 1;
			distances[index] = VL53L1X_read(true);

			if (distances[index] != 0)
			{
				// Log_Debug("ToF Distance %d\r\n", distance);

				if (distances[index] < TOF_OBSTACLE_DISTANCE_MM && distances[index] > 0 && !obstacles[index])
				{
					mtk_os_hal_pwm_config_freq_duty_normal(OS_HAL_PWM_GROUP1, PWM_CHANNEL2, PWM_PERIOD, 1000);
					// Log_Debug("Obstacle Found - adjusting setpoint\n");
					obstacles[index] = true;
					ObstacleDetected = true;
					ToF_ObstacleCounter++;

					if (useFrontToF)
					{
						ToFSetpointAdjust = -1.5;
					}
					else
					{
						ToFSetpointAdjust = 1.5;
					}
				}

				// see if we're moving away, and if yes, cancel the movement.
				if (obstacles[index] && lastDistances[index] < distances[index] && distances[index] > 0) //distances[index] > 100 && obstacles[index])
				{
					mtk_os_hal_pwm_config_freq_duty_normal(OS_HAL_PWM_GROUP1, PWM_CHANNEL2, PWM_PERIOD, 0);
					obstacles[index] = false;
					ObstacleDetected = false;
					ToFSetpointAdjust = 0.0;
				}
			}
			lastDistances[index] = distances[index];
			useFrontToF = !useFrontToF;
			SelectFanoutChannel(useFrontToF == true ? 1 : 2);
		}
	}

	printf("ToF Thread exit\r\n");
}

void Intercore_thread(ULONG thread_input)
{
	ULONG   actual_flags = 0;

	printf("Intercore Thread Starting\r\n");

	struct TURN_ROBOT* pTurn;
	struct SETPOINT* pSetpoint;
	struct REMOTE_CMD* pRemote;
	struct UPDATE_ACTIVE* pUpdate;

	while (true)
	{
		// wait on timer tick.
		tx_event_flags_get(&Intercore_event_flags_0, 0x1, TX_OR_CLEAR, &actual_flags, TX_WAIT_FOREVER);
		dataSize = sizeof(buf);
		int r = DequeueData(outbound, inbound, sharedBufSize, buf, &dataSize);
		if (r == 0 && dataSize > payloadStart) {
			size_t payloadBytes = dataSize - payloadStart;
			if (payloadBytes > 0) {
				switch (buf[payloadStart]) {
				case MSG_UPDATE_ACTIVE:
					pUpdate = (struct UPDATE_ACTIVE*)&buf[payloadStart];
					if (pUpdate->updateActive)
					{
						updating = true;
					}
					else
					{
						updating = false;
					}
					break;
				case MSG_REMOTE_CMD:
					pRemote=(struct REMOTE_CMD*)&buf[payloadStart];
					switch (pRemote->cmd)
					{
					case 4:	// stop.
						RemoteFwdBackAdjust = 0.0;
						RemoteRotateCommand = 0;
						break;
					case 1:	// foreward
						RemoteFwdBackAdjust = 1.5;
						RemoteRotateCommand = 0;
						break;
					case 3:	// back
						RemoteFwdBackAdjust = -1.5;
						RemoteRotateCommand = 0;
						break;
					case 2:		// right
						RemoteFwdBackAdjust = 0.0;
						RotateClockwise = false;
						RemoteRotateCommand = 2;	// right
							break;
					case 0:		// left
						RemoteFwdBackAdjust = 0.0;
						RotateClockwise = true;
						RemoteRotateCommand = 1;	// left.
						break;
					default:
						break;
					};
					break;
				case MSG_IMU_STABLE_REQUEST:
					tImuStatusMsg.id = MSG_IMU_STABLE_RESULT;
					tImuStatusMsg.imuStable = imuStable;
					EnqueueIntercoreMessage(&tImuStatusMsg, sizeof(tImuStatusMsg));
					break;
				case MSG_TELEMETRY_REQUEST:
					haveHLApp = true;		// have received at least one message from the HL App, can now start sending IMU telemetry.
					tMsg.id = MSG_DEVICE_STATUS;
					tMsg.timestamp = millis();
					tMsg.numObstaclesDetected = ToF_ObstacleCounter;
					tMsg.setpoint = CalibratedSetpoint;
					tMsg.pitch = g_pitch;
					tMsg.yaw = g_heading;
					tMsg.roll = g_roll;
					tMsg.turnNorth = TurnRobotFlag;
					tMsg.avoidActive = ObstacleDetected;
					EnqueueIntercoreMessage(&tMsg, sizeof(tMsg));
					break;
				case MSG_SETPOINT:
					pSetpoint = (struct SETPOINT*)&buf[payloadStart];
					if (pSetpoint->setpoint > 80 && pSetpoint->setpoint < 100)
					{
						CalibratedSetpoint = pSetpoint->setpoint;
					}
					break;
				case MSG_TURN_ROBOT:
					pTurn = (struct TURN_ROBOT*)&buf[payloadStart];
					
					// accept the turn if we're stood up.
					if (g_roll > 80)
					{
						if (pTurn->enabled)
						{
							RotateClockwise = GetRotationDirection(g_heading, pTurn->heading);
						}
						turnHeading = pTurn->heading;
						TurnRobotFlag = pTurn->enabled;
						if (TurnRobotFlag)
						{
							// store current heading (used in telemetry).
							turnStartHeading = g_heading;
						}
					}
					break;
				};
			}
		}
	}

	printf("Intercore Thread exit\r\n");
}

static bool GetRotationDirection(float current, float desired)
{
	bool increment = false;

	if (current < desired)
	{
		if (fabs(current - desired) < 180)
			increment = true;
		else
			increment = false;
	}
	else
	{
		if (fabs(current - desired) < 180)
			increment = false;
		else
			increment = true;
	}
	return increment;
}

void EnqueueIntercoreMessage(void* payload, size_t payload_size)
{
	uint8_t sendbuf[64];

	if ((payloadStart + payload_size) > sizeof(sendbuf)) {
		printf("EnqueueIntercoreMessage insufficient buffer\n");
		return;
	}

	memset(sendbuf, 0x00, 64);
	memcpy(sendbuf, HighLevelAppComponentId, sizeof(HighLevelAppComponentId));
	memcpy(&sendbuf[payloadStart], payload, payload_size);
	int result = EnqueueData(inbound, outbound, sharedBufSize, sendbuf, payloadStart + payload_size);
	if (result != 0)
	{
		printf("Unable to queue intercore data\r\n");
	}
}

int GetCompassDirection(float compassAngle)
{
	int pos = (int)((compassAngle / 22.5) + .5);
	pos = pos % 16;
	return pos;
}

void hardware_thread(ULONG thread_input)
{
	ULONG   actual_flags = 0;
#ifdef SHOW_DEBUG_MSGS
	unsigned long last = 0;
	unsigned long now = 0;
	unsigned long delta = 0;
	unsigned long loopTime = 0;
#endif

	printf("hardware thread starting...\r\n");

	while (true)
	{
		tx_event_flags_get(&hardware_event_flags_0, 0x1, TX_OR_CLEAR, &actual_flags, TX_WAIT_FOREVER);
#ifdef SHOW_DEBUG_MSGS
		loopTime = loop();
		now = millis();
		delta = now - last;
		last = now;

		printf("%u | %u | %s - Loop: %u\r\n",now, delta, delta > 5 ? "***"  : " ",loopTime);
#else
		loop();
#endif

	}
}

void timerFn(ULONG input)
{
	static unsigned long IntercoreTickCounter = 0;
	static unsigned long ToFTickCounter = 0;
	ULONG status = TX_SUCCESS;

	if (hardwareInitOK == true)
	{
		status = tx_event_flags_set(&hardware_event_flags_0, 0x1, TX_OR);
		if (status != TX_SUCCESS)
		{
			printf("failed to set hardware event flags\r\n");
		}

		ToFTickCounter++;
		if (ToFTickCounter == 20)	// 100ms
		{
			ToFTickCounter=0;
			status = tx_event_flags_set(&ToF_event_flags_0, 0x1, TX_OR);
			if (status != TX_SUCCESS)
			{
				printf("failed to set ToF event flags\r\n");
			}
		}

		// Azure HL App 'Device Twin' check is every 5 seconds
		// Request for Telemetry is every 20 seconds.
		IntercoreTickCounter++;
		if (IntercoreTickCounter == 100)	// 0.5 seconds.
		{
			IntercoreTickCounter = 0;
			status = tx_event_flags_set(&Intercore_event_flags_0, 0x1, TX_OR);
			if (status != TX_SUCCESS)
			{
				printf("failed to set Intercore event flags\r\n");
			}
		}
	}
}

// only purpose in life is to initialize the hardware.
void hardware_init_thread(ULONG thread_input)
{
	printf("Hardware Init Thread - start: %u\r\n", millis());
	UINT status = TX_SUCCESS;
	bool hwInitOk = InitHardware();

	if (hwInitOk)
	{
		hardwareInitOK = true;
		status = tx_timer_create(&msTimer, "5ms Timer", timerFn, 0, 5, 5, TX_AUTO_ACTIVATE);
		if (status != TX_SUCCESS)
		{
			printf("failed to create timer\r\n");
		}
		else
		{
			printf("timer created ok\r\n");
		}
	}

	printf("Hardware Init - %s\r\n", hardwareInitOK ? "OK" : "FAIL");
}

int main() {
	tx_kernel_enter();	// Enter the Azure RTOS kernel.
}

// Define what the initial system looks like.
void tx_application_define(void* first_unused_memory) {
	CHAR* pointer;
	UINT status = TX_SUCCESS;

	//volatile bool f = false;
	//while (!f) {
	//	// empty.
	//}

	printf("%c[2J%c[0;0HAzure Sphere Robot App\r\n", 0x1b, 0x1b);

	if (GetIntercoreBuffers(&outbound, &inbound, &sharedBufSize) == -1) {					// Initialize Inter-Core Communications
		return;	// don't let the intercore thread run.
	}

	/* Create a byte memory pool from which to allocate the thread stacks.  */
	tx_byte_pool_create(&byte_pool_0, "byte pool 0", memory_area, DEMO_BYTE_POOL_SIZE);

	// create event flags
	status = tx_event_flags_create(&hardware_event_flags_0, "Hardware Event");		// Hardware events fire every 5 ms
	if (status != TX_SUCCESS)
	{
		printf("failed to create hardware_event_flags\r\n");
	}

	status = tx_event_flags_create(&ToF_event_flags_0, "ToF Event");					// ToF events fire every 100 ms
	if (status != TX_SUCCESS)
	{
		printf("failed to create ToF_event_flags\r\n");
	}

	status = tx_event_flags_create(&Intercore_event_flags_0, "Intercore Event");		// Intercore events fire every 10 ms
	if (status != TX_SUCCESS)
	{
		printf("failed to create Intercore_event_flags\r\n");
	}

	/* Allocate the stack for thread 0.  */
	tx_byte_allocate(&byte_pool_0, (VOID**)&pointer, DEMO_STACK_SIZE, TX_NO_WAIT);
	/* Create the main thread.  */
	tx_thread_create(&tx_hardware_Thread, "hardware thread", hardware_thread, 0,
		pointer, DEMO_STACK_SIZE, 1, 1, TX_NO_TIME_SLICE, TX_AUTO_START);

	tx_byte_allocate(&byte_pool_0, (VOID**)&pointer, DEMO_STACK_SIZE, TX_NO_WAIT);
	/* Create the Time of Flight thread.  */
	tx_thread_create(&tx_ToF_Thread, "ToF Thread", ToF_thread, 0,
		pointer, DEMO_STACK_SIZE, 8, 8, TX_NO_TIME_SLICE, TX_AUTO_START);

	tx_byte_allocate(&byte_pool_0, (VOID**)&pointer, DEMO_STACK_SIZE, TX_NO_WAIT);
	/* Create the intercore msg thread.  */
	tx_thread_create(&tx_Intercore_Thread, "Intercore Thread", Intercore_thread, 0,
		pointer, DEMO_STACK_SIZE, 4, 4, TX_NO_TIME_SLICE, TX_AUTO_START);

	tx_byte_allocate(&byte_pool_0, (VOID**)&pointer, DEMO_STACK_SIZE, TX_NO_WAIT);
	// Create a hardware init thread.
	tx_thread_create(&tx_hardware_init_thread, "hardware init thread", hardware_init_thread, 0,
		pointer, DEMO_STACK_SIZE, 1, 1, TX_NO_TIME_SLICE, TX_AUTO_START);
}

