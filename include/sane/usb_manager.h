/*
   Copyright (C) 2024 Huawei Device Co., Ltd.

   This file is part of the SANE package.

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.

   As a special exception, the authors of SANE give permission for
   additional uses of the libraries contained in this release of SANE.

   The exception is that, if you link a SANE library with other files
   to produce an executable, this does not by itself cause the
   resulting executable to be covered by the GNU General Public
   License.  Your use of that executable is in no way restricted on
   account of linking the SANE library code into it.

   This exception does not, however, invalidate any other reasons why
   the executable file might be covered by the GNU General Public
   License.

   If you submit changes to SANE to the maintainers to be included in
   a subsequent release, you agree by submitting the changes that
   those changes may be distributed with this exception intact.

   If you write modifications of your own for SANE, it is your choice
   whether to permit this exception to apply to your modifications.
   If you do not wish that, delete this exception notice.

   This file provides an interface to use the usb manager.*/

#ifndef USB_MANAGER_H
#define USB_MANAGER_H

#include <stdint.h>
#include <stdbool.h>

#include "hilog/log.h"
#define SANE_LOG_TAG "sanekit"
#define SANE_HILOG_INFO(...) ((void)HiLogPrint(LOG_APP, LOG_INFO, 0, "sanekit", __VA_ARGS__))

