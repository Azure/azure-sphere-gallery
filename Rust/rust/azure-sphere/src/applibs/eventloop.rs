//! The eventloop module  contains functions and types used to monitor and dispatch events.
use crate::applibs::sysevent;
use azure_sphere_sys::applibs::eventloop;
use azure_sphere_sys::applibs::sysevent as sysevent_native;
use bitmask_enum::bitmask;
use std::io::Error;

/// Kind of I/O event to be monitored
#[bitmask(u32)]
pub enum IoEvents {
    Input = eventloop::EventLoop_Input,
    Output = eventloop::EventLoop_Output,
    Error = eventloop::EventLoop_Error,
}

/// I/O Callback
pub trait IoCallback {
    fn event(&mut self, events: IoEvents);
    unsafe fn fd(&self) -> i32;
}

/// I/O event registration
#[derive(Debug)]
pub struct IoEventRegistration {
    registration: *mut eventloop::EventRegistration,
}

/// System event registration
#[derive(Debug)]
pub struct SysEventRegistration {
    registration: *mut sysevent_native::EventRegistration,
}

/// An object that monitors event sources and dispatches their events to handlers.
///
/// To dispatch the events that need processing, the application must call [`EventLoop::run`]. The event handlers are called in the same thread where EventLoop_Run is called.
#[derive(Debug)]
pub struct EventLoop {
    el: *mut eventloop::EventLoop,
    io_observers: Vec<*mut eventloop::EventRegistration>,
    sys_observers: Vec<*mut sysevent_native::EventRegistration>,

    /// This ensures that the Event Loop cannot be used across threads
    _threading: core::marker::PhantomData<*const ()>,
}

// bindgen generates 'context' as a void*.  Manually wrap it with an IoEventRegistration pointer here.
type EventLoopIoCallback = unsafe extern "C" fn(
    el: *mut eventloop::EventLoop,
    fd: libc::c_int,
    events: eventloop::EventLoop_IoEvents,
    context: *mut libc::c_void,
);

extern "C" {
    #[allow(improper_ctypes)]
    // EventLoopIoCallback is a function pointer whose types aren't C, but that's OK
    // bindgen generates 'context' as a void*.  Manually wrap it with an IoEventRegistration pointer here.
    fn EventLoop_RegisterIo(
        el: *mut eventloop::EventLoop,
        fd: libc::c_int,
        eventBitmask: eventloop::EventLoop_IoEvents,
        callback: EventLoopIoCallback,
        context: *mut libc::c_void, // *mut dyn IoCallback,
    ) -> *mut eventloop::EventRegistration;
}

impl EventLoop {
    pub fn new() -> Result<Self, std::io::Error> {
        unsafe {
            let el = eventloop::EventLoop_Create();
            if el.is_null() {
                Err(Error::last_os_error())
            } else {
                Ok(Self {
                    el,
                    io_observers: Vec::new(),
                    sys_observers: Vec::new(),
                    _threading: Default::default(),
                })
            }
        }
    }

    /// Runs an EventLoop and dispatches pending events in the caller's thread of execution.
    pub fn run(
        &self,
        duration_in_milliseconds: i32,
        process_one_event: bool,
    ) -> Result<bool, std::io::Error> {
        let ret = unsafe {
            eventloop::EventLoop_Run(self.el, duration_in_milliseconds, process_one_event)
        };
        match ret {
            eventloop::EventLoop_Run_FinishedEmpty => Ok(true),
            eventloop::EventLoop_Run_Finished => Ok(false),
            _ => Err(Error::last_os_error()),
        }
    }

    /// Stops the EventLoop from running and causes EventLoop_Run to return control to its caller.
    pub fn stop(&self) -> Result<(), std::io::Error> {
        let r = unsafe { eventloop::EventLoop_Stop(self.el) };
        if r == 0 {
            Ok(())
        } else {
            Err(Error::last_os_error())
        }
    }

    unsafe extern "C" fn event_loop_callback_wrapper(
        _el: *mut eventloop::EventLoop,
        _fd: libc::c_int,
        events: eventloop::EventLoop_IoEvents,
        context: *mut libc::c_void,
    ) {
        let context = (context as *mut Box<&mut dyn IoCallback>).as_mut().unwrap();
        context.event(IoEvents::from(events));
    }

    /// Registers an I/O event with an EventLoop.
    pub fn register_io(
        &mut self,
        event_bitmask: IoEvents,
        observer: &mut dyn IoCallback,
    ) -> Result<IoEventRegistration, std::io::Error> {
        unsafe {
            let observer_fd = (*observer).fd();
            let context = Box::into_raw(Box::new(Box::new(observer) as Box<&mut dyn IoCallback>));

            let er = EventLoop_RegisterIo(
                self.el,
                observer_fd,
                event_bitmask.into(),
                Self::event_loop_callback_wrapper,
                context as _,
            );
            if er.is_null() {
                Err(Error::last_os_error())
            } else {
                self.io_observers.push(er);
                let elr = IoEventRegistration { registration: er };
                Ok(elr)
            }
        }
    }

