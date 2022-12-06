/* Copyright (c) Microsoft Corporation. All rights reserved.
Licensed under the MIT License. */

// This sample Rust application for Azure Sphere demonstrates how to use ADC (Analog to Digital
// Conversion).
// The sample opens an ADC controller which is connected to a potentiometer. Adjusting the
// potentiometer will change the displayed values.
//
// It uses the API for the following Azure Sphere application libraries:
// - adc (Analog to Digital Conversion)
// - log (displays messages in the Device Output window during debugging)
// - eventloop (system invokes handlers for timer events)
use azs::applibs::adc;
use azs::applibs::eventloop::{EventLoop, IoCallback, IoEvents};
use azs::applibs::eventloop_timer_utilities;
use azure_sphere as azs;
use nullable_result::NullableResult;
use std::io::Error;
use std::sync::atomic::AtomicI32;
use std::sync::atomic::{AtomicBool, Ordering};
use std::sync::Arc;
use std::time::Duration;

// Porting notes from C:
//  Rust doesn't have mutable global variables.  So...
//    exitCode:  replaced by STEP enumeration, of what is about to be attempted, via the get_step and set_step macros.  And std::io::Error(), which wraps errno nicely, and supports custom extensions
//    eventLoop: moved into main() as a local variable
//    callback:  moved into a lambda

const STEP_SUCCESS: i32 = 0;
const STEP_TERMHANDLER_SIGTERM: i32 = 1;
const STEP_SIGNAL_REGISTRATION: i32 = 2;
const STEP_ADC_GET_SAMPLE_BITCOUNT: i32 = 3;
const STEP_ADC_TIMERHANDLER_CONSUME: i32 = 4;
const STEP_ADC_TIMERHANDLER_POLL: i32 = 5;
const STEP_EVENTLOOP: i32 = 6;

/// The maximum voltage
const SAMPLE_MAX_VOLTAGE: f32 = 2.5;

/// Currently executing program step
static STEP: AtomicI32 = AtomicI32::new(STEP_SUCCESS);

/// Macro to assign a new value to STEP.
macro_rules! set_step {
    ($i:ident) => {
        STEP.store($i, Ordering::Relaxed);
    };
}
/// Macro to read the current STEP value
macro_rules! get_step {
    () => {
        STEP.load(Ordering::Relaxed)
    };
}

/// Hook SIGTERM so that it modifies the returned AtomicBool if signalled
fn hook_sigterm() -> Result<Arc<AtomicBool>, std::io::Error> {
    set_step!(STEP_SIGNAL_REGISTRATION);
    let term = Arc::new(AtomicBool::new(false));
    signal_hook::flag::register_conditional_shutdown(
        signal_hook::consts::SIGTERM,
        STEP_TERMHANDLER_SIGTERM,
        Arc::clone(&term),
    )?;

    Ok(term)
}

struct AdcTimerEvents {
    elt: eventloop_timer_utilities::EventLoopTimer,
    adc_controller: adc::AdcController,
    sample_bit_count: i32,
}

impl AdcTimerEvents {
    pub fn new(
        adc_controller: adc::AdcController,
        sample_bit_count: i32,
        period: Duration,
    ) -> Result<Self, std::io::Error> {
        let elt = eventloop_timer_utilities::EventLoopTimer::new()?;
        elt.set_period(period)?;
        Ok(Self {
            adc_controller,
            elt,
            sample_bit_count,
        })
    }
}

impl IoCallback for AdcTimerEvents {
    fn event(&mut self, _events: IoEvents) {
        set_step!(STEP_ADC_TIMERHANDLER_CONSUME);
        let ret = self.elt.consume_event();
        if ret.is_err() {
            azs::debug!(
                "EventLoopTimer::consume_event() failed with {:?}\n",
                ret.err()
            );
            std::process::exit(get_step!());
        }

        set_step!(STEP_ADC_TIMERHANDLER_POLL);
        let value = self
            .adc_controller
            .poll(hardware::sample_appliance::SAMPLE_POTENTIOMETER_ADC_CHANNEL);
        match value {
            NullableResult::Err(e) => {
                azs::debug!("adc_controller::poll() failed with {:?}\n", e);
                std::process::exit(get_step!());
            }
            NullableResult::Ok(voltage) => {
                let voltage = ((voltage as f32) * SAMPLE_MAX_VOLTAGE)
                    / ((1 << self.sample_bit_count) - 1) as f32;
                azs::debug!("The out sample value is {:?} V\n", voltage);
            }
            NullableResult::Null => {
                azs::debug!("The out sample value is {:?} V\n", 0.0);
            }
        }
    }

    unsafe fn fd(&self) -> i32 {
        return self.elt.fd();
    }
}

// A main(), except that it returns a Result<T,E>, making it easy to invoke functions using the '?' operator.
fn actual_main() -> Result<(), std::io::Error> {
    let term = hook_sigterm()?;

    //
    // InitPeripheralsAndHandlers() inlined into main() as the created objects must all remain live
    //
    let mut event_loop = EventLoop::new()?;

    let adc_controller =
        adc::AdcController::new(hardware::sample_appliance::SAMPLE_POTENTIOMETER_ADC_CONTROLLER)?;

    set_step!(STEP_ADC_GET_SAMPLE_BITCOUNT);
    let sample_bit_count = adc_controller
        .sample_bit_count(hardware::sample_appliance::SAMPLE_POTENTIOMETER_ADC_CHANNEL)?;
    if sample_bit_count == 0 {
        azs::debug!("adc::get_sample_bit_count returned sample size of 0 bits\n");
        return Err(Error::new(
            std::io::ErrorKind::InvalidData,
            "Sample size of 0",
        ));
    }

    adc_controller.set_reference_voltage(
        hardware::sample_appliance::SAMPLE_POTENTIOMETER_ADC_CHANNEL,
        SAMPLE_MAX_VOLTAGE,
    )?;

    let adc_check_period = Duration::new(1, 0);
    let mut adc_timer_events =
        AdcTimerEvents::new(adc_controller, sample_bit_count, adc_check_period)?;
    event_loop.register_io(IoEvents::Input, &mut adc_timer_events)?;

    //
    // Main loop
    //
    while !term.load(Ordering::Relaxed) {
        let result = event_loop.run(-1, true);
        if let Err(e) = result {
            if e.kind() != std::io::ErrorKind::Interrupted {
                std::process::exit(STEP_EVENTLOOP);
            }
        }
    }

    Ok(())
}

pub fn main() -> ! {
    let result = actual_main();
    if result.is_err() {
        azs::debug!("Failed at step {:?} with {:?}\n", get_step!(), result.err());
        std::process::exit(get_step!());
    }

    azs::debug!("Application exiting\n");
    std::process::exit(get_step!());
}
