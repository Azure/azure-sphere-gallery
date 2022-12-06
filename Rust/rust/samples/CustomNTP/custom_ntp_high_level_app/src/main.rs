/* Copyright (c) Microsoft Corporation. All rights reserved.
Licensed under the MIT License. */

// This sample C application for Azure Sphere demonstrates how to use the custom NTP server APIs.
// It shows how to:
// 1. Configure the default NTP server.
// 2. Configure the automatic NTP server.
// 3. Configure up to two custom NTP servers.
// 4. Get the last NTP sync information.

// It uses the API for the following Azure Sphere application libraries:
// - eventloop (system invokes handlers for timer events).
// - gpio (digital input for button, digital output for LED).
// - azs::debug! (displays messages in the Device Output window during debugging).
// - networking (functions to configure the NTP and retrieve last synced
// NTP information).

// You will need struct Args in actual_main() in order to
// use this application. Please see README.md for full details.
use azs::applibs::eventloop::EventLoop;
use azs::applibs::eventloop::{IoCallback, IoEvents};
use azs::applibs::eventloop_timer_utilities;
use azs::applibs::gpio;
use azs::applibs::gpio::{InputPin, OutputPin, Value};
use azs::applibs::networking;
use azure_sphere as azs;
use chrono::{DateTime, Utc};
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
const STEP_EVENTLOOP: i32 = 3;
const STEP_VALIDATE_PRIMARY_NTP_SERVER: i32 = 4;
const STEP_GET_LAST_NTP_SYNC_INFO: i32 = 5;
const STEP_LED_IS_NETWORK_READY: i32 = 6;
const STEP_ENABLE_TIME_SYNC: i32 = 7;
const STEP_DEFAULT_NTP_SERVER: i32 = 8;

/// Currently executing program step
static STEP: AtomicI32 = AtomicI32::new(STEP_SUCCESS);

static NETWORK_READY: AtomicBool = AtomicBool::new(false);

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

#[allow(dead_code)]
#[derive(Copy, Clone, PartialEq, Eq, PartialOrd, Ord, Debug)]
enum TimeSource {
    Default,
    Automatic,
    Custom,
}

#[derive(Debug)]
struct Args {
    time_source: TimeSource,
    primary_ntp_server: Option<String>,
    secondary_ntp_server: Option<String>,
    disable_fallback: bool,
}

fn validate_user_configuration(args: &Args) -> Result<(), std::io::Error> {
    match args.time_source {
        TimeSource::Custom => {
            if args.primary_ntp_server.is_none() {
                set_step!(STEP_VALIDATE_PRIMARY_NTP_SERVER);
                Err(Error::new(
                    std::io::ErrorKind::Other,
                    "Missing primary NTP server",
                ))
            } else {
                Ok(())
            }
        }
        _ => Ok(()),
    }
}

// Retrieves the last NTP sync information.
fn get_last_ntp_sync_information() -> bool {
    if !NETWORK_READY.load(Ordering::Relaxed) {
        azs::debug!("Device has not yet successfully time synced.\n");
        true
    } else {
        let mut time_before_sync: DateTime<Utc> = Default::default();
        let mut ntp_time: DateTime<Utc> = Default::default();

        set_step!(STEP_GET_LAST_NTP_SYNC_INFO);
        let server =
            networking::get_last_ntp_sync_info(Some(&mut time_before_sync), Some(&mut ntp_time));
        match server {
            Err(e) => {
                azs::debug!("ERROR: Get last NTP sync info failed: {:?}\n", e);
                false
            }
            Ok(None) => {
                azs::debug!("INFO: The device has not yet successfully completed a time sync.\n");
                true
            }
            Ok(Some(ntp_server)) => {
                azs::debug!("\nSuccessfully time synced to server {:?}\n", ntp_server);

                azs::debug!("\nTime before sync:\n");
                azs::debug!("UTC time        : {:?}\n", time_before_sync.format("%c"));

                azs::debug!("\nTime after sync:\n");
                azs::debug!("UTC time        : {:?}\n", ntp_time.format("%c"));

                true
            }
        }
    }
}

fn configure_default_ntp_server(_args: &Args) -> Result<(), std::io::Error> {
    azs::debug!("\nConfiguring Default NTP server\n");
    set_step!(STEP_DEFAULT_NTP_SERVER);
    networking::enable_default_ntp()
}

fn configure_automatic_ntp_server(args: &Args) -> Result<(), std::io::Error> {
    azs::debug!("\nConfiguring Automatic NTP server\n");
    azs::debug!("Fallback Server NTP Option: {:?}\n", args.disable_fallback);
    set_step!(STEP_DEFAULT_NTP_SERVER);
    let option = if args.disable_fallback {
        networking::NtpOption::FallbackServerEnabled
    } else {
        networking::NtpOption::FallbackServerDisabled
    };

    networking::enable_automatic_ntp(option)
}

fn configure_custom_ntp_server(args: &Args) -> Result<(), std::io::Error> {
    // Paramter validation earlier ensures this is Some()
    let primary_ntp_server = args.primary_ntp_server.as_ref().unwrap();

    azs::debug!("\nConfiguring Custom NTP server\n");
    azs::debug!("Primary Server: {:?}\n", primary_ntp_server);
    let s = if let Some(secondary) = args.secondary_ntp_server.as_ref() {
        azs::debug!("Secondary Server: {:?}\n", secondary);
        Some(secondary)
    } else {
        None
    };

    let option = if args.disable_fallback {
        networking::NtpOption::FallbackServerEnabled
    } else {
        networking::NtpOption::FallbackServerDisabled
    };

    networking::enable_custom_ntp(primary_ntp_server, s, option)
}