    unsafe extern "C" fn callback_wrapper<F>(
        event: sysevent_native::SysEvent_Events,
        state: sysevent_native::SysEvent_Status,
        info: *const sysevent_native::SysEvent_Info,
        context: *mut libc::c_void,
    ) where
        F: Fn(sysevent::SysEvent, sysevent::Status, *const sysevent::SysEventInfo),
    {
        let callback = &mut *(context as *mut F);
        let ev1 = if (event & sysevent_native::SysEvent_Events_UpdateReadyForInstall) != 0 {
            sysevent::SysEvent::UpdateReadyForInstall
        } else {
            sysevent::SysEvent::None
        };
        let ev2 = if (event & sysevent_native::SysEvent_Events_UpdateStarted) != 0 {
            sysevent::SysEvent::UpdateStarted
        } else {
            sysevent::SysEvent::None
        };
        let ev3 = if (event & sysevent_native::SysEvent_Events_NoUpdateAvailable) != 0 {
            sysevent::SysEvent::NoUpdateAvailable
        } else {
            sysevent::SysEvent::None
        };
        let ev = ev1.or(ev2).or(ev3);

        let state = match state {
            sysevent_native::SysEvent_Status_Pending => sysevent::Status::Pending,
            sysevent_native::SysEvent_Status_Final => sysevent::Status::Final,
            sysevent_native::SysEvent_Status_Deferred => sysevent::Status::Deferred,
            sysevent_native::SysEvent_Status_Complete => sysevent::Status::Complete,
            _ => sysevent::Status::Invalid,
        };

        (*callback)(ev, state, info);
    }

    /// Registers a system event with an EventLoop.
    pub fn register_sysevent<F>(
        &mut self,
        event_bitmask: sysevent::SysEvent,
        handler: F,
    ) -> Result<SysEventRegistration, std::io::Error>
    where
        F: Fn(sysevent::SysEvent, sysevent::Status, *const sysevent::SysEventInfo),
    {
        let handler = &handler as *const _ as *mut libc::c_void;

        let er = unsafe {
            sysevent_native::SysEvent_RegisterForEventNotifications(
                self.el as *mut azure_sphere_sys::applibs::sysevent::EventLoop,
                event_bitmask.bits(),
                Some(Self::callback_wrapper::<F>),
                handler,
            )
        };
        if er == std::ptr::null_mut() {
            Err(Error::last_os_error())
        } else {
            self.sys_observers.push(er);
            let elr = SysEventRegistration { registration: er };
            Ok(elr)
        }
    }

    /// Modify a registered I/O event
    pub fn modify_io_events(
        &self,
        reg: &IoEventRegistration,
        event_bit_mask: IoEvents,
    ) -> Result<(), std::io::Error> {
        let res = unsafe {
            eventloop::EventLoop_ModifyIoEvents(self.el, reg.registration, event_bit_mask.bits())
        };
        if res == 0 {
            Ok(())
        } else {
            Err(Error::last_os_error())
        }
    }

    /// Unregisters an I/O event from an EventLoop object.
    pub fn unregister_io(&mut self, reg: IoEventRegistration) -> Result<(), std::io::Error> {
        let res = unsafe { eventloop::EventLoop_UnregisterIo(self.el, reg.registration) };
        if res == 0 {
            self.io_observers.retain(|&x| x != reg.registration);
            Ok(())
        } else {
            Err(Error::last_os_error())
        }
    }

    /// Unregisters a system event from an EventLoop object.
    pub fn unregister_sysevent(&mut self, reg: SysEventRegistration) -> Result<(), std::io::Error> {
        let res =
            unsafe { sysevent_native::SysEvent_UnregisterForEventNotifications(reg.registration) };
        if res == 0 {
            self.sys_observers.retain(|&x| x != reg.registration);
            Ok(())
        } else {
            Err(Error::last_os_error())
        }
    }

    /// Gets a file descriptor for an EventLoop.
    ///
    /// The file descriptor is signaled for input when the EventLoop has work ready to process. The application can wait or poll the file descriptor to determine when to process the EventLoop with EventLoop_Run.
    pub unsafe fn get_wait_descriptor(&self) -> i32 {
        return eventloop::EventLoop_GetWaitDescriptor(self.el);
    }
}

impl Drop for EventLoop {
    fn drop(&mut self) {
        let _ = unsafe { eventloop::EventLoop_Close(self.el) };
    }
}
