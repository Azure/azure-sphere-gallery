/* Copyright (c) Microsoft Corporation. All rights reserved.
Licensed under the MIT License. */

use crate::eventloop::IoCallback;
use azs::applibs::adc;
use azs::applibs::application;
use azs::applibs::applications;
use azs::applibs::deviceauth;
use azs::applibs::deviceauth_curl;
use azs::applibs::eventloop;
use azs::applibs::gpio;
use azs::applibs::i2c;
use azs::applibs::networking;
use azs::applibs::networking_curl;
use azs::applibs::powermanagement;
use azs::applibs::pwm::*;
use azs::applibs::rtc;
use azs::applibs::spi;
use azs::applibs::storage;
use azs::applibs::sysevent;
use azs::applibs::uart;
use azure_sphere as azs;
use azure_sphere::applibs::eventloop_timer_utilities;
use curl::easy::Easy;
use nullable_result::NullableResult;
use std::io::{stdout, Write};
use std::os::unix::prelude::FileExt;
use std::time::Duration;

struct BlinkingLight {
    pin: gpio::OutputPin,
    elt: eventloop_timer_utilities::EventLoopTimer,
}

impl BlinkingLight {
    pub fn new(pin: gpio::OutputPin, period: Duration) -> Result<Self, std::io::Error> {
        let elt = eventloop_timer_utilities::EventLoopTimer::new()?;
        elt.set_period(period)?;
        Ok(Self { pin, elt })
    }
}

impl IoCallback for BlinkingLight {
    fn event(&mut self, _events: eventloop::IoEvents) {
        self.elt.consume_event().unwrap();

        let r = self.pin.set_value(gpio::Value::High);
        azs::debug!("Turned the pin on... {:?}\n", r);
        let r = self.pin.set_value(gpio::Value::Low);
        azs::debug!("Turned the pin off... {:?}\n", r);
    }

    unsafe fn fd(&self) -> i32 {
        return self.elt.fd();
    }
}

