/* Copyright (c) Codethink Ltd. All rights reserved.
   Licensed under the MIT License. */

#ifndef AZURE_SPHERE_NVIC_H_
#define AZURE_SPHERE_NVIC_H_

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
 extern "C" {
#endif

/// <summary>NVIC Registers, ARM DDI 0403E.b S3.4.3.</summary>
static const volatile uint32_t * const ICTR      = (const volatile uint32_t *)0xE000E004;
static       volatile uint32_t * const NVIC_ISER = (      volatile uint32_t *)0xE000E100;
static       volatile uint32_t * const NVIC_ICER = (      volatile uint32_t *)0xE000E180;
static       volatile uint32_t * const NVIC_ISPR = (      volatile uint32_t *)0xE000E200;
static       volatile uint32_t * const NVIC_ICPR = (      volatile uint32_t *)0xE000E280;
static const volatile uint32_t * const NVIC_IABR = (const volatile uint32_t *)0xE000E300;
static       volatile uint8_t  * const NVIC_IPR  = (      volatile uint8_t  *)0xE000E400;

/// <summary>The IOM4 cores on the MT3620 use three bits to encode interrupt priorities.</summary>
#define NVIC_PRIORITY_BITS 3

/// <summary>
/// <para>Blocks interrupts at priority 1 level and above.</para>
/// <para>Pair this with a call to <see cref="NVIC_RestoreIRQs" /> to unblock interrupts.</para>
/// </summary>
/// <returns>Previous value of BASEPRI register. This can be treated as an opaque value
/// which must be passed to <see cref="NVIC_RestoreIRQs" />.</returns>
static inline uint32_t NVIC_BlockIRQs(void)
{
    uint32_t prevBasePri;
    uint32_t newBasePri = 1; // block IRQs priority 1 and above

    __asm__("mrs %0, BASEPRI" : "=r"(prevBasePri) :);
    __asm__("msr BASEPRI, %0" : : "r"(newBasePri));
    return prevBasePri;
}

/// <summary>
/// Re-enables interrupts which were blocked by <see cref="NVIC_BlockIRQs" />.
/// </summary>
/// <param name="prevBasePri">Value returned from <see cref="NVIC_BlockIRQs" />.</param>
static inline void NVIC_RestoreIRQs(uint32_t prevBasePri)
{
    __asm__("msr BASEPRI, %0" : : "r"(prevBasePri));
}

/// <summary>
/// <para>Enable NVIC interrupt.</para>
/// <para>See DDI 0403E.d SB3.4.4, Interrupt Set-Enable Registers, NVIC_ISER0-NVIC_ISER7.</para>
/// </summary>
/// <param name="irq">Which interrupt to enable.</param>
/// <param name="priority">Priority, which must fit into the number of supported priority bits.</param>
static inline void NVIC_EnableIRQ(unsigned irq, unsigned priority)
{
    priority  &= ((1U << NVIC_PRIORITY_BITS) - 1);
    priority <<= (8 - NVIC_PRIORITY_BITS);
    NVIC_IPR[irq] = priority;

    unsigned offset = irq / 32;
    uint32_t mask   = 1U << (irq % 32);
    NVIC_ISER[offset] = mask;
}

/// <summary>
/// <para>Disable NVIC interrupt.</para>
/// <para>See DDI 0403E.d SB3.4.4, Interrupt Set-Enable Registers, NVIC_ICER0-NVIC_ICER7.</para>
/// </summary>
/// <param name="irq">Which interrupt to disable.</param>
static inline void NVIC_DisableIRQ(unsigned irq)
{
    unsigned offset = irq / 32;
    uint32_t mask   = 1U << (irq % 32);
    NVIC_ICER[offset] = mask;
}

#ifdef __cplusplus
 }
#endif

#endif /* AZURE_SPHERE_NVIC_H_ */
