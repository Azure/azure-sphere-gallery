#include "VectorTable.h"
#include "NVIC.h"
#include "mt3620/dma.h"
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>


extern uintptr_t StackTop; // &StackTop == end of TCM0

_Noreturn void RTCoreMain(void);

// Exception Handlers
static _Noreturn void DefaultExceptionHandler(void)      { while (true); }

static _Noreturn void NMIExceptionHandler(void)          { while (true); }

static _Noreturn void HardFaultExceptionHandler(void)    { while (true); }

static _Noreturn void MPUFaultExceptionHandler(void)     { while (true); }

static _Noreturn void BusFaultExceptionHandler(void)     { while (true); }

static _Noreturn void UsageFaultExceptionHandler(void)   { while (true); }

static _Noreturn void SVCallExceptionHandler(void)       { while (true); }

static _Noreturn void DebugMonitorExceptionHandler(void) { while (true); }

static _Noreturn void PendSVExceptionHandler(void)       { while (true); }

static _Noreturn void SysTickExceptionHandler(void)      { while (true); }

void __attribute__((weak, alias("NMIExceptionHandler")))        NMI(void);
void __attribute__((weak, alias("HardFaultExceptionHandler")))  HardFault(void);
void __attribute__((weak, alias("MPUFaultExceptionHandler")))   MPUFault(void);
void __attribute__((weak, alias("BusFaultExceptionHandler")))   BusFault(void);
void __attribute__((weak, alias("UsageFaultExceptionHandler"))) UsageFault(void);
void __attribute__((weak, alias("SVCallExceptionHandler")))     SVCall(void);
void __attribute__((weak, alias("DebugMonitorExceptionHandler"))) DebugMonitor(void);
void __attribute__((weak, alias("PendSVExceptionHandler")))     PendSV(void);
void __attribute__((weak, alias("SysTickExceptionHandler")))    SysTick(void);

