//! The Applibs Gpio module contains functions and types that interact with GPIOs.
use azure_sphere_sys::applibs::gpio;
use std::io::Error;

/// The output mode of a GPIO
///
/// For more info [see here](https://docs.microsoft.com/en-us/azure-sphere/reference/applibs-reference/applibs-gpio/enum-gpio-outputmode)
#[repr(u8)]
#[derive(Debug, PartialEq, Eq, Clone, Copy)]
pub enum OutputMode {
    /// Configures the GPIO output pin such that it sinks current when driven low and sources current when driven high.
    PushPull = 0,
    /// Configures the GPIO output pin such that it sinks current when driven low; it cannot source current.
    OpenDrain,
    /// Configures the GPIO output pin such that it sources current when driven high; it cannot sink current.
    OpenSource,
}

/// The possible read/write values for a GPIO.
#[repr(u8)]
#[derive(Debug, PartialEq, Eq, Clone, Copy)]
pub enum Value {
    /// Low, or logic 0
    Low = 0,
    /// High, or logic 1
    High,
}

/// A GPIO pin configured for output.
#[derive(Debug)]
pub struct OutputPin {
    fd: i32,
    // The underlying GPIO pins are threadsafe, so no need for this: _threading: core::marker::PhantomData<*const ()>,
}

impl OutputPin {
    /// Create a new `OutputPin` with a given mode and initial value
    ///
    /// Either returns an `OutputPin` or an error
    pub fn new(id: u32, mode: OutputMode, initial_value: Value) -> Result<Self, std::io::Error> {
        let fd = unsafe { gpio::GPIO_OpenAsOutput(id as i32, mode as u8, initial_value as u8) };
        if fd == -1 {
            Err(Error::last_os_error())
        } else {
            Ok(Self { fd })
        }
    }

    /// Set the value of the GPIO pin
    ///
    /// Returns Ok or an error with the errno inside. You can find more
    /// information [here](https://docs.microsoft.com/en-us/azure-sphere/reference/applibs-reference/applibs-gpio/function-gpio-setvalue).
    pub fn set_value(&self, value: Value) -> Result<(), std::io::Error> {
        let result = unsafe { gpio::GPIO_SetValue(self.fd, value as u8) };
        if result == -1 {
            Err(Error::last_os_error())
        } else {
            Ok(())
        }
    }

    /// Get the value of the GPIO pin
    ///
    /// Returns Ok with the value or an error with the errno inside. You can find more
    /// information [here](https://docs.microsoft.com/en-us/azure-sphere/reference/applibs-reference/applibs-gpio/function-gpio-getvalue).
    pub fn value(&self) -> Result<Value, std::io::Error> {
        value(self.fd)
    }
}

impl Drop for OutputPin {
    fn drop(&mut self) {
        let _ = unsafe { libc::close(self.fd) };
    }
}

/// A GPIO pin configured for input.
#[derive(Debug)]
pub struct InputPin {
    fd: i32,
    // The underlying GPIO pins are threadsafe, so no need for this: _threading: core::marker::PhantomData<*const ()>,
}

impl InputPin {
    /// Create a new `InputPin` with a given mode and initial value
    ///
    /// Either returns an `InputPin` or an error
    pub fn new(id: u32) -> Result<Self, std::io::Error> {
        let fd = unsafe { gpio::GPIO_OpenAsInput(id as i32) };
        if fd == -1 {
            Err(Error::last_os_error())
        } else {
            Ok(Self { fd })
        }
    }

    /// Get the value of the GPIO pin
    ///
    /// Returns Ok with the value or an error with the errno inside. You can find more
    /// information [here](https://docs.microsoft.com/en-us/azure-sphere/reference/applibs-reference/applibs-gpio/function-gpio-getvalue).
    pub fn value(&self) -> Result<Value, std::io::Error> {
        value(self.fd)
    }
}

impl Drop for InputPin {
    fn drop(&mut self) {
        let _ = unsafe { libc::close(self.fd) };
    }
}

// Internal function for sharing logic around get a value from a pin
fn value(fd: libc::c_int) -> Result<Value, std::io::Error> {
    let mut value = Value::Low;
    let result = unsafe { gpio::GPIO_GetValue(fd, &mut value as *mut _ as _) };
    if result == -1 {
        Err(Error::last_os_error())
    } else {
        Ok(value)
    }
}
