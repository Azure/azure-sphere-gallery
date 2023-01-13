//! The Applibs pwm module contains functions that interact with pulse-width modulators (PWM). PWM functions are thread-safe.
use azure_sphere_sys::applibs::static_inline_helpers;
use std::io::Error;

/// The polarity of a PWM channel
///
/// For more info [see here](https://docs.microsoft.com/en-us/azure-sphere/reference/applibs-reference/applibs-pwm/pwm-overview)
#[repr(u8)]
#[derive(Copy, Clone, Debug)]
pub enum PwmPolarity {
    Normal,
    Inversed,
}

#[derive(Copy, Clone, Debug)]
pub struct PwmState {
    pub period_nsec: libc::c_uint,
    pub duty_cycle_nsec: libc::c_uint,
    pub polarity: PwmPolarity,
    pub enabled: bool,
}

type PwmControllerId = u32;
type PwmChannelId = u32;

#[derive(Debug, Clone)]
pub struct PwmController {
    fd: libc::c_int,
}

impl PwmController {
    /// Opens a PWM controller.
    /// To access a PWM controller, your application must identify it in the Pwm field of the application manifest.
    pub fn new(controller_id: PwmControllerId) -> Result<Self, std::io::Error> {
        let fd = unsafe { static_inline_helpers::PWM_Open_inline(controller_id) };
        if fd == -1 {
            Err(Error::last_os_error())
        } else {
            Ok(Self { fd })
        }
    }

    /// Sets the state of a PWM channel for a PWM controller.
    /// To access a PWM controller, your application must identify it in the Pwm field of the application manifest.
    pub fn apply(
        &self,
        channel_id: PwmChannelId,
        pwm_state: PwmState,
    ) -> Result<(), std::io::Error> {
        let pwm_state_sys = static_inline_helpers::PwmState {
            period_nsec: pwm_state.period_nsec,
            dutyCycle_nsec: pwm_state.duty_cycle_nsec,
            polarity: match pwm_state.polarity {
                PwmPolarity::Normal => 0,
                PwmPolarity::Inversed => 1,
            },
            enabled: pwm_state.enabled,
        };

        let result =
            unsafe { static_inline_helpers::PWM_Apply_inline(self.fd, channel_id, &pwm_state_sys) };
        if result == -1 {
            Err(Error::last_os_error())
        } else {
            Ok(())
        }
    }
}

impl Drop for PwmController {
    fn drop(&mut self) {
        let _ = unsafe { libc::close(self.fd) };
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn open_works() {
        // Need mt3620 HW to work
        let controller_id = 0;
        let pwm_controller = PwmController::new(controller_id);
        assert!(pwm_controller.is_ok());
    }

    #[test]
    fn apply_works() {
        let controller_id = 0;
        let channel_id = 0;
        let mut pwm_state = PwmState {
            period_nsec: 30000,
            duty_cycle_nsec: 15000,
            polarity: PwmPolarity::Normal,
            enabled: true,
        };

        let pwm_controller = PwmController::new(controller_id).unwrap();
        assert!(pwm_controller.apply(channel_id, pwm_state).is_ok());

        pwm_state.enabled = false;
        assert!(pwm_controller.apply(channel_id, pwm_state).is_ok());
    }
}