void __attribute__((weak, alias("DefaultExceptionHandler"))) wic_int_wake_up(void);
void __attribute__((weak, alias("DefaultExceptionHandler"))) gpt_int_b(void);
void __attribute__((weak, alias("DefaultExceptionHandler"))) gpt3_int_b(void);
void __attribute__((weak, alias("DefaultExceptionHandler"))) wdt_m4_io_irq_b(void);
void __attribute__((weak, alias("DefaultExceptionHandler"))) uart_irq_b(void);
void __attribute__((weak, alias("DefaultExceptionHandler"))) infra_bus_int(void);
void __attribute__((weak, alias("DefaultExceptionHandler"))) cm4_mbox_m4a2a7n_rd_int(void);
void __attribute__((weak, alias("DefaultExceptionHandler"))) cm4_mbox_m4a2a7n_nf_int(void);
void __attribute__((weak, alias("DefaultExceptionHandler"))) cm4_mbox_a7n2m4a_wr_int(void);
void __attribute__((weak, alias("DefaultExceptionHandler"))) cm4_mbox_a7n2m4a_ne_int(void);
void __attribute__((weak, alias("DefaultExceptionHandler"))) cm4_mbox_a7n_fifo_int(void);
void __attribute__((weak, alias("DefaultExceptionHandler"))) cm4_mbox_a7n2m4a_sw_int(void);
void __attribute__((weak, alias("DefaultExceptionHandler"))) cm4_mbox_m4a2m4b_rd_int(void);
void __attribute__((weak, alias("DefaultExceptionHandler"))) cm4_mbox_m4a2m4b_nf_int(void);
void __attribute__((weak, alias("DefaultExceptionHandler"))) cm4_mbox_m4b2m4a_wr_int(void);
void __attribute__((weak, alias("DefaultExceptionHandler"))) cm4_mbox_m4b2m4a_ne_int(void);
void __attribute__((weak, alias("DefaultExceptionHandler"))) cm4_mbox_m4b_fifo_int(void);
void __attribute__((weak, alias("DefaultExceptionHandler"))) cm4_mbox_m4b2m4a_sw_int(void);
void __attribute__((weak, alias("DefaultExceptionHandler"))) mbox_a7n_wake_m4a_int(void);
void __attribute__((weak, alias("DefaultExceptionHandler"))) mbox_m4b_wake_m4a_int(void);
void __attribute__((weak, alias("DefaultExceptionHandler"))) gpio_g0_irq0(void); // EINT0
void __attribute__((weak, alias("DefaultExceptionHandler"))) gpio_g0_irq1(void); // EINT1
void __attribute__((weak, alias("DefaultExceptionHandler"))) gpio_g0_irq2(void); // EINT2
void __attribute__((weak, alias("DefaultExceptionHandler"))) gpio_g0_irq3(void); // EINT3
void __attribute__((weak, alias("DefaultExceptionHandler"))) gpio_g1_irq0(void); // EINT4
void __attribute__((weak, alias("DefaultExceptionHandler"))) gpio_g1_irq1(void); // EINT5
void __attribute__((weak, alias("DefaultExceptionHandler"))) gpio_g1_irq2(void); // EINT6
void __attribute__((weak, alias("DefaultExceptionHandler"))) gpio_g1_irq3(void); // EINT7
void __attribute__((weak, alias("DefaultExceptionHandler"))) gpio_g2_irq0(void); // EINT8
void __attribute__((weak, alias("DefaultExceptionHandler"))) gpio_g2_irq1(void); // EINT9
void __attribute__((weak, alias("DefaultExceptionHandler"))) gpio_g2_irq2(void); // EINT10
void __attribute__((weak, alias("DefaultExceptionHandler"))) gpio_g2_irq3(void); // EINT11
void __attribute__((weak, alias("DefaultExceptionHandler"))) gpio_g3_irq0(void); // EINT12
void __attribute__((weak, alias("DefaultExceptionHandler"))) gpio_g3_irq1(void); // EINT13
void __attribute__((weak, alias("DefaultExceptionHandler"))) gpio_g3_irq2(void); // EINT14
void __attribute__((weak, alias("DefaultExceptionHandler"))) gpio_g3_irq3(void); // EINT15
void __attribute__((weak, alias("DefaultExceptionHandler"))) gpio_g4_irq0(void); // EINT16
void __attribute__((weak, alias("DefaultExceptionHandler"))) gpio_g4_irq1(void); // EINT17
void __attribute__((weak, alias("DefaultExceptionHandler"))) gpio_g4_irq2(void); // EINT18
void __attribute__((weak, alias("DefaultExceptionHandler"))) gpio_g4_irq3(void); // EINT19
void __attribute__((weak, alias("DefaultExceptionHandler"))) gpio_g5_irq0(void); // EINT20
void __attribute__((weak, alias("DefaultExceptionHandler"))) gpio_g5_irq1(void); // EINT21
void __attribute__((weak, alias("DefaultExceptionHandler"))) gpio_g5_irq2(void); // EINT22
void __attribute__((weak, alias("DefaultExceptionHandler"))) gpio_g5_irq3(void); // EINT23
void __attribute__((weak, alias("DefaultExceptionHandler"))) isu_g0_i2c_irq(void);
void __attribute__((weak, alias("DefaultExceptionHandler"))) isu_g0_spim_irq(void);
void __attribute__((weak, alias("DefaultExceptionHandler"))) isu_g0_uart_irq_b(void);
void __attribute__((weak, alias("DefaultExceptionHandler"))) isu_g1_i2c_irq(void);
void __attribute__((weak, alias("DefaultExceptionHandler"))) isu_g1_spim_irq(void);
void __attribute__((weak, alias("DefaultExceptionHandler"))) isu_g1_uart_irq_b(void);
void __attribute__((weak, alias("DefaultExceptionHandler"))) isu_g2_i2c_irq(void);
void __attribute__((weak, alias("DefaultExceptionHandler"))) isu_g2_spim_irq(void);
void __attribute__((weak, alias("DefaultExceptionHandler"))) isu_g2_uart_irq_b(void);
void __attribute__((weak, alias("DefaultExceptionHandler"))) isu_g3_i2c_irq(void);
void __attribute__((weak, alias("DefaultExceptionHandler"))) isu_g3_spim_irq(void);
void __attribute__((weak, alias("DefaultExceptionHandler"))) isu_g3_uart_irq_b(void);
void __attribute__((weak, alias("DefaultExceptionHandler"))) isu_g4_i2c_irq(void);
void __attribute__((weak, alias("DefaultExceptionHandler"))) isu_g4_spim_irq(void);
void __attribute__((weak, alias("DefaultExceptionHandler"))) isu_g4_uart_irq_b(void);
void __attribute__((weak, alias("DefaultExceptionHandler"))) isu_g5_i2c_irq(void);
void __attribute__((weak, alias("DefaultExceptionHandler"))) isu_g5_spim_irq(void);
void __attribute__((weak, alias("DefaultExceptionHandler"))) isu_g5_uart_irq_b(void);
void __attribute__((weak, alias("DefaultExceptionHandler"))) i2s0_irq_b(void);
void __attribute__((weak, alias("DefaultExceptionHandler"))) i2s1_irq_b(void);
void __attribute__((weak, alias("DefaultExceptionHandler"))) adc_irq_b(void);
void __attribute__((weak, alias("DefaultExceptionHandler"))) gpio_g3_cnt_irq(void);
void __attribute__((weak, alias("DefaultExceptionHandler"))) gpio_g4_cnt_irq(void);
void __attribute__((weak, alias("DefaultExceptionHandler"))) gpio_g5_cnt_irq(void);
void __attribute__((weak, alias("DefaultExceptionHandler"))) iom4_CDBGPWRUPREQ(void);
void __attribute__((weak, alias("DefaultExceptionHandler"))) iom4_CDBGPWRUPACK(void);

