/* Copyright (c) Microsoft Corporation. All rights reserved.
Licensed under the MIT License. */

#![allow(non_upper_case_globals)]
#![allow(non_camel_case_types)]
#![allow(non_snake_case)]
#![allow(dead_code)]
#![no_std]

#[link(name = "applibs", kind = "dylib")]
#[link(name = "tlsutils", kind = "dylib")]
#[link(name = "c", kind = "dylib")]
extern "C" {}

macro_rules! sys_mod {
    ($name:ident) => {
        pub mod $name {
            include!(concat!(
                env!("OUT_DIR"),
                concat!("/", stringify!($name), ".rs")
            ));
        }
    };
}

pub mod applibs {
    sys_mod!(adc);
    sys_mod!(application);
    sys_mod!(applications);
    sys_mod!(certstore);
    sys_mod!(deviceauth);
    sys_mod!(deviceauth_curl);
    sys_mod!(eventloop);
    sys_mod!(i2c);
    sys_mod!(networking);
    sys_mod!(powermanagement);
    sys_mod!(pwm);
    sys_mod!(rtc);
    sys_mod!(spi);
    sys_mod!(storage);
    sys_mod!(sysevent);
    sys_mod!(log);
    sys_mod!(gpio);
    sys_mod!(wificonfig);
    sys_mod!(static_inline_helpers);
}
