/* Copyright (c) Microsoft Corporation. All rights reserved.
Licensed under the MIT License. */

// This sample C application for Azure Sphere demonstrates how to use the certificate
// store APIs. Each press of SAMPLE_BUTTON_1 will advance through a cycle that installs,
// moves certificates, reloads the Wi-Fi network (step required for an EAP-TLS network)
// and deletes the certificates. SAMPLE_BUTTON_2 displays the available space on the divice,
// lists the installed certificates, and displays specific information about each certificate.
//
// It uses the API for the following Azure Sphere application libraries:
// - gpio (digital input for button)
// - log (displays messages in the Device Output window during debugging)
// - certstore (functions and types that interact with certificates)
// - wificonfig (functions and types that interact with networking)

use azs::applibs::certstore::Certificate;
use azs::applibs::eventloop::{EventLoop, IoCallback, IoEvents};
use azs::applibs::gpio;
use azs::applibs::gpio::InputPin;
use azs::applibs::wificonfig;
use azs::applibs::{certstore, eventloop_timer_utilities};
use azure_sphere as azs;
use std::io::Read;
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
const STEP_BUTTON_TIMERHANDLER_CONSUME: i32 = 10;
const STEP_CHECK_BUTTON1: i32 = 11;
const STEP_CHECK_BUTTON2: i32 = 12;
const STEP_GET_AVAILABLE_SPACE: i32 = 13;
const STEP_INSTALL_NEW_ROOT_CERTIFICATE: i32 = 15;
const STEP_MOVE_CERTIFICATE: i32 = 16;
const STEP_RELOAD_WIFI_CONFIG: i32 = 17;
const STEP_DELETE_CERTIFICATE: i32 = 18;
const STEP_INSTALL_ROOT_CA_CERTIFICATE: i32 = 19;
const STEP_INSTALL_CLIENT_CERTIFICATE: i32 = 20;
const STEP_GET_CERT_COUNT: i32 = 21;
const STEP_GET_IDENTIFIER: i32 = 22;
const STEP_GET_SUBJECT: i32 = 23;
const STEP_GET_ISSUER: i32 = 24;
const STEP_GET_NOT_BEFORE: i32 = 25;
const STEP_GET_NOT_AFTER: i32 = 26;
const STEP_READ_ROOT_CA: i32 = 27;
const STEP_READ_CLIENT_CERT: i32 = 28;
const STEP_READ_PRIVATE_KEY: i32 = 29;

// Certificate identifiers
const ROOT_CA_CERT_IDENTIFIER: &str = "SmplRootCACertId";
const NEW_ROOT_CA_CERT_IDENTIFIER: &str = "NewRootCACertId";
const CLIENT_CERT_IDENTIFIER: &str = "SmplClientCertId";

// Configure the variable with the password of the client private key
const CLIENT_PRIVATE_KEY_PASSWORD: &str = "client_private_key_password";

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