void __attribute__((weak, alias("DefaultExceptionHandler"))) m4dma_irq_b_adc(void);
void __attribute__((weak, alias("DefaultExceptionHandler"))) m4dma_irq_b_i2s0_tx(void);
void __attribute__((weak, alias("DefaultExceptionHandler"))) m4dma_irq_b_i2s0_rx(void);
void __attribute__((weak, alias("DefaultExceptionHandler"))) m4dma_irq_b_i2s1_tx(void);
void __attribute__((weak, alias("DefaultExceptionHandler"))) m4dma_irq_b_i2s1_rx(void);

// DMA can interrupt many drivers, so we handle that here.
static void m4dma_irq_b(void)
{
    static void (*m4dma_irq_b_isr[MT3620_DMA_COUNT])(void) = {
        [25] = m4dma_irq_b_i2s0_tx,
        [26] = m4dma_irq_b_i2s0_rx,
        [27] = m4dma_irq_b_i2s1_tx,
        [28] = m4dma_irq_b_i2s1_rx,
        [29] = m4dma_irq_b_adc,
    };

    unsigned i;
    for (i = 0; i < MT3620_DMA_COUNT; i++) {
        uint32_t glbsta = (i < 16
            ? mt3620_dma_global->glbsta0
            : mt3620_dma_global->glbsta1);
        if (((glbsta >> (((i % 16) * 2) + 1)) & 1) != 0) {
            void (*isr)(void) = m4dma_irq_b_isr[i];
            if (isr) isr();
        }
    }
}

#define EXC(x) ((x) +  0)
#define INT(x) ((x) + 16)

// ARM DDI0403E.d SB1.5.2-3
// From SB1.5.3, "The Vector table must be naturally aligned to a power of two whose alignment
// value is greater than or equal to (Number of Exceptions supported x 4), with a minimum alignment
// of 128 bytes.". The array is aligned in linker.ld, using the dedicated section ".vector_table".

