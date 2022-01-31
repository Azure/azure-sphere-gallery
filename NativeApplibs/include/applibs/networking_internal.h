/// \file networking_internal.h
/// \brief This header contains internal functions of the Networking API and should not be included
/// directly; include applibs/networking.h instead.
#pragma once

#include <errno.h>
#include <stdint.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/// <summary>
///    Version support info for the Networking_NetworkInterface structs. Refer to
///    Networking_NetworkInterface and the appropriate networking_structs_v[n].h header for actual
///    struct fields.
/// </summary>
struct z__Networking_NetworkInterface_Base {
    /// <summary>Internal version field.</summary>
    uint32_t z__magicAndVersion;
};

/// <summary>
///     The network interface attribute to configure.
/// </summary>
typedef uint8_t z__Networking_Attribute;
enum {
    z__Networking_Attribute_Unknown = 0,
    /// <summary>Network interface hardware address</summary>
    z__Networking_Attribute_Hardware_Address = 1,
};

/// <summary>
///    A magic value that provides version support for the Networking_NetworkInterface struct.
/// </summary>
#define z__NETWORKING_NETWORK_INTERFACE_STRUCT_MAGIC 0xfa9a0000

/// <summary>
///    Adds struct version support to Networking_GetInterfaces. Do not use this directly; use
///    Networking_GetInterfaces instead.
/// </summary>
/// <param name="outNetworkInterfacesArray">
///     A pointer to a Networking_NetworkInterface structure that is type cast to
///     z__Networking_NetworkInterface_Base.
/// </param>
/// <param name="networkInterfacesArrayCount">
///     The number of elements in the outNetworkInterfacesArray.
/// </param>
/// <param name="networkInterfaceStructVersion">
///     The version of the network interface structure.
/// </param>
/// <returns>
///     If the call is successful, the number of network interfaces populated in the <paramref
///     name="outNetworkInterfacesArray"/> is returned. On failure, -1 is returned, in which case
///     errno is set to the error value.
/// </returns>
ssize_t z__Networking_GetInterfaces(
    struct z__Networking_NetworkInterface_Base *outNetworkInterfacesArray,
    size_t networkInterfacesArrayCount, uint32_t networkInterfaceStructVersion);

/// <summary>
///     Gets the list of registered network interfaces in the Azure Sphere device. If <paramref
///     name="outNetworkInterfacesArray"/> is too small to hold all network interfaces in the
///     system, this function uses the available space and returns the total number of interfaces
///     it is able to provide. The number of interfaces in the system will not change within a boot
///     cycle.
///      <para> **Errors** </para>
///     If an error is encountered, returns -1 and sets errno to the error value.
///     <para>EFAULT: the <paramref name="outNetworkInterfacesArray"/> is NULL.</para>
///     <para>ERANGE: the <paramref name="networkInterfacesArrayCount"/> is 0.</para>
///     Any other errno may be specified; such errors aren't deterministic and
///     there's no guarantee that the same behaviour will be retained through system updates.
/// </summary>
/// <param name="outNetworkInterfacesArray">
///      A pointer to the array, <see cref="Networking_NetworkInterface"/> to populate with
///      network interface properties. The caller needs to allocate memory for this after calling
///      <see cref="Networking_GetInterfacesCount" /> to find out the number of interfaces.
/// </param>
/// <param name="networkInterfacesArrayCount">
///      The number of elements <paramref name="outNetworkInterfacesArray"/> can hold. The array
///      should have one element for each network interface in the device.
/// </param>
/// <returns>
///     If the call is successful, the number of network interfaces populated in the <paramref
///     name="outNetworkInterfacesArray"/> is returned. On failure, -1 is returned, in which case
///     errno is set to the error value.
/// </returns>
static inline ssize_t Networking_GetInterfaces(
    Networking_NetworkInterface *outNetworkInterfacesArray, size_t networkInterfacesArrayCount)
{
    return z__Networking_GetInterfaces(
        (struct z__Networking_NetworkInterface_Base *)outNetworkInterfacesArray,
        networkInterfacesArrayCount, NETWORKING_STRUCTS_VERSION);
}

/// <summary>
///     Sets the attribute value for the specified network
///     <para> **Errors** </para>
///     If an error is encountered, returns -1 and sets errno to the error value.
///     <para>EACCES: the application manifest does not include the HardwareAddressConfig
///     capability.</para>
///     <para>ENOENT: the network interface <paramref name="networkInterfaceName"/> does not
///     exist.</para>
///     <para>EPERM: this function is not allowed on the interface.</para>
///     <para>EAGAIN: the networking stack isn't ready.</para>
///     <para>ERANGE: the <paramref name="valueLength"/> is greater than the max length for the
///     selected <paramref name="attribute"/>.</para>
///     <para>EINVAL: the value for <paramref name="attribute"/> or <paramref name="value"/> is
///     invalid.</para>
///     Any other errno may be specified; such errors aren't deterministic and there's no guarantee
///     that the same behavior will be retained through system updates.
/// </summary>
/// <param name="networkInterfaceName">
///     A pointer to a null-terminated string holding the interface name.
/// </param>
/// <param name="attribute">
///     The <see cref="z__Networking_Attribute"/> attribute to be configured.
/// </param>
/// <param name="value">
///     Pointer to appropriate data type containing the attribute value.
/// </param>
/// <param name="valueLength">
///     0 for null-terminated strings. For other types, size of <paramref name="value"/>.
/// </param>
/// <returns>
///     0 for success, or -1 for failure, in which case errno is set to the error value.
/// </returns>
int z__Networking_SetInterfaceOpt(const char *networkInterfaceName, z__Networking_Attribute attribute,
                                  const void *value, size_t valueLength);

