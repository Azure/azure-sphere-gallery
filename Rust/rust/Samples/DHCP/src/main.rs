/* Copyright (c) Microsoft Corporation. All rights reserved.
Licensed under the MIT License. */

// This sample Rust application for Azure Sphere demonstrates how to use the DHCP client APIs.
// It shows how to:
// 1. Renew the current IP address.
// 2. Release the current IP address.

// It uses the API for the following Azure Sphere application libraries:
// - eventloop (system invokes handlers for timer events).
// - gpio (digital input for button, digital output for LED).
// - azs::debug! (displays messages in the Device Output window during debugging).
// - networking (functions to renew/release the current IP address from network's DHCP server).

use azs::applibs::eventloop::{EventLoop, IoCallback, IoEvents};
use azs::applibs::eventloop_timer_utilities;
use azs::applibs::gpio;
use azs::applibs::gpio::{InputPin, OutputPin};
use azs::applibs::networking;
use azs::applibs::networking::ConnectionStatus;
use azure_sphere as azs;
use pnet::datalink;
use std::sync::atomic::{AtomicBool, AtomicI32, AtomicU32, AtomicU8, Ordering};
use std::sync::Arc;
use std::time::Duration;
extern crate pnet;

// Porting notes from C:
//  Rust doesn't have mutable global variables.  So...
//    exitCode:  replaced by STEP enumeration, of what is about to be attempted, via the get_step and set_step macros.  And std::io::Error(), which wraps errno nicely, and supports custom extensions
//    eventLoop: moved into main() as a local variable
//    callback:  moved into a lambda

const STEP_SUCCESS: i32 = 0;
const STEP_TERMHANDLER_SIGTERM: i32 = 1;
const STEP_SIGNAL_REGISTRATION: i32 = 2;
const STEP_EVENTLOOP: i32 = 3;
const STEP_INIT_BUTTON1_OPEN: i32 = 4;
const STEP_INIT_BUTTON2_OPEN: i32 = 5;
const STEP_INIT_RED_LED: i32 = 6;
const STEP_INIT_BLUE_LED: i32 = 7;
const STEP_INIT_GREEN_LED: i32 = 8;
const STEP_INIT_EVENTLOOP: i32 = 9;
const STEP_INIT_BUTTON_TIMER: i32 = 10;
const STEP_INIT_NETWORK_STATUS_TIMER: i32 = 11;
const STEP_NETWORK_STATUS_TIMERHANDLER_CONSUME: i32 = 12;
const STEP_BUTTON_TIMERHANDLER_CONSUME: i32 = 13;
const STEP_CHECK_BUTTON1: i32 = 14;
const STEP_CHECK_BUTTON2: i32 = 15;

const NET_INTERFACE_WLAN: &str = "wlan0";
const NET_INTERFACE_ETHERNET: &str = "eth0";
const CURRENT_NET_INTERFACE: &str = NET_INTERFACE_WLAN;

static INTERFACE_STATUS: AtomicU32 = AtomicU32::new(0u32);

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

fn release_ip_config() {
    let result = networking::release_ip(CURRENT_NET_INTERFACE);
    if let Err(e) = result {
        azs::debug!("ERROR: Networking_IpConfig_ReleaseIp() failed: {:?}\n", e);
    } else {
        azs::debug!("INFO: Successfully released the IP address.\n");
    }
}

fn renew_ip_config() {
    let result = networking::renew_ip(CURRENT_NET_INTERFACE);
    if let Err(e) = result {
        azs::debug!("ERROR: Networking_IpConfig_RenewIp() failed: {:?}\n", e);
    } else {
        azs::debug!("INFO: Successfully renewed the IP address.\n");
    }
}