// The exception vector table contains a stack pointer, 15 exception handlers, and an entry for
// each interrupt.
const void (*ExceptionVectorTable[])(void)
    __attribute__((section(".vector_table"))) __attribute__((used)) = {
        [EXC( 0)] = (void*)&StackTop, // Main Stack Pointer (MSP)
        [EXC( 1)] = RTCoreMain,  // Reset
        [EXC( 2)] = NMI,
        [EXC( 3)] = HardFault,
        [EXC( 4)] = MPUFault,
        [EXC( 5)] = BusFault,
        [EXC( 6)] = UsageFault,
        [EXC( 7)] = NULL,
        [EXC( 8)] = NULL,
        [EXC( 9)] = NULL,
        [EXC(10)] = NULL,
        [EXC(11)] = SVCall,
        [EXC(12)] = DebugMonitor,
        [EXC(13)] = NULL,
        [EXC(14)] = PendSV,
        [EXC(15)] = SysTick,

        [INT( 0)] = wic_int_wake_up,
        [INT( 1)] = gpt_int_b,
        [INT( 2)] = gpt3_int_b,
        [INT( 3)] = wdt_m4_io_irq_b,
        [INT( 4)] = uart_irq_b,
        [INT( 5)] = infra_bus_int,
        [INT( 6)] = cm4_mbox_m4a2a7n_rd_int,
        [INT( 7)] = cm4_mbox_m4a2a7n_nf_int,
        [INT( 8)] = cm4_mbox_a7n2m4a_wr_int,
        [INT( 9)] = cm4_mbox_a7n2m4a_ne_int,
        [INT(10)] = cm4_mbox_a7n_fifo_int,
        [INT(11)] = cm4_mbox_a7n2m4a_sw_int,
        [INT(12)] = cm4_mbox_m4a2m4b_rd_int,
        [INT(13)] = cm4_mbox_m4a2m4b_nf_int,
        [INT(14)] = cm4_mbox_m4b2m4a_wr_int,
        [INT(15)] = cm4_mbox_m4b2m4a_ne_int,
        [INT(16)] = cm4_mbox_m4b_fifo_int,
        [INT(17)] = cm4_mbox_m4b2m4a_sw_int,
        [INT(18)] = mbox_a7n_wake_m4a_int,
        [INT(19)] = mbox_m4b_wake_m4a_int,
        [INT(20)] = gpio_g0_irq0, // EINT0
        [INT(21)] = gpio_g0_irq1, // EINT1
        [INT(22)] = gpio_g0_irq2, // EINT2
        [INT(23)] = gpio_g0_irq3, // EINT3
        [INT(24)] = gpio_g1_irq0, // EINT4
        [INT(25)] = gpio_g1_irq1, // EINT5
        [INT(26)] = gpio_g1_irq2, // EINT6
        [INT(27)] = gpio_g1_irq3, // EINT7
        [INT(28)] = gpio_g2_irq0, // EINT8
        [INT(29)] = gpio_g2_irq1, // EINT9
        [INT(30)] = gpio_g2_irq2, // EINT10
        [INT(31)] = gpio_g2_irq3, // EINT11
        [INT(32)] = gpio_g3_irq0, // EINT12
        [INT(33)] = gpio_g3_irq1, // EINT13
        [INT(34)] = gpio_g3_irq2, // EINT14
        [INT(35)] = gpio_g3_irq3, // EINT15
        [INT(36)] = gpio_g4_irq0, // EINT16
        [INT(37)] = gpio_g4_irq1, // EINT17
        [INT(38)] = gpio_g4_irq2, // EINT18
        [INT(39)] = gpio_g4_irq3, // EINT19
        [INT(40)] = gpio_g5_irq0, // EINT20
        [INT(41)] = gpio_g5_irq1, // EINT21
        [INT(42)] = gpio_g5_irq2, // EINT22
        [INT(43)] = gpio_g5_irq3, // EINT23
        [INT(44)] = isu_g0_i2c_irq,
        [INT(45)] = isu_g0_spim_irq,
        [INT(46)] = NULL,
        [INT(47)] = isu_g0_uart_irq_b,
        [INT(48)] = isu_g1_i2c_irq,
        [INT(49)] = isu_g1_spim_irq,
        [INT(50)] = NULL,
        [INT(51)] = isu_g1_uart_irq_b,
        [INT(52)] = isu_g2_i2c_irq,
        [INT(53)] = isu_g2_spim_irq,
        [INT(54)] = NULL,
        [INT(55)] = isu_g2_uart_irq_b,
        [INT(56)] = isu_g3_i2c_irq,
        [INT(57)] = isu_g3_spim_irq,
        [INT(58)] = NULL,
        [INT(59)] = isu_g3_uart_irq_b,
        [INT(60)] = isu_g4_i2c_irq,
        [INT(61)] = isu_g4_spim_irq,
        [INT(62)] = NULL,
        [INT(63)] = isu_g4_uart_irq_b,
        [INT(64)] = isu_g5_i2c_irq,
        [INT(65)] = isu_g5_spim_irq,
        [INT(66)] = NULL,
        [INT(67)] = isu_g5_uart_irq_b,
        [INT(68)] = i2s0_irq_b,
        [INT(69)] = i2s1_irq_b,
        [INT(70)] = adc_irq_b,
        [INT(71)] = NULL,
        [INT(72)] = NULL,
        [INT(73)] = NULL,
        [INT(74)] = gpio_g3_cnt_irq,
        [INT(75)] = gpio_g4_cnt_irq,
        [INT(76)] = gpio_g5_cnt_irq,
        [INT(77)] = m4dma_irq_b,
        [INT(78)] = iom4_CDBGPWRUPREQ,
        [INT(79)] = iom4_CDBGPWRUPACK,
        [INT(80)] = NULL,
        [INT(81)] = NULL,
        [INT(82)] = NULL,
        [INT(83)] = NULL,
        [INT(84)] = NULL,
        [INT(85)] = NULL,
        [INT(86)] = NULL,
        [INT(87)] = NULL,
        [INT(88)] = NULL,
        [INT(89)] = NULL,
        [INT(90)] = NULL,
        [INT(91)] = NULL,
        [INT(92)] = NULL,
        [INT(93)] = NULL,
        [INT(94)] = NULL,
        [INT(95)] = NULL,
        [INT(96)] = NULL,
        [INT(97)] = NULL,
        [INT(98)] = NULL,
        [INT(99)] = NULL,
};

