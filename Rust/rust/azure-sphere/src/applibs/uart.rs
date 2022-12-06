//! The Applibs uart module contains functions and types that open and use a UART (Universal Asynchronous Receiver/Transmitter) on a device.
use azure_sphere_sys::applibs::static_inline_helpers;
use std::fs::File;
use std::io::Error;
use std::os::unix::io::FromRawFd;

/// The configuration options for a UART
pub struct UARTConfig {
    /// The baud rate of the UART.
    pub baud_rate: u32,
    /// The blocking mode setting for the UART.
    pub blocking_mode: u8,
    //// The data bits setting for the UART.
    pub data_bits: u8,
    /// The parity setting for the UART.
    pub parity: u8,
    /// The stop bits setting for the UART.
    pub stop_bits: u8,
    /// The flow control setting for the UART.
    pub flow_control: u8,
}

/// The default UART settings are 8 for dataBits, 0 (none) for parity, and 1 for stopBits.
impl Default for UARTConfig {
    fn default() -> Self {
        UARTConfig {
            baud_rate: 0,
            blocking_mode: 0,
            data_bits: 8,
            parity: 0,
            stop_bits: 1,
            flow_control: 0,
        }
    }
}

/// Opens and configures a UART, and returns a File to use for subsequent calls.
/// To access individual UARTs, your application must identify them in the Uart field of the application manifest.
pub fn open(uart_id: i32, config: UARTConfig) -> Result<std::fs::File, std::io::Error> {
    let mut ffi_cfg = static_inline_helpers::UART_Config {
        z__magicAndVersion: 0,
        baudRate: 0,
        blockingMode: 0,
        dataBits: 0,
        parity: 0,
        stopBits: 0,
        flowControl: 0,
    };
    unsafe { static_inline_helpers::UART_InitConfig_inline(&mut ffi_cfg) };
    ffi_cfg.baudRate = config.baud_rate;
    ffi_cfg.blockingMode = config.blocking_mode;
    ffi_cfg.dataBits = config.data_bits;
    ffi_cfg.parity = config.parity;
    ffi_cfg.stopBits = config.stop_bits;
    ffi_cfg.flowControl = config.flow_control;

    let fd = unsafe { static_inline_helpers::UART_Open_inline(uart_id, &mut ffi_cfg) };
    if fd == -1 {
        Err(Error::last_os_error())
    } else {
        Ok(unsafe { File::from_raw_fd(fd) })
    }
}
