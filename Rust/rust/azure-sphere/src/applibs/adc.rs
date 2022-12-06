//! Analog-to-digital (ADC) Controller
//!
//! Azure Sphere supports analog to digital conversion. An ADC converts an
//! analog input to a corresponding digital value. The number of input
//! channels and the resolution (as number of ADC output bits) are device
//! dependent.
//!
//! The MT3620 contains a 12-bit ADC with 8 input channels. The ADC compares
//! an input voltage to a reference voltage and produces a value between 0
//! and 4095 as its output. The ADC input channels and the GPIO pins GPIO41
//! through GPIO48 map to the same pins on the MT3260. However, if your
//! application uses the ADC then all 8 pins are allocated for use as ADC
//! inputs. None of them can be used for GPIO.
//!
//! # Examples
//!
//! ```
//! use azure_sphere::AdcController::{AdcController, AdcControllerId};
//! use std::io;
//! use nullable_result::NullableResult;
//!
//! fn get_adc_sample(controller_id: AdcControllerId, channel_u32) -> Result<u32, std::io::Error> {
//!     let adc = AdcController::new(controller_id)?;
//!     adc.set_reference_voltage(2.5)?;
//!     let sample_bit_count = adc.sample_bit_count(channel)?;
//!     let sample_value = adc.poll(channel)?;
//!     match value {
//!         NullableResult::Err(e) => Err(e)
//!         NullableResult::Null => Ok(0.0)
//!         NullableResult::Ok(voltage) => {
//!             voltage = ((voltage as f32) * SAMPLE_MAX_VOLTAGE)
//!                 / ((1 << sample_bit_count) - 1) as f32;
//!             Ok(voltage)
//!         }
//!     }
//!
//! }
//!
use azure_sphere_sys::applibs::static_inline_helpers;
use nullable_result::NullableResult;
use std::io::Error;

/// The ID of an ADC controller. This ID is a zero-based index.
pub type AdcControllerId = u32;

/// The ID of an ADC channel.
///
/// ADCs often have multiple channels on a single chip. A channel corresponds
/// to a single pin or input on the device. The range of allowed values for a
/// channel ID is device-dependent, and is typically a zero-based index.
type AdcChannelId = u32;

#[derive(Debug)]
pub struct AdcController {
    fd: libc::c_int,
    /// This ensures that the ADC Controller cannot be used across threads
    _threading: core::marker::PhantomData<*const ()>,
}

impl AdcController {
    /// Creates a new [`AdcController`] instance for a given [`AdcControllerId`]
    pub fn new(controller_id: AdcControllerId) -> Result<Self, std::io::Error> {
        let fd = unsafe { static_inline_helpers::ADC_Open_inline(controller_id) };
        if fd == -1 {
            Err(Error::last_os_error())
        } else {
            Ok(Self {
                fd,
                _threading: Default::default(),
            })
        }
    }

    /// Get the bit depth of the ADC.
    ///
    /// An example return value is 12, which indicates that the ADC controller
    /// can supply 12 bits of data that range from 0 to 4095.
    pub fn sample_bit_count(&self, channel_id: AdcChannelId) -> Result<i32, std::io::Error> {
        let res =
            unsafe { static_inline_helpers::ADC_GetSampleBitCount_inline(self.fd, channel_id) };
        if res == -1 {
            Err(Error::last_os_error())
        } else {
            Ok(res)
        }
    }

    /// Sets the reference voltage for the ADC.
    pub fn set_reference_voltage(
        &self,
        channel_id: AdcChannelId,
        reference_voltage: f32,
    ) -> Result<(), std::io::Error> {
        let res = unsafe {
            static_inline_helpers::ADC_SetReferenceVoltage_inline(
                self.fd,
                channel_id,
                reference_voltage,
            )
        };
        if res == -1 {
            Err(Error::last_os_error())
        } else {
            Ok(())
        }
    }

    /// Gets sample data for an ADC channel.
    pub fn poll(&self, channel_id: AdcChannelId) -> NullableResult<u32, std::io::Error> {
        let mut sample_value = 0;
        let res = unsafe {
            static_inline_helpers::ADC_Poll_inline(self.fd, channel_id, &mut sample_value)
        };
        if res == -1 {
            let e = Error::last_os_error();
            match e.kind() {
                std::io::ErrorKind::TimedOut => NullableResult::Null,
                _ => NullableResult::Err(e),
            }
        } else {
            NullableResult::Ok(sample_value)
        }
    }
}

impl Drop for AdcController {
    fn drop(&mut self) {
        let _ = unsafe { libc::close(self.fd) };
    }
}
