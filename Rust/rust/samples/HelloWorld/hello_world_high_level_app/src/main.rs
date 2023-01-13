/* Copyright (c) Microsoft Corporation. All rights reserved.
Licensed under the MIT License. */

// This minimal Azure Sphere app repeatedly toggles GPIO 8, which is the red channel of RGB
// LED 1 on the MT3620 RDB. Use this app to test that device and SDK installation succeeded
// that you can build, deploy, and debug a Rust app.
//
// It uses the API for the following Azure Sphere application libraries:
// - gpio (digital input for button)
// - azs::debug! (displays messages in the Device Output window during debugging)

use azs::applibs::gpio;
use azure_sphere as azs;
use std::{thread, time};

// A main(), except that it returns a Result<T,E>, making it easy to invoke functions using the '?' operator.
fn actual_main() -> Result<(), std::io::Error> {
    azs::debug!("Starting Rust Hello World application...\n");

    let pin = gpio::OutputPin::new(
        hardware::sample_appliance::SAMPLE_LED,
        gpio::OutputMode::PushPull,
        gpio::Value::High,
    )?;

    let sleep_time = time::Duration::from_millis(1000);
    loop {
        pin.set_value(gpio::Value::Low)?;
        thread::sleep(sleep_time);
        pin.set_value(gpio::Value::High)?;
        thread::sleep(sleep_time);
    }
}

pub fn main() -> ! {
    let result = actual_main();
    if result.is_err() {
        azs::debug!("Failed with {:?}\n", result.err());
        std::process::exit(1);
    }

    azs::debug!(
        "Application exiting - {:?}\n",
        azs::applibs::applications::total_memory_usage()
    );
    std::process::exit(0);
}
