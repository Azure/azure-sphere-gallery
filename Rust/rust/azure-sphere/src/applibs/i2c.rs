//! The Applibs i2c module contains functions and types that interact with an I2C (Inter-Integrated Circuit) interface.
//!
//! To access an I2C master interface, your application must identify it in the I2cMaster field of the application manifest.
//!
//! I2C functions reference some identifiers that are hardware dependent. Hardware dependent IDs are constants that are defined in the hardware definition files for a device.
use azure_sphere_sys::applibs::i2c;
use std::io::Error;

pub type InterfaceId = i2c::I2C_InterfaceId;
pub type DeviceAddress = i2c::I2C_DeviceAddress; // 7/10-bit i2c device addresses

#[derive(Debug)]
pub struct I2CMaster {
    fd: i32,
}

impl I2CMaster {
    pub fn new(interface_id: InterfaceId) -> Result<Self, std::io::Error> {
        let fd = unsafe { i2c::I2CMaster_Open(interface_id) };
        if fd == -1 {
            Err(Error::last_os_error())
        } else {
            Ok(Self { fd })
        }
    }

    /// Sets the I2C bus speed for operations on the I2C master interface.
    ///
    /// *Note*
    /// Not all speeds are supported on all Azure Sphere devices.
    pub fn set_bus_speed(&self, speed_in_hz: u32) -> Result<(), std::io::Error> {
        let result = unsafe { i2c::I2CMaster_SetBusSpeed(self.fd, speed_in_hz) };
        if result == -1 {
            Err(Error::last_os_error())
        } else {
            Ok(())
        }
    }

    // Skipping this since we are not supporting POSIX read/write
    // pub fn set_default_target_address()

    /// Sets the timeout for operations on an I2C master interface.
    pub fn set_timeout(&self, tiemout_in_msec: u32) -> Result<(), std::io::Error> {
        let result = unsafe { i2c::I2CMaster_SetTimeout(self.fd, tiemout_in_msec) };
        if result == -1 {
            Err(Error::last_os_error())
        } else {
            Ok(())
        }
    }

    /// Performs a read operation on an I2C master interface. This function provides the same functionality as the POSIX read(2) function except it specifies the address of the subordinate I2C device that is the target of the operation.
    pub fn read(
        &self,
        device_address: DeviceAddress,
        buffer: &mut [u8],
    ) -> Result<isize, std::io::Error> {
        let read_bytes_size = unsafe {
            i2c::I2CMaster_Read(self.fd, device_address, buffer.as_ptr() as _, buffer.len())
        };

        if read_bytes_size == -1 {
            Err(Error::last_os_error())
        } else {
            Ok(read_bytes_size)
        }
    }

    /// Performs a write operation on an I2C master interface. This function provides the same functionality as the POSIX write() function, except it specifies the address of the subordinate I2C device that is the target of the operation.
    pub fn write(
        &self,
        device_address: DeviceAddress,
        buffer: &[u8],
    ) -> Result<isize, std::io::Error> {
        let bytes_written = unsafe {
            i2c::I2CMaster_Write(self.fd, device_address, buffer.as_ptr() as _, buffer.len())
        };

        if bytes_written == -1 {
            Err(Error::last_os_error())
        } else {
            Ok(bytes_written)
        }
    }

    /// Performs a combined write-then-read operation on an I2C master interface. The operation appears as a single bus transaction with the following steps:
    /// 1. start condition
    /// 1. write
    /// 1. repeated start condition
    /// 1. read
    /// 1. stop condition
    pub fn write_then_read(
        &self,
        device_address: DeviceAddress,
        write_buffer: &[u8],
        read_buffer: &mut [u8],
    ) -> Result<isize, std::io::Error> {
        let total_bytes = unsafe {
            i2c::I2CMaster_WriteThenRead(
                self.fd,
                device_address,
                write_buffer.as_ptr() as _,
                write_buffer.len(),
                read_buffer.as_ptr() as _,
                read_buffer.len(),
            )
        };

        if total_bytes == -1 {
            Err(Error::last_os_error())
        } else {
            Ok(total_bytes)
        }
    }

    /// Sets the address of the subordinate device that is targeted by calls to read(2) and write(2) POSIX functions on the I2C master interface.
    ///
    /// *Note*
    /// This is not required when using [`I2CMaster::read`], [`I2CMaster::write`], or [`I2CMaster::write_then_read`], and has no impact on the address parameter of those functions.
    pub fn set_default_target_address(
        &self,
        device_address: DeviceAddress,
    ) -> Result<(), std::io::Error> {
        let result = unsafe { i2c::I2CMaster_SetDefaultTargetAddress(self.fd, device_address) };
        if result == -1 {
            Err(Error::last_os_error())
        } else {
            Ok(())
        }
    }
}

impl Drop for I2CMaster {
    fn drop(&mut self) {
        let _ = unsafe { libc::close(self.fd) };
    }
}