/// <summary>
/// Retrieves the attribute value for the specified network interface.
///     <para> **Errors** </para>
///     If an error is encountered, returns -1 and sets errno to the error value.
///     <para>EFAULT: the <paramref name="buffer"/> is NULL while <paramref name="capacity"/> is
///     non-zero.</para>
///     <para>ENOENT: the <paramref name="networkInterfaceName"/> does not exist.</para>
///     <para>EPERM: this function is not allowed on the interface.</para>
///     <para>ERANGE: <paramref name="capacity"/> is not large enough to hold the given <paramref
///     name="attribute"/></para>
///     <para>EAGAIN: the networking stack isn't ready.</para>
///     <para>EINVAL: the value for <paramref name="attribute"/> is invalid.</para>
///     Any other errno may be specified; such errors aren't deterministic and there's no guarantee
///     that the same behavior will be retained through system updates.
/// </summary>
/// <param name="networkInterfaceName">
///     A pointer to a null-terminated string that identifies the interface.
/// </param>
/// <param name="attribute">
///     The <see cref="z__Networking_Attribute"/> value that defines the attribute being retrieved.
/// </param>
/// <param name="buffer">
///      A pointer to the buffer to receive the value.
///     If this pointer is NULL and <paramref name="capacity"/> is 0, the API will return the number
///     of bytes needed for copying the full data.
///     Otherwise the API will fail with ERANGE if the buffer is not NULL but not large enough to
///     receive the full data.
/// </param>
/// <param name="capacity">
///     The capacity of the <paramref name="buffer"/>.
/// </param>
/// <returns>
///     0 for success, or -1 for failure, in which case errno is set to the error value.
///
///     If <paramref name="buffer"/> is NULL and <paramref name="capacity"/> is 0, the number of
///     bytes needed to copy the data is returned.
///
///     If <paramref name="buffer"/> is not NULL and it's large enough to copy the data, the number
///     of bytes copied is returned.
/// </returns>
ssize_t z__Networking_GetInterfaceOpt(const char *networkInterfaceName,
                                      z__Networking_Attribute attribute, void *buffer,
                                      size_t capacity);

/// <summary>
///     Sets the hardware address for an interface. The hardware address is persisted across
///     reboots, and can only be set on an Ethernet interface. The application manifest must
///     include the HardwareAddressConfig capability.
///     <para> **Errors** </para>
///     If an error is encountered, returns -1 and sets errno to the error value.
///     <para>EACCES: the application manifest does not include the HardwareAddressConfig
///     capability.</para>
///     <para>ENOENT: the network interface does not exist.</para>
///     <para>EPERM: this function is not allowed on the interface.</para>
///     <para>EAGAIN: the networking stack isn't ready.</para>
///     <para>ERANGE: the <paramref name="hardwareAddressLength"/> greater than
///     <see cref="HARDWARE_ADDRESS_LENGTH"/>.</para>
///     <para>EINVAL: the <paramref name="hardwareAddress"/> is invalid. An all zeros hardware
///     address (00:00:00:00:00:00) is invalid.</para>
///     Any other errno may be specified; such errors aren't deterministic and there's no guarantee
///     that the same behavior will be retained through system updates.
/// </summary>
/// <param name="networkInterfaceName">
///      The name of the network interface to update.
/// </param>
/// <param name="hardwareAddress">
///      A pointer to an array of bytes containing the hardware address.
/// </param>
/// <param name="hardwareAddressLength">
///      The length of the hardware address. This should always be equal to <see
///      cref="HARDWARE_ADDRESS_LENGTH"/>.
/// </param>
/// <returns>
///     0 for success, or -1 for failure, in which case errno is set to the error value.
/// </returns>
static inline int Networking_SetHardwareAddress(const char *networkInterfaceName,
                                                const uint8_t *hardwareAddress,
                                                size_t hardwareAddressLength)
{
    return z__Networking_SetInterfaceOpt(networkInterfaceName,
                                         z__Networking_Attribute_Hardware_Address, hardwareAddress,
                                         hardwareAddressLength);
}

/// <summary>
///      Retrieves the hardware address of the given network interface.
///     <para> **Errors** </para>
///     If an error is encountered, returns -1 and sets errno to the error value.
///     <para>ENOENT: the network interface does not exist.</para>
///     <para>EPERM: this function is not allowed on the interface.</para>
///     <para>EAGAIN: the networking stack isn't ready.</para>
///     <para>EINVAL: the <paramref name="outAddress"/> is invalid.</para>
///     Any other errno may be specified; such errors aren't deterministic and there's no guarantee
///     that the same behavior will be retained through system updates.
/// </summary>
/// <param name="networkInterfaceName">
///      The name of the network interface to update.
/// </param>
/// <param name="outAddress">
///      A pointer to a <see cref="HardwareAddress"/> that receives the network interface's
///     hardware address.
/// </param>
/// <returns>
///     0 for success, or -1 for failure, in which case errno is set to the error value.
/// </returns>
static inline int Networking_GetHardwareAddress(const char *networkInterfaceName,
                                                Networking_Interface_HardwareAddress *outAddress)
{
    if (outAddress == NULL || networkInterfaceName == NULL) {
        errno = EFAULT;
        return -1;
    }

    uint8_t *buffer = outAddress->address;
    size_t bufferLength = sizeof(outAddress->address);
    ssize_t bytes_read = z__Networking_GetInterfaceOpt(
        networkInterfaceName, z__Networking_Attribute_Hardware_Address, buffer, bufferLength);

    if (bytes_read != (ssize_t)bufferLength) {
        return -1;
    }

    return 0;
}

#ifdef __cplusplus
}
#endif