static volatile uint32_t * const VTOR = (volatile uint32_t *)0xE000ED08;

void VectorTableInit(void)
{
    // SCB->VTOR = ExceptionVectorTable
    *VTOR = (uint32_t)ExceptionVectorTable;

    // We have to enable DMA here as it's used in multiple drivers.
    NVIC_EnableIRQ(MT3620_DMA_INTERRUPT, 2);
}



#include "CPUFreq.h"
#include "mt3620/clock.h"

#define CPUFREQ_TOLERANCE_PERCENT 5

bool CPUFreq_Set(unsigned freq)
{
    unsigned freq_high = (freq * (100ULL + CPUFREQ_TOLERANCE_PERCENT)) / 100;
    unsigned freq_low  = (freq * (100ULL - CPUFREQ_TOLERANCE_PERCENT)) / 100;

    unsigned i;
    for (i = 0; i < MT3620_CLOCK_COUNT; i++) {
        if ((mt3620_clock_freq[i] >= freq_low)
            && (mt3620_clock_freq[i] <= freq_high)) {
            mt3620_hclk_clock_source_set(i);
            return true;
        }
    }

    return false;
}

unsigned CPUFreq_Get(void)
{
    mt3620_clock_t src = mt3620_hclk_clock_source_get();
    if (src >= MT3620_CLOCK_COUNT) return 0;
    return mt3620_clock_freq[src];
}
