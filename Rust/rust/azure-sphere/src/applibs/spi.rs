//! The Applibs SPI module contains functions and types that access a Serial Peripheral Interface (SPI) on a device.
use azure_sphere_sys::applibs::spi;
use azure_sphere_sys::applibs::static_inline_helpers;
use std::cmp::min;
use std::io::Error;

/// The ID of an SPI interface instance.
pub type InterfaceId = spi::SPI_InterfaceId;

/// A SPI chip select ID.
pub type ChipSelectId = spi::SPI_ChipSelectId;

/// Chip select polarity
#[repr(u32)]
#[derive(Debug, PartialEq, Eq, Clone, Copy)]
pub enum ChipSelectPolarity {
    ActiveLow = static_inline_helpers::SPI_ChipSelectPolarity_SPI_ChipSelectPolarity_ActiveLow,
    ActiveHigh = static_inline_helpers::SPI_ChipSelectPolarity_SPI_ChipSelectPolarity_ActiveHigh,
}

/// Describes data to read and write within a SPI Master transfer
pub struct SPIMasterTransfer<'a, 'b> {
    /// The transfer flags for the operation
    pub flags: static_inline_helpers::SPI_TransferFlags,
    /// The data for write operations. This value is ignored for half-duplex reads.
    pub write_data: &'a [u8],
    /// The buffer for read operations. This value is ignored for half-duplex writes.
    pub read_data: &'b mut [u8],
}

#[derive(Debug)]
pub struct SPIMaster {
    fd: i32,
}

/// Access the SPI controller
/// To access individual SPI interfaces, your application must identify them in the SpiMaster field of the application manifest.
impl SPIMaster {
    pub fn new(
        interface_id: InterfaceId,
        chip_select_id: ChipSelectId,
        polarity: ChipSelectPolarity,
    ) -> Result<Self, std::io::Error> {
        let fd = unsafe {
            let mut config = static_inline_helpers::SPIMaster_Config {
                z__magicAndVersion: 0,
                csPolarity: spi::SPI_ChipSelectPolarity_SPI_ChipSelectPolarity_Invalid,
            };
            let ret = static_inline_helpers::SPIMaster_InitConfig_inline(&mut config);
            if ret == -1 {
                return Err(Error::last_os_error());
            }
            config.csPolarity = polarity as u32;
            static_inline_helpers::SPIMaster_Open_inline(interface_id, chip_select_id, &config)
        };
        if fd == -1 {
            Err(Error::last_os_error())
        } else {
            Ok(Self { fd })
        }
    }

    /// Sets the SPI bus speed for operations on an SPI master interface.
    pub fn set_bus_speed(&self, speed_in_hz: u32) -> Result<(), std::io::Error> {
        let ret = unsafe { spi::SPIMaster_SetBusSpeed(self.fd, speed_in_hz) };
        if ret == 0 {
            Ok(())
        } else {
            Err(Error::last_os_error())
        }
    }

    /// Sets the communication mode for an SPI master interface.
    pub fn set_mode(&self, mode: spi::SPI_Mode) -> Result<(), std::io::Error> {
        let ret = unsafe { spi::SPIMaster_SetMode(self.fd, mode) };
        if ret == 0 {
            Ok(())
        } else {
            Err(Error::last_os_error())
        }
    }

    /// Configures the order for transferring data bits on a SPI master interface.
    pub fn set_bit_order(&self, order: spi::SPI_BitOrder) -> Result<(), std::io::Error> {
        let ret = unsafe { spi::SPIMaster_SetBitOrder(self.fd, order) };
        if ret == 0 {
            Ok(())
        } else {
            Err(Error::last_os_error())
        }
    }

    /// Performs a sequence of a half-duplex writes immediately followed by a half-duplex read using the SPI master interface. This function enables chip select once before the sequence, and disables it when it ends.
    ///
    /// Each call is limited to at most 4096 bytes to read, and 4096 bytes to write. To transfer additional data, you need to call this function multiple times. Note that chip select will be asserted multiple times in this case.
    pub fn write_then_read(
        &self,
        write_buffer: &[u8],
        read_buffer: &mut [u8],
    ) -> Result<isize, std::io::Error> {
        let total_bytes = unsafe {
            static_inline_helpers::SPIMaster_WriteThenRead_inline(
                self.fd,
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

    /// Performs a sequence of half-duplex read or write transfers using the SPI master interface. This function enables chip select once before the sequence, and disables it when it ends. This function does not support simultaneously reading and writing in a single transfer.
    ///
    /// Each call to is limited to at most 4096 bytes to read, and 4096 bytes to write, independent of the number of actual transfers. To transfer additional data, you need to call this function multiple times. Note that chip select will be asserted multiple times in this case.
    pub fn transfer_sequential<'a, 'b>(
        &self,
        transfers: &mut [SPIMasterTransfer<'a, 'b>],
    ) -> Result<isize, std::io::Error> {
        // Initialize a template SPIMaster_Transfer
        let mut t_template = static_inline_helpers::SPIMaster_Transfer {
            z__magicAndVersion: 0,
            flags: static_inline_helpers::SPI_TransferFlags_SPI_TransferFlags_None,
            writeData: std::ptr::null(),
            readData: std::ptr::null_mut(),
            length: 0,
        };
        let ret =
            unsafe { static_inline_helpers::SPIMaster_InitTransfers_inline(&mut t_template, 1) };
        if ret == -1 {
            return Err(Error::last_os_error());
        }

        // Populate a vector of transfers
        let mut v =
            Vec::<static_inline_helpers::SPIMaster_Transfer>::with_capacity(transfers.len());
        for t in transfers {
            t_template.flags = t.flags;
            t_template.writeData = t.write_data.as_ptr();
            t_template.readData = t.read_data.as_mut_ptr();
            t_template.length = if t.write_data.len() == 0 {
                t.read_data.len() as libc::size_t
            } else {
                min(t.write_data.len(), t.read_data.len()) as libc::size_t
            };
            v.push(t_template)
        }

        let total_bytes = unsafe {
            static_inline_helpers::SPIMaster_TransferSequential_inline(
                self.fd,
                v.as_mut_ptr(),
                v.len(),
            )
        };
        if total_bytes == -1 {
            Err(Error::last_os_error())
        } else {
            Ok(total_bytes)
        }
    }
}

impl Drop for SPIMaster {
    fn drop(&mut self) {
        let _ = unsafe { libc::close(self.fd) };
    }
}