#ifdef __cplusplus
extern "C" {
#endif

/* in bmAttributes */
#define USB_MANAGER_TRANSFER_TYPE_MASK      0x03
#define USB_MANAGER_ENDPOINT_DIR_MASK       0x80
#define USB_DT_CONFIGURATION     0x02
#define USB_DT_INTERFACE         0x04
#define USB_DT_ENDPOINT          0x05

enum usb_manager_option {
    USB_MANAGER_OPTION_LOG_LEVEL = 0,
};

/** \ingroup usb_manager_desc
 * Endpoint direction. Values for bit 7 of the
 * \ref usb_manager_endpoint_descriptor::bEndpointAddress "endpoint address" scheme.
 */
enum usb_manager_endpoint_direction {
    /** Out: host-to-device */
    USB_MANAGER_ENDPOINT_OUT = 0x00,

    /** In: device-to-host */
    USB_MANAGER_ENDPOINT_IN = 0x80
};

enum usb_manager_transfer_type {
    /** Control transfer */
    USB_MANAGER_TRANSFER_TYPE_CONTROL = 0U,

    /** Isochronous transfer */
    USB_MANAGER_TRANSFER_TYPE_ISOCHRONOUS = 1U,

    /** Bulk transfer */
    USB_MANAGER_TRANSFER_TYPE_BULK = 2U,

    /** Interrupt transfer */
    USB_MANAGER_TRANSFER_TYPE_INTERRUPT = 3U,

    /** Bulk stream transfer */
    USB_MANAGER_TRANSFER_TYPE_BULK_STREAM = 4U
};

enum usb_manager_error {
    /** Success (no error) */
    USB_MANAGER_SUCCESS = 0,

    /** Input/output error */
    USB_MANAGER_ERROR_IO = -1,

    /** Invalid parameter */
    USB_MANAGER_ERROR_INVALID_PARAM = -2,

    /** Access denied (insufficient permissions) */
    USB_MANAGER_ERROR_ACCESS = -3,

    /** No such device (it may have been disconnected) */
    USB_MANAGER_ERROR_NO_DEVICE = -4,

    /** Entity not found */
    USB_MANAGER_ERROR_NOT_FOUND = -5,

    /** Resource busy */
    USB_MANAGER_ERROR_BUSY = -6,

    /** Operation timed out */
    USB_MANAGER_ERROR_TIMEOUT = -7,

    /** Overflow */
    USB_MANAGER_ERROR_OVERFLOW = -8,

    /** Pipe error */
    USB_MANAGER_ERROR_PIPE = -9,

    /** System call interrupted (perhaps due to signal) */
    USB_MANAGER_ERROR_INTERRUPTED = -10,

    /** Insufficient memory */
    USB_MANAGER_ERROR_NO_MEM = -11,

    /** Operation not supported or unimplemented on this platform */
    USB_MANAGER_ERROR_NOT_SUPPORTED = -12,

    /* NB: Remember to update USB_MANAGER_ERROR_COUNT below as well as the
        message strings in strerror.c when adding new error codes here. */

    /** Other error */
    USB_MANAGER_ERROR_OTHER = -99
};

/** \ingroup usb_manager_desc
 * Device and/or Interface Class codes */
enum usb_manager_class_code {
    /** In the context of a \ref usb_manager_device_descriptor "device descriptor",
        * this bDeviceClass value indicates that each interface specifies its
        * own class information and all interfaces operate independently.
        */
    USB_MANAGER_CLASS_PER_INTERFACE = 0x00,

    /** Audio class */
    USB_MANAGER_CLASS_AUDIO = 0x01,

    /** Communications class */
    USB_MANAGER_CLASS_COMM = 0x02,

    /** Human Interface Device class */
    USB_MANAGER_CLASS_HID = 0x03,

    /** Physical */
    USB_MANAGER_CLASS_PHYSICAL = 0x05,

    /** Image class */
    USB_MANAGER_CLASS_IMAGE = 0x06,
    USB_MANAGER_CLASS_PTP = 0x06, /* legacy name from usb manager usb.h */

    /** Printer class */
    USB_MANAGER_CLASS_PRINTER = 0x07,

    /** Mass storage class */
    USB_MANAGER_CLASS_MASS_STORAGE = 0x08,

    /** Hub class */
    USB_MANAGER_CLASS_HUB = 0x09,

    /** Data class */
    USB_MANAGER_CLASS_DATA = 0x0a,

    /** Smart Card */
    USB_MANAGER_CLASS_SMART_CARD = 0x0b,

    /** Content Security */
    USB_MANAGER_CLASS_CONTENT_SECURITY = 0x0d,

    /** Video */
    USB_MANAGER_CLASS_VIDEO = 0x0e,

    /** Personal Healthcare */
    USB_MANAGER_CLASS_PERSONAL_HEALTHCARE = 0x0f,

    /** Diagnostic Device */
    USB_MANAGER_CLASS_DIAGNOSTIC_DEVICE = 0xdc,

    /** Wireless class */
    USB_MANAGER_CLASS_WIRELESS = 0xe0,

    /** Miscellaneous class */
    USB_MANAGER_CLASS_MISCELLANEOUS = 0xef,

    /** Application class */
    USB_MANAGER_CLASS_APPLICATION = 0xfe,

    /** Class is vendor-specific */
    USB_MANAGER_CLASS_VENDOR_SPEC = 0xff
};

/** \ingroup usb_manager_desc
 * A structure representing the standard USB device descriptor. This
 * descriptor is documented in section 9.6.1 of the USB 3.0 specification.
 * All multiple-byte fields are represented in host-endian format.
 */
struct usb_manager_device_descriptor {
    /** Size of this descriptor (in bytes) */
    uint8_t  bLength;

    /** Descriptor type. Will have value
        * \ref usb_manager_descriptor_type::USB_MANAGER_DT_DEVICE USB_MANAGER_DT_DEVICE in this
        * context. */
    uint8_t  bDescriptorType;

    /** USB specification release number in binary-coded decimal. A value of
        * 0x0200 indicates USB 2.0, 0x0110 indicates USB 1.1, etc. */
    uint16_t bcdUSB;

    /** USB-IF class code for the device. See \ref usb_manager_class_code. */
    uint8_t  bDeviceClass;

    /** USB-IF subclass code for the device, qualified by the bDeviceClass
        * value */
    uint8_t  bDeviceSubClass;

    /** USB-IF protocol code for the device, qualified by the bDeviceClass and
        * bDeviceSubClass values */
    uint8_t  bDeviceProtocol;

    /** Maximum packet size for endpoint 0 */
    uint8_t  bMaxPacketSize0;

    /** USB-IF vendor ID */
    uint16_t idVendor;

    /** USB-IF product ID */
    uint16_t idProduct;

    /** Device release number in binary-coded decimal */
    uint16_t bcdDevice;

    /** Index of string descriptor describing manufacturer */
    uint8_t  iManufacturer;

    /** Index of string descriptor describing product */
    uint8_t  iProduct;

    /** Index of string descriptor containing device serial number */
    uint8_t  iSerialNumber;

    /** Number of possible configurations */
    uint8_t  bNumConfigurations;
};

/** \ingroup usb_manager_desc
 * A structure representing the standard USB endpoint descriptor. This
 * descriptor is documented in section 9.6.6 of the USB 3.0 specification.
 * All multiple-byte fields are represented in host-endian format.
 */
struct usb_manager_endpoint_descriptor {
    /** Size of this descriptor (in bytes) */
    uint8_t  bLength;

    /** Descriptor type. Will have value
        * \ref usb_manager_descriptor_type::USB_MANAGER_DT_ENDPOINT USB_MANAGER_DT_ENDPOINT in
        * this context. */
    uint8_t  bDescriptorType;

    /** The address of the endpoint described by this descriptor. Bits 0:3 are
        * the endpoint number. Bits 4:6 are reserved. Bit 7 indicates direction,
        * see \ref usb_manager_endpoint_direction. */
    uint8_t  bEndpointAddress;

    /** Attributes which apply to the endpoint when it is configured using
        * the bConfigurationValue. Bits 0:1 determine the transfer type and
        * correspond to \ref usb_manager_endpoint_transfer_type. Bits 2:3 are only used
        * for isochronous endpoints and correspond to \ref usb_manager_iso_sync_type.
        * Bits 4:5 are also only used for isochronous endpoints and correspond to
        * \ref usb_manager_iso_usage_type. Bits 6:7 are reserved. */
    uint8_t  bmAttributes;

    /** Maximum packet size this endpoint is capable of sending/receiving. */
    uint16_t wMaxPacketSize;

    /** Interval for polling endpoint for data transfers. */
    uint8_t  bInterval;

    /** For audio devices only: the rate at which synchronization feedback
        * is provided. */
    uint8_t  bRefresh;

    /** For audio devices only: the address if the synch endpoint */
    uint8_t  bSynchAddress;
};

/** \ingroup usb_manager_desc
 * A structure representing the standard USB interface descriptor. This
 * descriptor is documented in section 9.6.5 of the USB 3.0 specification.
 * All multiple-byte fields are represented in host-endian format.
 */
struct usb_manager_interface_descriptor {
    /** Size of this descriptor (in bytes) */
    uint8_t  bLength;

    /** Descriptor type. Will have value
        * \ref usb_manager_descriptor_type::USB_MANAGER_DT_INTERFACE USB_MANAGER_DT_INTERFACE
        * in this context. */
    uint8_t  bDescriptorType;

    /** Number of this interface */
    uint8_t  bInterfaceNumber;

    /** Value used to select this alternate setting for this interface */
    uint8_t  bAlternateSetting;

    /** Number of endpoints used by this interface (excluding the control
        * endpoint). */
    uint8_t  bNumEndpoints;

    /** USB-IF class code for this interface. See \ref usb_manager_class_code. */
    uint8_t  bInterfaceClass;

    /** USB-IF subclass code for this interface, qualified by the
        * bInterfaceClass value */
    uint8_t  bInterfaceSubClass;

    /** USB-IF protocol code for this interface, qualified by the
        * bInterfaceClass and bInterfaceSubClass values */
    uint8_t  bInterfaceProtocol;

    /** Index of string descriptor describing this interface */
    uint8_t  iInterface;

    /** Array of endpoint descriptors. This length of this array is determined
        * by the bNumEndpoints field. */
    struct usb_manager_endpoint_descriptor *endpoint;
};


/** \ingroup usb_manager_desc
 * A collection of alternate settings for a particular USB interface.
 */
struct usb_manager_interface {
    /** Array of interface descriptors. The length of this array is determined
        * by the num_altsetting field. */
    struct usb_manager_interface_descriptor *altsetting;

    /** The number of alternate settings that belong to this interface.
        * Must be non-negative. */
    int num_altsetting;
};

/** \ingroup usb_manager_desc
 * A structure representing the standard USB configuration descriptor. This
 * descriptor is documented in section 9.6.3 of the USB 3.0 specification.
 * All multiple-byte fields are represented in host-endian format.
 */
struct usb_manager_config_descriptor {
    /** Size of this descriptor (in bytes) */
    uint8_t  bLength;

    /** Descriptor type. Will have value
        * \ref usb_manager_descriptor_type::USB_MANAGER_DT_CONFIG USB_MANAGER_DT_CONFIG
        * in this context. */
    uint8_t  bDescriptorType;

    /** Total length of data returned for this configuration */
    uint16_t wTotalLength;

    /** Number of interfaces supported by this configuration */
    uint8_t  bNumInterfaces;

    /** Identifier value for this configuration */
    uint8_t  bConfigurationValue;

    /** Index of string descriptor describing this configuration */
    uint8_t  iConfiguration;

    /** Configuration characteristics */
    uint8_t  bmAttributes;

    /** Maximum power consumption of the USB device from this bus in this
        * configuration when the device is fully operation. Expressed in units
        * of 2 mA when the device is operating in high-speed mode and in units
        * of 8 mA when the device is operating in super-speed mode. */
    uint8_t  MaxPower;

    /** Array of interfaces supported by this configuration. The length of
        * this array is determined by the bNumInterfaces field. */
    struct usb_manager_interface *interface;
};

struct usb_manager_device;

struct usb_manager_device_handle;

struct usb_manager_context {
    struct usb_manager_device *devices;
    uint32_t deviceCount;
};

typedef struct usb_manager_context usb_manager_context;
typedef struct usb_manager_device usb_manager_device;
typedef struct usb_manager_device_handle usb_manager_device_handle;
typedef struct usb_manager_device_descriptor usb_manager_device_descriptor;
typedef struct usb_manager_config_descriptor usb_manager_config_descriptor;

int usb_manager_init(usb_manager_context **ctx);
int usb_manager_exit(usb_manager_context *ctx);
ssize_t usb_manager_get_device_list(usb_manager_context *ctx, usb_manager_device ***list);
int usb_manager_get_bus_number(usb_manager_device *dev);
int usb_manager_get_device_address(usb_manager_device *dev);
int usb_manager_get_device_descriptor(usb_manager_device *dev, usb_manager_device_descriptor *desc);
int usb_manager_open(usb_manager_device *dev, usb_manager_device_handle **dev_handle);
void usb_manager_close(usb_manager_device_handle *dev_handle);
int usb_manager_get_configuration(usb_manager_device_handle *dev, int *config);
int usb_manager_get_config_descriptor(usb_manager_device *dev, uint8_t config_index,
    usb_manager_config_descriptor **config);
void usb_manager_free_config_descriptor(usb_manager_config_descriptor *config);
void usb_manager_free_device_list(usb_manager_device **list, int unref_devices);
int usb_manager_set_configuration(usb_manager_device_handle *dev_handle, int configuration);
int usb_manager_claim_interface(usb_manager_device_handle *dev_handle, int interface_number);
int usb_manager_release_interface(usb_manager_device_handle *dev_handle, int interface_number);
int usb_manager_clear_halt(usb_manager_device_handle *dev_handle, unsigned char endpoint);
int usb_manager_reset_device(usb_manager_device_handle *dev_handle);
int usb_manager_bulk_transfer(usb_manager_device_handle *dev_handle, unsigned char endpoint,
    unsigned char *data, int length, int *transferred, unsigned int timeout);
int usb_manager_control_transfer(usb_manager_device_handle *dev_handle, uint8_t request_type,
    uint8_t bRequest, uint16_t wValue, uint16_t wIndex, unsigned char *data, uint16_t wLength,
    unsigned int timeout);
int usb_manager_interrupt_transfer(usb_manager_device_handle *dev_handle, unsigned char endpoint,
    unsigned char *data, int length, int *actual_length, unsigned int timeout);
int usb_manager_set_interface_alt_setting(usb_manager_device_handle *dev_handle, int interface_number,
    int alternate_setting);
usb_manager_device* usb_manager_ref_device(usb_manager_device *dev);

#ifdef __cplusplus
}
#endif

#endif /* USB_MANAGER_H */