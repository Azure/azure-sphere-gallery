/* Copyright (c) Codethink Ltd. All rights reserved.
   Licensed under the MIT License. */

#ifndef AZURE_SPHERE_GPIO_H_
#define AZURE_SPHERE_GPIO_H_

#include <stdbool.h>
#include <stdint.h>

#include "Common.h"

#ifdef __cplusplus
 extern "C" {
#endif

#define ERROR_GPIO_NOT_A_PIN             (ERROR_SPECIFIC - 1)
#define ERROR_PWM_UNSUPPORTED_DUTY_CYCLE (ERROR_SPECIFIC - 2)
#define ERROR_PWM_UNSUPPORTED_CLOCK_SEL  (ERROR_SPECIFIC - 3)
#define ERROR_PWM_NOT_A_PIN              (ERROR_SPECIFIC - 4)
#define ERROR_EINT_NOT_A_PIN             (ERROR_SPECIFIC - 5)
#define ERROR_EINT_ATTRIBUTE             (ERROR_SPECIFIC - 6)

/// <summary>
/// <para>Configure a pin for output. Call <see cref="GPIO_Write" /> to set the
/// state.</para>
/// </summary>
/// <param name="pin">A specific pin.</param>
/// <returns>Zero on success or an error value as defined in this file or Common.h.</returns>
int32_t GPIO_ConfigurePinForOutput(uint32_t pin);

/// <summary>
/// <para>Configure a pin for input. Call <see cref="GPIO_Read" /> to read the
/// state.</para>
/// <para>This function does not control the pull-up or pull-down resistors.
/// If the pin is connected to a possibly-floating input, the application may
/// want to additionally enable these via the register interface.</para>
/// </summary>
/// <param name="pin">A specific pin.</param>
/// <returns>Zero on success or an error value as defined in this file or Common.h.</returns>
int32_t GPIO_ConfigurePinForInput(uint32_t pin);

/// <summary>
/// <para>Set the state of a pin which has been configured for output.</para>
/// <para><see cref="GPIO_ConfigurePinForOutput" /> must be called before this
/// function.</para>
/// </summary>
/// <param name="pin">A specific pin.</param>
/// <param name="state">true to drive the pin high; false to drive it low.</param>
/// <returns>Zero on success or an error value as defined in this file or Common.h.</returns>
int32_t GPIO_Write(uint32_t pin, bool state);

/// <summary>
/// <para>Read the state of a pin which has been configured for input.</para>
/// <para><see cref="GPIO_ConfigurePinForInput" /> must be called before this
/// function.</para>
/// </summary>
/// <param name="pin">A specific pin.</param>
/// <param name="state">On return, contains true means the input is high, and false means
/// low.</param>
/// <returns>Zero on success or an error value as defined in this file or Common.h.</returns>
int32_t GPIO_Read(uint32_t pin, bool *state);

/// <summary>
/// <para>Setup a GPIO pin for PWM output.</para>
/// </summary>
/// <param name="pin">A specific pin.</param>
/// <param name="clockFrequency">The desired clock frequency for the PWM counter.</param>
/// <param name="onTime">The number of clock cycles the PWM signal is high for.</param>
/// <param name="offTime">The number of clock cycles the PWM signal is low for.</param>
/// <returns>Zero on success or an error value as defined in this file or Common.h.</returns>
int32_t PWM_ConfigurePin(uint32_t pin, uint32_t clockFrequency, uint32_t onTime, uint32_t offTime);


/* External Interrupts */

#define GPIO_EINT_PIN_COUNT 24

typedef enum {
    GPIO_EINT_DBNC_FREQ_8KHZ   = 0,
    GPIO_EINT_DBNC_FREQ_4KHZ   = 1,
    GPIO_EINT_DBNC_FREQ_2KHZ   = 2,
    GPIO_EINT_DBNC_FREQ_1KHZ   = 3,
    GPIO_EINT_DBNC_FREQ_500HZ  = 4,
    GPIO_EINT_DBNC_FREQ_250HZ  = 5,
    GPIO_EINT_DBNC_FREQ_125HZ  = 6,
    GPIO_EINT_DBNC_FREQ_62_5HZ = 7,
    GPIO_EINT_DBNC_FREQ_INVALID
} gpio_eint_dbnc_freq_e;

/// <summary>
/// <para>EINT configurations.</para>
/// </summary>
/// <param name="positive">Whether the interrupt triggers on logic high or not.</param>
/// <param name="dualEdge">Whether to detect both transistions.</param>
/// <param name="freq">Frequency of debounce counter.</param>
typedef struct {
    bool                  positive;
    bool                  dualEdge;
    gpio_eint_dbnc_freq_e freq;
} gpio_eint_attr_t;

static const gpio_eint_attr_t gpioEINTAttrDefault = {
    .positive = true,
    .dualEdge = false,
    .freq     = GPIO_EINT_DBNC_FREQ_8KHZ
};

/// <summary>
/// <para>Setup a GPIO pin to trigger an external interrupt.</para>
/// </summary>
/// <param name="pin">A specific pin, only pins < GPIO_EINT_PIN_COUNT are supported.</param>
/// <param name="attr">Attributes to set - see struct docs above.</param>
/// <returns>Zero on success or an error value as defined in this file or Common.h.</returns>
int32_t EINT_ConfigurePin(
    uint32_t           pin,
    gpio_eint_attr_t  *attr);

/// <summary>
/// <para>Remove configuration for GPIO pin as EINT source.</para>
/// </summary>
/// <param name="pin">A specific pin, only pins < GPIO_EINT_PIN_COUNT are supported.</param>
/// <returns>Zero on success or an error value as defined in this file or Common.h.</returns>
int32_t EINT_DeConfigurePin(uint32_t pin);

/// <summary>
/// <para>Get debounce counter value.</para>
/// </summary>
/// <param name="pin">A specific pin, only pins < GPIO_EINT_PIN_COUNT are supported.</param>
/// <returns>Debounce counter value.</returns>
uint8_t EINT_GetDebounceCounter(uint32_t pin);

#ifdef __cplusplus
 }
#endif

#endif // #ifndef AZURE_SPHERE_GPIO_H_
