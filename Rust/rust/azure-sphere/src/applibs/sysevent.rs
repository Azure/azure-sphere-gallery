//! The Applibs sysevent module contains functions and types for system event notifications. Applications can register for and unregister from update notifications. Apps can use these notifications to put themselves in a safe state before application shutdown, or can attempt to defer these events.
use azure_sphere_sys::applibs::sysevent;
use bitmask_enum::bitmask;
use std::io::Error;

/// An opaque struct that contains information about a system event.
/// Instead it must be accessed by calling the system event function that is specific to the event type, such as ['info_get_update_data'].
pub type SysEventInfo = sysevent::SysEvent_Info;

/// The type of update to apply.
#[repr(u32)]
#[derive(Debug, PartialEq, Eq, Clone, Copy)]
pub enum UpdateType {
    /// This enum was improperly initialized.
    Invalid = sysevent::SysEvent_UpdateType_Invalid,
    /// An application update that restarts the updated application but doesn't reboot the device.
    App = sysevent::SysEvent_UpdateType_App,
    /// An OS update that requires a device reboot.
    System = sysevent::SysEvent_UpdateType_System,
}

/// Flags for system event types.
#[bitmask(u32)]
pub enum SysEvent {
    /// No event given.
    None = sysevent::SysEvent_Events_None,
    /// An OS or application update has been downloaded and is ready to install.
    UpdateReadyForInstall = sysevent::SysEvent_Events_UpdateReadyForInstall,
    /// An OS or application update has started. This event will be followed by the UpdateReadyForInstall event when the update is fully downloaded and ready for installation. If no update is available, NoUpdateAvailable will be sent instead of this event.
    UpdateStarted = sysevent::SysEvent_Events_UpdateStarted,
    /// No updates are available.
    NoUpdateAvailable = sysevent::SysEvent_Events_NoUpdateAvailable,
}

/// The status of a system event.
#[derive(Debug, PartialEq, Eq, Clone, Copy)]
pub enum Status {
    /// The status was improperly initialized.
    Invalid,
    /// A 10-second warning that an event will occur, with the opportunity to defer the event.
    Pending,
    /// A 10-second warning that an event will occur, without the opportunity to defer the event.
    Final,
    /// The previous pending event was deferred, and will occur at a later time.
    Deferred,
    /// The system event is complete. For software update events, this status is sent only for application updates because OS updates require a device reboot to complete.
    Complete,
}

/// A struct that contains information about update events.
#[derive(Debug, PartialEq, Eq, Clone, Copy)]
pub struct UpdateData {
    /// The maximum allowed deferral time, in minutes. The deferral time indicates how long an event listener can defer the event. This parameter is only defined if the status of the event is Pending.
    pub max_deferral_time_in_minutes: u32,
    /// The type of update event.
    pub update_type: UpdateType,
}

/// Retrieves application or OS update information.
/// The application manifest must include the SystemEventNotifications capability.
pub fn info_get_update_data(info: &SysEventInfo) -> Result<UpdateData, std::io::Error> {
    let mut update_info = sysevent::SysEvent_Info_UpdateData {
        max_deferral_time_in_minutes: 0,
        update_type: sysevent::SysEvent_UpdateType_Invalid,
    };
    let ret = unsafe { sysevent::SysEvent_Info_GetUpdateData(info, &mut update_info) };
    if ret == -1 {
        Err(Error::last_os_error())
    } else {
        let ut = match update_info.update_type {
            sysevent::SysEvent_UpdateType_App => UpdateType::App,
            sysevent::SysEvent_UpdateType_System => UpdateType::System,
            _ => UpdateType::Invalid,
        };
        Ok(UpdateData {
            max_deferral_time_in_minutes: update_info.max_deferral_time_in_minutes,
            update_type: ut,
        })
    }
}

/// Attempts to defer an event for the specified duration. This function should only be called when the event status is Pending. If the event is not pending, the call fails.
/// The application manifest must include the SystemEventNotifications and SoftwareUpdateDeferral capabilities.
pub fn defer_event(
    event: SysEvent,
    requested_defer_time_in_minutes: u32,
) -> Result<(), std::io::Error> {
    let event = event.bits();
    let ret = unsafe { sysevent::SysEvent_DeferEvent(event, requested_defer_time_in_minutes) };
    if ret == -1 {
        Err(Error::last_os_error())
    } else {
        Ok(())
    }
}

/// Attempts to resume an event if it is deferred.
/// This function can only be called if [`defer_event`] was called successfully, otherwise the call will fail. [`resume_event`] is not required after a [`defer_event`] call. It should only be used if the update deferral is no longer needed.
/// The application manifest must include the SystemEventNotifications and SoftwareUpdateDeferral capabilities.
pub fn resume_event(event: SysEvent) -> Result<(), std::io::Error> {
    let event = event.bits();
    let ret = unsafe { sysevent::SysEvent_ResumeEvent(event) };
    if ret == -1 {
        Err(Error::last_os_error())
    } else {
        Ok(())
    }
}