fn configure_ntp_server(args: &Args) -> Result<(), std::io::Error> {
    set_step!(STEP_ENABLE_TIME_SYNC);
    networking::set_timesync_enabled(true)?;

    match args.time_source {
        TimeSource::Default => configure_default_ntp_server(args),
        TimeSource::Automatic => configure_automatic_ntp_server(args),
        TimeSource::Custom => configure_custom_ntp_server(args),
    }?;

    Ok(())
}

struct UpdateLastSyncButton {
    input_pin: InputPin,
    elt: eventloop_timer_utilities::EventLoopTimer,
    old_value: Value,
    term: Arc<AtomicBool>,
}

impl UpdateLastSyncButton {
    fn new(
        input_pin: InputPin,
        period: Duration,
        term: Arc<AtomicBool>,
    ) -> Result<Self, std::io::Error> {
        let elt = eventloop_timer_utilities::EventLoopTimer::new()?;
        elt.set_period(period)?;
        Ok(Self {
            input_pin,
            elt,
            old_value: Value::High,
            term,
        })
    }
}

impl IoCallback for UpdateLastSyncButton {
    fn event(&mut self, _events: IoEvents) {
        self.elt.consume_event().unwrap();

        let result = self.input_pin.value();
        if let Ok(new_state) = result {
            let is_button_pressed = {
                let is_pressed = new_state != self.old_value && new_state == gpio::Value::Low;
                self.old_value = new_state;
                is_pressed
            };
            if is_button_pressed {
                if get_last_ntp_sync_information() == false {
                    self.term.store(false, Ordering::Relaxed);
                }
            }
        } else {
            azs::debug!("ERROR: Could not read button GPIO {:?}\n", result.err());
        }
    }

    unsafe fn fd(&self) -> i32 {
        self.elt.fd()
    }
}

struct Leds {
    not_synced_led: OutputPin,
    synced_led: OutputPin,
    elt: eventloop_timer_utilities::EventLoopTimer,
}

impl Leds {
    pub fn new(
        not_synced_led: OutputPin,
        synced_led: OutputPin,
        period: Duration,
    ) -> Result<Self, std::io::Error> {
        let elt = eventloop_timer_utilities::EventLoopTimer::new()?;
        elt.set_period(period)?;
        Ok(Self {
            not_synced_led,
            synced_led,
            elt,
        })
    }
}

impl IoCallback for Leds {
    fn event(&mut self, _events: IoEvents) {
        self.elt.consume_event().unwrap();

        set_step!(STEP_LED_IS_NETWORK_READY);
        let current_networking_ready = networking::is_networking_ready();
        let current_networking_ready = if let Ok(ready) = current_networking_ready {
            ready
        } else {
            azs::debug!(
                "ERROR: Networking_IsNetworkingReady: {:?}\n",
                current_networking_ready.err()
            );
            std::process::exit(get_step!());
        };
        if !current_networking_ready {
            azs::debug!(concat!(
                "Device has not yet successfully time synced. Ensure that the network interface is ",
                "up and the configured NTP server is valid.\n")
                );
        };
        if current_networking_ready != NETWORK_READY.load(Ordering::Relaxed) {
            // Toggle the states
            NETWORK_READY.store(current_networking_ready, Ordering::Relaxed);
            if current_networking_ready {
                // Network is ready. Turn off Red LED. Turn on Green LED.
                let _ = self.not_synced_led.set_value(gpio::Value::High);
                let _ = self.synced_led.set_value(gpio::Value::Low);
            } else {
                // Network is not ready. Turn on Red LED. Turn off Green LED.
                let _ = self.not_synced_led.set_value(gpio::Value::Low);
                let _ = self.synced_led.set_value(gpio::Value::High);
            }
        }
    }

    unsafe fn fd(&self) -> i32 {
        self.elt.fd()
    }
}

// A main(), except that it returns a Result<T,E>, making it easy to invoke functions using the '?' operator.
fn actual_main() -> Result<(), std::io::Error> {
    azs::debug!("INFO: Custom NTP High Level Application starting.\n");

    // Hard-code configuration options here.
    let args = Args {
        time_source: TimeSource::Default,
        primary_ntp_server: None,
        secondary_ntp_server: None,
        disable_fallback: false,
    };

    validate_user_configuration(&args)?;

    //
    // InitPeripheralsAndHandlers() inlined into main() as the created objects must all remain live
    //
    let term = hook_sigterm()?;
    let mut event_loop = EventLoop::new()?;

    // Open SAMPLE_BUTTON_1 GPIO as input, and set up a timer to poll it.
    let last_ntp_sync_info_button = InputPin::new(hardware::sample_appliance::SAMPLE_BUTTON_1)?;
    let last_ntp_sync_period = Duration::new(0, 100 * 1000 * 1000);
    let mut button = UpdateLastSyncButton::new(
        last_ntp_sync_info_button,
        last_ntp_sync_period,
        term.clone(),
    )?;
    event_loop.register_io(IoEvents::Input, &mut button)?;

    // Open LEDs for NTP sync status.
    // Turn on Red LED at startup, till we get a successful sync.
    let not_synced_led = OutputPin::new(
        hardware::sample_appliance::SAMPLE_RGBLED_RED,
        gpio::OutputMode::PushPull,
        gpio::Value::Low,
    )?;
    let synced_led = OutputPin::new(
        hardware::sample_appliance::SAMPLE_RGBLED_GREEN,
        gpio::OutputMode::PushPull,
        gpio::Value::High,
    )?;
    let ntp_sync_status_period = Duration::new(1, 0);
    let mut leds = Leds::new(not_synced_led, synced_led, ntp_sync_status_period)?;
    event_loop.register_io(IoEvents::Input, &mut leds)?;

    configure_ntp_server(&args)?;

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