fn read_imagepackage_file(filename: &str) -> Result<Vec<u8>, std::io::Error> {
    let mut f = azs::applibs::storage::open_in_image_package(filename)?;
    let mut data = Vec::new();
    let _bytes_read = f.read_to_end(&mut data)?;
    Ok(data)
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

fn check_device_space_for_installation(certificate_size: usize) -> Result<bool, std::io::Error> {
    let available_space = certstore::get_certificate_available_space()?;

    Ok(available_space >= certificate_size)
}

type StateFunction = fn(&mut ButtonsChecker) -> Result<(), std::io::Error>;

struct ButtonsChecker {
    button1: Button,
    button2: Button,
    next_state_function: StateFunction,
    elt: eventloop_timer_utilities::EventLoopTimer,
}

impl ButtonsChecker {
    fn new(button1: Button, button2: Button, period: Duration) -> Result<Self, std::io::Error> {
        let elt = eventloop_timer_utilities::EventLoopTimer::new()?;
        elt.set_period(period)?;
        Ok(Self {
            button1,
            button2,
            elt,
            next_state_function: Self::cert_install_state,
        })
    }

    ///     Installs the certificates.
    fn cert_install_state(&mut self) -> Result<(), std::io::Error> {
        set_step!(STEP_READ_ROOT_CA);
        let root_ca = read_imagepackage_file("certs/root.pem")?;
        set_step!(STEP_READ_CLIENT_CERT);
        let client_cert = read_imagepackage_file("certs/cert.pem")?;
        set_step!(STEP_READ_PRIVATE_KEY);
        let private_key = read_imagepackage_file("certs/private_key.pem")?;

        set_step!(STEP_GET_AVAILABLE_SPACE);
        if !check_device_space_for_installation(root_ca.len())? {
            azs::debug!(
                "ERROR: Failed to install the root CA and client certificates because there isn't "
            );
            azs::debug!("enough space on the device.\n");
        } else {
            set_step!(STEP_INSTALL_ROOT_CA_CERTIFICATE);
            certstore::install_root_ca_certificate(ROOT_CA_CERT_IDENTIFIER, &root_ca)?;

            set_step!(STEP_INSTALL_CLIENT_CERTIFICATE);
            certstore::install_client_certificate(
                CLIENT_CERT_IDENTIFIER,
                &client_cert,
                &private_key,
                CLIENT_PRIVATE_KEY_PASSWORD.as_bytes(),
            )?;
        }

        // set the next state
        self.next_state_function = Self::install_new_root_ca_certificate_state;
        azs::debug!(
            "Finished installing the root CA and the client certificates with status: SUCCESS. By "
        );
        azs::debug!("pressing BUTTON_1 the new root CA certificate will be installed.\n");
        Ok(())
    }

    ///    Installs an additional root CA certificate.
    fn install_new_root_ca_certificate_state(&mut self) -> Result<(), std::io::Error> {
        set_step!(STEP_READ_ROOT_CA);
        let root_ca = read_imagepackage_file("certs/new_root.pem")?;
        set_step!(STEP_GET_AVAILABLE_SPACE);
        if check_device_space_for_installation(root_ca.len())? == false {
            azs::debug!(
                "ERROR: Failed to install the root CA and client certificates because there isn't "
            );
            azs::debug!("enough space on the device.\n");
        } else {
            set_step!(STEP_INSTALL_NEW_ROOT_CERTIFICATE);
            certstore::install_root_ca_certificate(NEW_ROOT_CA_CERT_IDENTIFIER, &root_ca)?;

            // set the next state
            self.next_state_function = Self::root_ca_cert_move_state;
            azs::debug!(
                "Finished installing the new root CA certificate with status: SUCCESS. By pressing ");
            azs::debug!("BUTTON_1 the root CA certificate will be replaced by the new root CA certificate.\n");
        }
        Ok(())
    }
    ///     Replace the certificate identified by rootCACertIdentifier with the certificate identified
    ///     by newRootCACertIdentifier. The certificate data previously identified by
    ///     rootCACertIdentifier will be deleted, and the the identifier newRootCACertIdentifier will no
    ///     longer be valid.
    fn root_ca_cert_move_state(&mut self) -> Result<(), std::io::Error> {
        set_step!(STEP_MOVE_CERTIFICATE);
        certstore::move_certificate(
            &Certificate::new(NEW_ROOT_CA_CERT_IDENTIFIER),
            &Certificate::new(ROOT_CA_CERT_IDENTIFIER),
        )?;

        // set the next state
        self.next_state_function = Self::wifi_reload_config_state;
        azs::debug!(
            "Finished replacing the root CA certificate with the new root CA certificate with status: ");
        azs::debug!(
            "SUCCESS. By pressing BUTTON_1 the device Wi-Fi configuration will be reloaded.\n"
        );
        Ok(())
    }

    ///    Reload the device Wi-Fi configuration following changes to the certificate store.
    ///    It is necessary to reload the Wi-Fi config after making any change to the certificate store,
    ///    in order to make the changes available for configuring an EAP-TLS network.
    fn wifi_reload_config_state(&mut self) -> Result<(), std::io::Error> {
        set_step!(STEP_RELOAD_WIFI_CONFIG);
        wificonfig::reload_config()?;

        // set the next state
        self.next_state_function = Self::cert_delete_state;
        azs::debug!(
            "Finished reloading the Wi-Fi configuration with status: SUCCESS. By pressing BUTTON_1 the ");
        azs::debug!("new root CA and client certificates will be deleted.\n");
        Ok(())
    }

    ///    Deletes the installed certificates.
    fn cert_delete_state(&mut self) -> Result<(), std::io::Error> {
        set_step!(STEP_DELETE_CERTIFICATE);
        Certificate::new(ROOT_CA_CERT_IDENTIFIER).delete()?;
        azs::debug!(
            "INFO: Erased certificate with identifier: {:?}",
            ROOT_CA_CERT_IDENTIFIER
        );

        set_step!(STEP_DELETE_CERTIFICATE);
        Certificate::new(CLIENT_CERT_IDENTIFIER).delete()?;
        azs::debug!(
            "INFO: Erased certificate with identifier: {:?}",
            CLIENT_CERT_IDENTIFIER
        );

        // set the next state
        self.next_state_function = Self::cert_install_state;
        azs::debug!(
            "Finished deleting the new root CA and client certificates with status: SUCCESS. By "
        );
        azs::debug!("pressing BUTTON_1 the root CA, new root CA, and client certificates will be installed.\n");
        Ok(())
    }
}

fn display_cert_information() -> Result<(), std::io::Error> {
    set_step!(STEP_GET_AVAILABLE_SPACE);
    let available_space = certstore::get_certificate_available_space()?;
    azs::debug!(
        "INFO: Available space in device certificate store: {:?} B.\n",
        available_space
    );

    set_step!(STEP_GET_CERT_COUNT);
    let cert_count = certstore::get_certificate_count()?;
    if cert_count == 0 {
        azs::debug!("INFO: No certificates installed on this device.\n");
        return Ok(());
    }
    azs::debug!(
        "INFO: There are {:?} certificate(s) installed on this device.\n",
        cert_count
    );

    let mut i = 0;
    for cert in certstore::certificates() {
        set_step!(STEP_GET_IDENTIFIER);
        let identifier = cert.identifer()?;
        azs::debug!(
            "INFO: Certificate {:?} has identifier: {:?}.\n",
            i,
            identifier
        );

        set_step!(STEP_GET_SUBJECT);
        let subject_name = cert.subject_name()?;
        azs::debug!(
            "\tINFO: Certificate subject name: {:?}.\n",
            String::from_utf8_lossy(&subject_name)
        );

        set_step!(STEP_GET_ISSUER);
        let issuer_name = cert.issuer_name()?;
        azs::debug!(
            "\tINFO: Certificate issuer name: {:?}.\n",
            String::from_utf8_lossy(&issuer_name)
        );

        set_step!(STEP_GET_NOT_BEFORE);
        let not_before = cert.not_before()?;
        azs::debug!(
            "\tINFO: Certificate not before validity date: {:?}\n",
            not_before
        );

        set_step!(STEP_GET_NOT_AFTER);
        let not_after = cert.not_after()?;
        azs::debug!(
            "\tINFO: Certificate not before validity date: {:?}\n",
            not_after
        );

        i = i + 1;
    }

    Ok(())
}

impl IoCallback for ButtonsChecker {
    fn event(&mut self, _events: IoEvents) {
        let mut event_handler = || -> Result<(), std::io::Error> {
            set_step!(STEP_BUTTON_TIMERHANDLER_CONSUME);
            self.elt.consume_event()?;

            // Check if BUTTON_1 (A) was pressed.
            set_step!(STEP_CHECK_BUTTON1);
            if self.button1.is_pressed()? {
                let result = (self.next_state_function)(self);
                if let Err(e) = result {
                    azs::debug!("Next State Function failed: {:?}", e);
                    std::process::exit(get_step!());
                }
            }

            // Check if BUTTON_2 (B) was pressed.
            set_step!(STEP_CHECK_BUTTON2);
            if self.button2.is_pressed()? {
                let result = display_cert_information();
                if let Err(e) = result {
                    azs::debug!("display_cert_information failed: {:?}", e);
                    std::process::exit(get_step!());
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
fn actual_main<'a>() -> Result<(), std::io::Error> {
    azs::debug!("Cert application starting.\n");
    azs::debug!(
        "Each press of BUTTON_1 will advance through a cycle that installs, moves certificates, "
    );
    azs::debug!("reloads the Wi-Fi network and deletes the certificates.\n");
    azs::debug!(
        "BUTTON_2 displays the available space on the device, lists the installed certificates, "
    );
    azs::debug!("and displays specific information about each certificate.\n");

    //
    // InitPeripheralsAndHandlers() inlined into main() as the created objects must all remain live
    //
    let term = hook_sigterm()?;

    let mut event_loop = EventLoop::new()?;

    // Open SAMPLE_BUTTON_1 and _2 GPIO as input, and set up a timer to poll them
    azs::debug!("Opening SAMPLE_BUTTON_1 as input.\n");
    set_step!(STEP_INIT_BUTTON);
    let advance_state_button = Button::new(hardware::sample_appliance::SAMPLE_BUTTON_1)?;
    let show_cert_status_button = Button::new(hardware::sample_appliance::SAMPLE_BUTTON_2)?;
    let button_press_check_period = Duration::new(0, 100 * 1000 * 1000);
    let mut button_checker = ButtonsChecker::new(
        advance_state_button,
        show_cert_status_button,
        button_press_check_period,
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