fn get_ip_address() -> String {
    for iface in datalink::interfaces() {
        if iface.name == CURRENT_NET_INTERFACE {
            for ip in iface.ips {
                if ip.is_ipv4() {
                    return ip.to_string();
                }
            }
        }
    }
    return "".to_string();
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

struct Leds {
    red: OutputPin,
    green: OutputPin,
    blue: OutputPin,
}

impl Leds {
    pub fn new() -> Result<Self, std::io::Error> {
        set_step!(STEP_INIT_RED_LED);
        let red = OutputPin::new(
            hardware::sample_appliance::SAMPLE_RGBLED_RED,
            gpio::OutputMode::PushPull,
            gpio::Value::Low,
        )?;

        set_step!(STEP_INIT_GREEN_LED);
        let green = OutputPin::new(
            hardware::sample_appliance::SAMPLE_RGBLED_GREEN,
            gpio::OutputMode::PushPull,
            gpio::Value::High,
        )?;

        set_step!(STEP_INIT_BLUE_LED);
        let blue = OutputPin::new(
            hardware::sample_appliance::SAMPLE_RGBLED_BLUE,
            gpio::OutputMode::PushPull,
            gpio::Value::High,
        )?;

        Ok(Self { red, blue, green })
    }

    fn gpio_value(on: bool) -> gpio::Value {
        if on {
            gpio::Value::Low
        } else {
            gpio::Value::High
        }
    }

    pub fn set_led_states(&self, red: bool, green: bool, blue: bool) {
        let _ = self.red.set_value(Self::gpio_value(red));
        let _ = self.green.set_value(Self::gpio_value(green));
        let _ = self.blue.set_value(Self::gpio_value(blue));
    }
}

impl Drop for Leds {
    fn drop(&mut self) {}
}

fn enable_current_network_interface() {
    azs::debug!(
        "INFO: Attempting to enable network interface {:?}.\n",
        CURRENT_NET_INTERFACE
    );

    if let Err(e) = networking::set_interface_state(CURRENT_NET_INTERFACE, true) {
        azs::debug!(
            "ERROR: enabling network interface {:?}: {:?}.\n",
            CURRENT_NET_INTERFACE,
            e
        );
    } else {
        azs::debug!(
            "INFO: Network interface is now set to {:?}.\n",
            CURRENT_NET_INTERFACE
        );
        // If the network is on Wi-Fi, then disable the Ethernet interface (and vice versa).
        let interface_to_disable = if CURRENT_NET_INTERFACE == NET_INTERFACE_WLAN {
            NET_INTERFACE_ETHERNET
        } else {
            NET_INTERFACE_WLAN
        };
        if let Err(e) = networking::set_interface_state(interface_to_disable, false) {
            azs::debug!(
                "ERROR: Disabling network interface {:?}: {:?}.\n",
                interface_to_disable,
                e
            );
        }
    }
}

fn display_interface_status(leds: &Leds, current_interface_status: ConnectionStatus) {
    if current_interface_status.is_none() {
        // The network interface is unavailable.
        // Turn all LEDs off.
        leds.set_led_states(false, false, false);
        azs::debug!(
            "ERROR: network interface '{:?}' NOT ready!\n",
            CURRENT_NET_INTERFACE
        );
        enable_current_network_interface();
    } else if current_interface_status == ConnectionStatus::InterfaceUp {
        // The network interface is up and available, but hasn't yet connected
        // to the network.
        // Turn on the RED LED.
        leds.set_led_states(true, false, false);
        azs::debug!(
            "INFO: Network interface {:?} is up but not connected to the network.\n",
            CURRENT_NET_INTERFACE
        );
    } else if current_interface_status
        == ConnectionStatus::InterfaceUp | ConnectionStatus::ConnectedToNetwork
    {
        // The network interface is up and connected to the network, but hasn't yet
        // received an IP address from the network's DHCP server.
        // Turn on the RED+GREEN LEDs for a YELLOW.
        leds.set_led_states(true, true, false);
        azs::debug!(
            "INFO: Network interface {:?} is connected to the network (no IP address assigned).\n",
            CURRENT_NET_INTERFACE
        );
    } else if current_interface_status
        == ConnectionStatus::InterfaceUp
            | ConnectionStatus::ConnectedToNetwork
            | ConnectionStatus::IpAvailable
    {
        // The network interface is up, connected to the network and successfully
        // acquired an IP address from the network's DHCP server.
        // Turn on the BLUE LED.
        leds.set_led_states(false, false, true);
        azs::debug!(
            "INFO: Network interface {:?} is connected and has been assigned IP address [{:?}].\n",
            CURRENT_NET_INTERFACE,
            get_ip_address()
        );
    } else if current_interface_status
        == ConnectionStatus::InterfaceUp
            | ConnectionStatus::ConnectedToNetwork
            | ConnectionStatus::IpAvailable
            | ConnectionStatus::ConnectedToInternet
    {
        // The network interface is fully operative and connected up to the Internet.
        // Turn on the GREEN LED.
        leds.set_led_states(false, true, false);
        azs::debug!(
            "INFO: Network interface {:?} is connected to the Internet (local IP address [{:?}]).\n",
            CURRENT_NET_INTERFACE, get_ip_address());
    } else {
        // The network interface is in a transient state.
        // Turn all LEDs off.
        leds.set_led_states(false, false, false);
        azs::debug!(
            "INFO: Network interface {:?} is in a transient state [{:?}].\n",
            CURRENT_NET_INTERFACE,
            current_interface_status
        );
    }
}

struct ButtonsChecker {
    release_ip_button: Button,
    renew_ip_button: Button,
    elt: eventloop_timer_utilities::EventLoopTimer,
}

impl ButtonsChecker {
    fn new(
        release_ip_button: Button,
        renew_ip_button: Button,
        period: Duration,
    ) -> Result<Self, std::io::Error> {
        let elt = eventloop_timer_utilities::EventLoopTimer::new()?;
        elt.set_period(period)?;
        Ok(Self {
            release_ip_button,
            renew_ip_button,
            elt,
        })
    }
}

impl IoCallback for ButtonsChecker {
    fn event(&mut self, _events: IoEvents) {
        let event_handler = || -> Result<(), std::io::Error> {
            set_step!(STEP_BUTTON_TIMERHANDLER_CONSUME);
            self.elt.consume_event()?;

            // Check if BUTTON_1 (A) was pressed.
            set_step!(STEP_CHECK_BUTTON1);
            if self.release_ip_button.is_pressed()? {
                release_ip_config();
            }

            // Check if BUTTON_2 (B) was pressed.
            set_step!(STEP_CHECK_BUTTON2);
            if self.renew_ip_button.is_pressed()? {
                renew_ip_config();
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

struct LedsChecker {
    leds: Leds,
    elt: eventloop_timer_utilities::EventLoopTimer,
}

impl LedsChecker {
    pub fn new(leds: Leds, period: Duration) -> Result<Self, std::io::Error> {
        let elt = eventloop_timer_utilities::EventLoopTimer::new()?;
        elt.set_period(period)?;
        Ok(Self { leds, elt })
    }
}

impl IoCallback for LedsChecker {
    fn event(&mut self, _events: IoEvents) {
        let event_handler = || -> Result<(), std::io::Error> {
            set_step!(STEP_NETWORK_STATUS_TIMERHANDLER_CONSUME);
            self.elt.consume_event()?;

            // For the UX purposes of this sample, the Networking_GetInterfaceConnectionStatus()
            // API is called more frequently than the minimum recommended interval, which may lead
            // to receive transient states in return. These are managed below in the switch's default
            // statement.
            let current_interface_status =
                networking::get_interface_connection_status(CURRENT_NET_INTERFACE);
            match current_interface_status {
                Err(e) => {
                    azs::debug!(
                        "ERROR: retrieving the {:?} network interface's status: {:?}.\n",
                        CURRENT_NET_INTERFACE,
                        e
                    );
                }
                Ok(current_interface_status) => {
                    // Load old status u32 and convert to a ConnectionStatus bitmask enum
                    let old_status =
                        ConnectionStatus::from(INTERFACE_STATUS.load(Ordering::Relaxed));
                    if old_status == 0 || old_status != current_interface_status {
                        INTERFACE_STATUS.store(current_interface_status.bits(), Ordering::Relaxed);
                        display_interface_status(&self.leds, current_interface_status);
                    }
                }
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
fn actual_main() -> Result<(), std::io::Error> {
    let term = hook_sigterm()?;

    //
    // InitPeripheralsAndHandlers() inlined into main() as the created objects must all remain live
    //
    set_step!(STEP_INIT_BUTTON1_OPEN);
    let release_ip_button = Button::new(hardware::sample_appliance::SAMPLE_BUTTON_1)?;

    set_step!(STEP_INIT_BUTTON2_OPEN);
    let renew_ip_button = Button::new(hardware::sample_appliance::SAMPLE_BUTTON_2)?;

    let leds = Leds::new()?;

    set_step!(STEP_INIT_EVENTLOOP);
    let mut event_loop = EventLoop::new()?;

    set_step!(STEP_INIT_BUTTON_TIMER);
    let button_timer_period = Duration::new(0, 50 * 1000 * 1000); // every 50 milliseconds
    let mut buttons_checker =
        ButtonsChecker::new(release_ip_button, renew_ip_button, button_timer_period)?;
    event_loop.register_io(IoEvents::Input, &mut buttons_checker)?;

    set_step!(STEP_INIT_NETWORK_STATUS_TIMER);
    let network_status_period = Duration::new(1, 0); // every 1 second
    let mut leds_checker = LedsChecker::new(leds, network_status_period)?;
    event_loop.register_io(IoEvents::Input, &mut leds_checker)?;

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
