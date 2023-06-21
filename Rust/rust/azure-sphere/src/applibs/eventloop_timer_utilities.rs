//! Periodic timers, on top of EventLoop
use crate::applibs::eventloop;
use azure_sphere_sys::applibs::static_inline_helpers;
use libc::timerfd_create;
use std::fmt;
use std::io::Error;

/// A periodic timer.
/// Once configured, it must be registered with `crate::eventloop::EventLoop::register_io()` in order to become active.
/// #[]
#[derive(Debug)]
pub struct EventLoopTimer {
    fd: i32,

    /// This ensures that the EventLoopTimer cannot be used across threads
    _threading: core::marker::PhantomData<*const ()>,
}

impl EventLoopTimer {
    pub fn new() -> Result<Self, std::io::Error> {
        let fd = unsafe { timerfd_create(libc::CLOCK_MONOTONIC, libc::TFD_NONBLOCK) };
        if fd == -1 {
            Err(Error::last_os_error())
        } else {
            Ok(Self {
                fd,
                _threading: Default::default(),
            })
        }
    }

    /// Get the underlying OS timer file descriptor
    pub unsafe fn fd(&self) -> i32 {
        return self.fd;
    }

    /// Set the timer period and initial delay.  Both are optional, so a timer can be one-shot,
    /// or repeating but with no initial delay.
    fn set_timer_period(
        timerfd: i32,
        initial: Option<std::time::Duration>,
        repeat: Option<std::time::Duration>,
    ) -> Result<(), std::io::Error> {
        let initial = match initial {
            // BUGBUG: libc::time_t is 32-bit, but Azure Sphere uses 64-bit.  So use the static_inline_helpers versions instead.
            Some(t) => static_inline_helpers::timespec {
                tv_sec: t.as_secs() as i64,
                tv_nsec: t.subsec_nanos() as libc::c_long,
                _bitfield_align_1: [],
                _bitfield_1: static_inline_helpers::timespec::new_bitfield_1(),
            },
            None => static_inline_helpers::timespec {
                tv_sec: 0,
                tv_nsec: 0,
                _bitfield_align_1: [],
                _bitfield_1: static_inline_helpers::timespec::new_bitfield_1(),
            },
        };
        let repeat = match repeat {
            Some(t) => static_inline_helpers::timespec {
                tv_sec: t.as_secs() as i64,
                tv_nsec: t.subsec_nanos() as libc::c_long,
                _bitfield_align_1: [],
                _bitfield_1: static_inline_helpers::timespec::new_bitfield_1(),
            },
            None => static_inline_helpers::timespec {
                tv_sec: 0,
                tv_nsec: 0,
                _bitfield_align_1: [],
                _bitfield_1: static_inline_helpers::timespec::new_bitfield_1(),
            },
        };

        let new_value = static_inline_helpers::itimerspec {
            it_value: initial,
            it_interval: repeat,
        };
        let ret = unsafe {
            static_inline_helpers::timerfd_settime_inline(
                timerfd,
                0,
                &new_value,
                std::ptr::null_mut(),
            )
        };
        if ret == -1 {
            Err(Error::last_os_error())
        } else {
            Ok(())
        }
    }

    /// Consume the timer event after it has fired
    pub fn consume_event(&self) -> Result<(), std::io::Error> {
        let mut timer_data: u64 = 0;
        let ret = unsafe { libc::read(self.fd, &mut timer_data as *mut _ as *mut libc::c_void, 8) };
        if ret == -1 {
            Err(Error::last_os_error())
        } else {
            Ok(())
        }
    }

    /// Modify the timer period
    pub fn set_period(&self, period: std::time::Duration) -> Result<(), std::io::Error> {
        let period = Some(period);
        Self::set_timer_period(self.fd, period, period)
    }

    /// Make the timer one-shot
    pub fn set_one_shot(&self, delay: std::time::Duration) -> Result<(), std::io::Error> {
        let delay = Some(delay);
        Self::set_timer_period(self.fd, delay, None)
    }

    // Disable the timer
    pub fn disarm(&self) -> Result<(), std::io::Error> {
        Self::set_timer_period(self.fd, None, None)
    }
}

impl Drop for EventLoopTimer {
    fn drop(&mut self) {
        let _ = unsafe { libc::close(self.fd) };
    }
}

/// A timer with callbacks via a mutable closure
pub struct EventLoopTimerWithCallback {
    pub elt: EventLoopTimer,
    callback: Box<dyn FnMut()>,
}

impl eventloop::IoCallback for EventLoopTimerWithCallback {
    fn event(&mut self, _events: eventloop::IoEvents) {
        self.elt.consume_event().unwrap();
        (self.callback)();
    }

    unsafe fn fd(&self) -> i32 {
        self.elt.fd()
    }
}

impl EventLoopTimerWithCallback {
    pub fn new(callback: Box<dyn FnMut()>) -> Result<Self, std::io::Error> {
        let elt = EventLoopTimer::new()?;
        Ok(Self { elt, callback })
    }
}

impl fmt::Debug for EventLoopTimerWithCallback {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.debug_struct("Point")
            .field("elt", &self.elt)
            .field("callback", &"<mutable closure>")
            .finish()
    }
}