pub fn main() -> ! {
    let os_version = applications::os_version();
    azs::debug!("OS Version: {:?}", os_version);

    let adc_controller = adc::AdcController::new(0).unwrap();

    let r = adc_controller.set_reference_voltage(0, 5.0);
    azs::debug!("Adc Channel 0 ref volt set to 5V... {:?}\n", r);

    let r = adc_controller.sample_bit_count(0);
    azs::debug!("Adc Channel 0 sample bit count is... {:?}\n", r);

    let r = adc_controller.poll(0);
    match r {
        NullableResult::Null => azs::debug!("Adc Channel 0 no sample available.\n"),
        NullableResult::Err(e) => azs::debug!("Adc Channel 0 failed with errno. Fail... {:?}\n", e),
        NullableResult::Ok(v) => azs::debug!("Adc Channel 0 sample output value... {:?}\n", v),
    };

    let mut read_buffer = [0; 8];
    let write_buffer = [42; 15];
    let device_address = 0;

    let i2c_master_or_err = i2c::I2CMaster::new(0);
    if i2c_master_or_err.is_err() {
        azs::debug!("I2C isn't available.\n");
    } else {
        let i2c_master = i2c_master_or_err.unwrap();
        let read_bytes = i2c_master.read(device_address, &mut read_buffer).unwrap();
        azs::debug!("I2C interface 0, dev addr 0, bytes read = {:?}", read_bytes);

        let _ = i2c_master.write_then_read(device_address, &write_buffer[0..8], &mut read_buffer);
        azs::debug!(
            "I2C interface 0, dev addr 0, read_buffer[0] = {:?}",
            read_buffer[0]
        );
    }

    let controller_id = 0; // TODO can we avoid hard-coded values and use MT3620_PWM_CONTROLLER0
    let channel_id = 0;
    let mut pwm_state = PwmState {
        period_nsec: 30000,
        duty_cycle_nsec: 15000,
        polarity: PwmPolarity::Normal,
        enabled: true,
    };

    let pwm_controller_or_err = PwmController::new(controller_id);
    if pwm_controller_or_err.is_err() {
    } else {
        let pwm_controller = pwm_controller_or_err.unwrap();
        pwm_controller.apply(channel_id, pwm_state).unwrap();
        std::thread::sleep(Duration::from_secs(1));

        pwm_state.enabled = false;
        pwm_controller.apply(channel_id, pwm_state).unwrap();
    }

    let result = rtc::clock_systohc();
    azs::debug!("Synchronized RTC with system time... {:?}\n", result);

    let r = storage::open_mutable_file();
    if r.is_err() {
        azs::debug!("No mutable storage... {:?}\n", r);
    } else {
        let f = r.unwrap();
        let mut buf = [0u8; 8];
        let num_bytes_read = f.read_at(&mut buf, 10);
        azs::debug!("Read from mutable storage... {:?}\n", num_bytes_read);
    }

    let result = application::is_device_auth_ready();
    azs::debug!("application::is_device_auth_ready()... {:?}\n", result);

    let result = applications::total_memory_usage();
    azs::debug!("Total memory usage... {:?} kb\n", result);
    let result = applications::user_mode_memory_usage();
    azs::debug!("Usermode memory usage... {:?} kb\n", result);
    let result = applications::peak_user_mode_memory_usage();
    azs::debug!("Peak usermode memory usage... {:?} kb\n", result);

    let result = spi::SPIMaster::new(0, 0x3a, spi::ChipSelectPolarity::ActiveLow);
    if result.is_err() {
        azs::debug!("SPIMaster new failed .. {:?} kb\n", result.unwrap_err());
    } else {
        azs::debug!("SPIMaster new succceeded..\n");
    }

    let is_ready = networking::is_networking_ready().unwrap();
    if is_ready {
        // Write the contents of rust-lang.org to stdout
        let mut easy = Easy::new();
        easy.verbose(true).unwrap();
        let r = networking_curl::set_default_proxy(&easy);
        azs::debug!("networking_curl::set_default_proxy... {:?} kb\n", r);
        let r = easy.url("http://example.com");
        if r.is_err() {
            azs::debug!("No network connectivity via curl... {:?}\n", r);
        } else {
            let curl_timeout = Duration::new(30, 0);
            easy.timeout(curl_timeout).unwrap();

            let r = easy.write_function(|data| {
                stdout().write_all(data).unwrap();
                Ok(data.len())
            });
            azs::debug!("curl write_function... {:?}\n", r);

            let r = easy.debug_function(|_info_type, data| {
                stdout().write_all(data).unwrap();
            });
            azs::debug!("curl debug_function... {:?}\n", r);
            let r = easy.perform();
            azs::debug!("curl easy.perform... {:?}\n", r);

            // Confirm that the types match, even though we don't test TLS mutual auth
            let r = easy.ssl_ctx_function(deviceauth_curl::curl_ssl_function);
            azs::debug!("curl ssl_ctx_function... {:?}\n", r);
        }
    } else {
        azs::debug!("networking demo skipped as the network isn't ready\n");
    }

    let interfaces = networking::interfaces();
    azs::debug!("Network interfaces: {:?}\n", interfaces);

    let r =
        powermanagement::set_system_power_profile(powermanagement::PowerProfile::HighPerformance);
    azs::debug!("set_system_power_profile to HighPerformance: {:?}\n", r);

    let r = sysevent::resume_event(sysevent::SysEvent::UpdateReadyForInstall);
    azs::debug!("SysEvent_ResumeEvent for UpdateReadyForInstall: {:?}\n", r);

    let uart_config = uart::UARTConfig {
        baud_rate: 115200,
        ..Default::default()
    };
    let u = uart::open(0, uart_config);
    if u.is_err() {
        azs::debug!("uart::open() failed .. {:?} kb\n", u.unwrap_err());
    }

    let pin = gpio::OutputPin::new(8, gpio::OutputMode::PushPull, gpio::Value::Low).unwrap();
    let period = Duration::new(1, 0);
    let mut blinking_light = BlinkingLight::new(pin, period).unwrap();

    let mut el = eventloop::EventLoop::new().unwrap();
    el.register_io(eventloop::IoEvents::Input, &mut blinking_light)
        .unwrap();

    let er = el.register_sysevent(
        sysevent::SysEvent::UpdateReadyForInstall,
        |event: sysevent::SysEvent,
         status: sysevent::Status,
         _info: *const sysevent::SysEventInfo| {
            azs::debug!(
                "SysEvent triggered... event {:?} status {:?}\n",
                event,
                status
            );
        },
    );
    if er.is_err() {
        azs::debug!(
            "eventloop::register_sysevent failed {:?}\n",
            er.unwrap_err()
        );
    }

    let cert_path = deviceauth::certificate_path();
    if cert_path.is_err() {
        azs::debug!(
            "deviceauth::certificate_path() failed .. {:?} kb\n",
            cert_path.unwrap_err()
        );
    } else {
        azs::debug!("deviceauth::certificate_path:  {:?}\n", cert_path.unwrap());
    }

    loop {
        let result = el.run(-1, true);
        if result.is_err() {
            azs::debug!("eventloop::run() failed {:?}\n", result.unwrap_err());
        }
    }
}
