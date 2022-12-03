/* Copyright (c) Microsoft Corporation. All rights reserved.
Licensed under the MIT License. */

// This sample C application for Azure Sphere demonstrates General-Purpose Input/Output (GPIO)
// peripherals using a blinking LED and a button.
// The blink rate can be changed through a button press.
//
// It uses the API for the following Azure Sphere application libraries:
// - gpio (digital input for button, digital output for LED)
// - azs::debug! (displays messages in the Device Output window during debugging)
// - eventloop (system invokes handlers for IO events)
use azs::applibs::eventloop::{EventLoop, IoCallback, IoEvents};
use azs::applibs::eventloop_timer_utilities;
use azs::applibs::gpio;
use azs::applibs::gpio::{InputPin, OutputPin, Value};
use azure_sphere as azs;
use std::sync::atomic::{AtomicBool, Ordering};
use std::sync::atomic::{AtomicI32, AtomicU8};
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
const STEP_EVENTLOOP: i32 = 5;
const STEP_INIT_BUTTON: i32 = 6;
const STEP_INIT_LED: i32 = 7;
const STEP_LED_TIMERHANDLER_CONSUME: i32 = 8;
const STEP_LED_TIMERHANDLER_LED_SET_STATE: i32 = 9;
const STEP_BUTTON_TIMERHANDLER_CONSUME: i32 = 10;
const STEP_CHECK_BUTTON1: i32 = 11;

/// Currently executing program step
static STEP: AtomicI32 = AtomicI32::new(STEP_SUCCESS);

static BLINK_INTERVALS: [Duration; 3] = [
    Duration::new(0, 125 * 1000 * 1000),
    Duration::new(0, 250 * 1000 * 1000),
    Duration::new(0, 500 * 1000 * 1000),
];

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

struct Button {
    button: InputPin,
    old_value: AtomicU8,
}

impl Button {
    pub fn new(id: u32) -> Result<Self, std::io::Error> {
        let button = InputPin::new(id)?;
        Ok(Self {
            button,
            old_value: AtomicU8::new(0u8),
        })
    }

    pub fn is_pressed(&self) -> Result<bool, std::io::Error> {
        match self.button.value() {
            Ok(new_state) => {
                let is_pressed = (new_state as u8) != self.old_value.load(Ordering::Relaxed)
                    && new_state == gpio::Value::Low;
                self.old_value.store(new_state as u8, Ordering::Relaxed);
                Ok(is_pressed)
            }
            Err(e) => {
                azs::debug!("ERROR: could not read button GPIO: {:?}\n", e);
                Err(e)
            }
        }
    }
}

struct Led {
    led: OutputPin,
    state: gpio::Value,
}

impl Led {
    pub fn new(id: u32, initial_value: gpio::Value) -> Result<Self, std::io::Error> {
        let led = OutputPin::new(id, gpio::OutputMode::PushPull, initial_value)?;
        Ok(Self {
            led,
            state: initial_value,
        })
    }

    pub fn toggle(&mut self) -> Result<(), std::io::Error> {
        let new_state = if self.state == Value::High {
            Value::Low
        } else {
            Value::High
        };
        self.led.set_value(new_state)?;
        self.state = new_state;
        Ok(())
    }
}

struct BlinkTimer {
    led: Led,
    elt: Arc<eventloop_timer_utilities::EventLoopTimer>,
}

impl BlinkTimer {
    fn new(
        elt: Arc<eventloop_timer_utilities::EventLoopTimer>,
        led: Led,
        period: Duration,
    ) -> Result<Self, std::io::Error> {
        elt.set_period(period)?;
        Ok(Self { led, elt })
    }
}

impl IoCallback for BlinkTimer {
    fn event(&mut self, _events: IoEvents) {
        set_step!(STEP_LED_TIMERHANDLER_CONSUME);
        let ret = self.elt.consume_event();
        if ret.is_err() {
            azs::debug!(
                "EventLoopTimer::consume_event() failed with {:?}\n",
                ret.err()
            );
            std::process::exit(get_step!());
        }

        set_step!(STEP_LED_TIMERHANDLER_LED_SET_STATE);
        let _ = self.led.toggle();
    }

    unsafe fn fd(&self) -> i32 {
        self.elt.fd()
    }
}

struct ButtonsChecker {
    button: Button,
    elt: eventloop_timer_utilities::EventLoopTimer,
    blink_interval_index: usize,
    leds_elt: Arc<eventloop_timer_utilities::EventLoopTimer>,
}

impl ButtonsChecker {
    fn new(
        button: Button,
        period: Duration,
        leds_elt: Arc<eventloop_timer_utilities::EventLoopTimer>,
    ) -> Result<Self, std::io::Error> {
        let elt = eventloop_timer_utilities::EventLoopTimer::new()?;
        elt.set_period(period)?;
        Ok(Self {
            button,
            elt,
            blink_interval_index: 0,
            leds_elt,
        })
    }
}

impl IoCallback for ButtonsChecker {
    fn event(&mut self, _events: IoEvents) {
        let mut event_handler = || -> Result<(), std::io::Error> {
            set_step!(STEP_BUTTON_TIMERHANDLER_CONSUME);
            self.elt.consume_event()?;

            // Check if BUTTON_1 (A) was pressed.
            set_step!(STEP_CHECK_BUTTON1);
            if self.button.is_pressed()? {
                self.blink_interval_index = (self.blink_interval_index + 1) % BLINK_INTERVALS.len();
                self.leds_elt
                    .set_period(BLINK_INTERVALS[self.blink_interval_index])?;
            }
            Ok(())
        };
        if let Err(e) = event_handler() {
            azs::debug!("Button timer callback failed with {:?}\n", e);
            std::process::exit(get_step!());
        }
    }

    unsafe fn fd(&self) -> i32 {
        self.elt.fd()
    }
}

// A main(), except that it returns a Result<T,E>, making it easy to invoke functions using the '?' operator.
fn actual_main<'a>() -> Result<(), std::io::Error> {
    azs::debug!("GPIO application starting.\n");

    //
    // InitPeripheralsAndHandlers() inlined into main() as the created objects must all remain live
    //
    let term = hook_sigterm()?;

    let mut event_loop = EventLoop::new()?;

    // Open SAMPLE_LED GPIO, set as output with value GPIO_Value_High (off), and set up a timer to
    // blink it
    azs::debug!("Opening SAMPLE_LED as output.\n");
    set_step!(STEP_INIT_LED);
    let elt = eventloop_timer_utilities::EventLoopTimer::new()?;
    let elt = std::sync::Arc::new(elt);
    let blinking_led = Led::new(hardware::sample_appliance::SAMPLE_LED, Value::High)?;
    let mut blink_timer = BlinkTimer::new(elt.clone(), blinking_led, BLINK_INTERVALS[0])?;
    event_loop.register_io(IoEvents::Input, &mut blink_timer)?;

    // Open SAMPLE_BUTTON_1 GPIO as input, and set up a timer to poll it
    azs::debug!("Opening SAMPLE_BUTTON_1 as input.\n");
    set_step!(STEP_INIT_BUTTON);
    let led_blink_rate_button = Button::new(hardware::sample_appliance::SAMPLE_BUTTON_1)?;
    let button_press_check_period = Duration::new(0, 1000000);
    let mut button_checker = ButtonsChecker::new(
        led_blink_rate_button,
        button_press_check_period,
        elt.clone(),
    )?;
    event_loop.register_io(IoEvents::Input, &mut button_checker)?;

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
