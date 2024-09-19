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

#include <cstring>
#include <string>
#include "iusb_srv.h"
#include "securec.h"
#include "cstring"
#include "usb_srv_client.h"
#include "usb_manager.h"
#include "v1_0/iusb_interface.h"
#include "hilog/log.h"

#define SANE_LOG_TAG "sanekit"
#define SANE_HILOG_INFO(...) ((void)HiLogPrint(LOG_APP, LOG_INFO, 0, "sanekit", __VA_ARGS__))
#define SANE_HILOG_ERROR(...) ((void)HiLogPrint(LOG_APP, LOG_ERROR, 0, "sanekit", __VA_ARGS__))
#define SANE_HILOG_DEBUG(...) ((void)HiLogPrint(LOG_APP, LOG_DEBUG, 0, "sanekit", __VA_ARGS__))
#define SANE_HILOG_WARN(...) ((void)HiLogPrint(LOG_APP, LOG_WARN, 0, "sanekit", __VA_ARGS__))

using namespace OHOS::USB;

static std::vector<UsbDevice> g_deviceList;
static ssize_t g_deviceCount = 0;
static uint8_t g_interfaceNumber = 0;
static usb_manager_config_descriptor* g_deviceConfig = nullptr;
static std::vector<usb_manager_device*> g_refDevices;
constexpr int maxUsbInterfaceNum = 1000;
constexpr int maxUsbEndpointNum = 1000;

struct usb_manager_device_handle {
    UsbDevice device;
    USBDevicePipe pipe;
};

struct usb_manager_device {
    UsbDevice device;
};

struct UsbEndPointDes {
    uint8_t bmAttributes;
    uint8_t  bEndpointAddress;
};

struct UsbInterfaceAltDes{
    uint8_t bInterfaceClass;
    std::vector<UsbEndPointDes> endpointDes;
};

struct UsbAltInterface {
    std::vector<UsbInterfaceAltDes> altDes;
};

namespace {
    void MemoryFreeDeviceList(usb_manager_device **list)
    {
        if (list == nullptr) {
            return;
        }
        for (ssize_t i = 0 ; i < g_deviceCount; i++) {
            if (list[i] != nullptr) {
                delete list[i];
            }
        }
        delete[] list;
        list = nullptr;
        g_deviceCount = 0;
    }

    void InterfaceConvert(std::vector<UsbInterface> &usbInterfaces, std::vector<UsbAltInterface> &usbAlt)
    {
        auto memCopyInter = [&](size_t &i, UsbAltInterface &altInter) {
            UsbInterfaceAltDes des;
            des.bInterfaceClass = static_cast<uint8_t>(usbInterfaces[i].GetClass());
            std::vector<USBEndpoint> &endPoints = usbInterfaces[i].GetEndpoints();
            for (size_t j = 0; j < endPoints.size(); j++) {
                UsbEndPointDes pointDes;
                pointDes.bmAttributes = static_cast<uint8_t>(endPoints[j].GetAttributes());
                pointDes.bEndpointAddress = static_cast<uint8_t>(endPoints[j].GetAddress());
                des.endpointDes.push_back(pointDes);
            }
            altInter.altDes.push_back(des);
        };
        for (size_t i = 0; i < usbInterfaces.size(); i++) {
            UsbInterface& inter = usbInterfaces[i];
            if (inter.GetAlternateSetting() == 0) {
                UsbAltInterface altInter;
                memCopyInter(i, altInter);
                usbAlt.push_back(altInter);
            } else {
                int id = usbInterfaces[i].GetId();
                UsbAltInterface &altInter = usbAlt[id];
                memCopyInter(i, altInter);
            }
        }
    }

    int GetRetConfigAndInterface(usb_manager_device *dev, uint8_t config_index, usb_manager_config_descriptor* &retConfig,
        usb_manager_interface* &retInterface, std::vector<UsbAltInterface> &usbAlt)
    {
        std::vector<USBConfig> &configs = dev->device.GetConfigs();
        if (config_index >= configs.size()) {
            SANE_HILOG_ERROR("%s: config_index(%u) is invalid.", __func__, config_index);
            return USB_MANAGER_ERROR_INVALID_PARAM;
        }
        USBConfig &usbConfig = configs[config_index];
        SANE_HILOG_DEBUG("%s: config[%u] = %s", __func__, config_index, usbConfig.getJsonString().c_str());
        std::vector<UsbInterface> &usbInterfaces = usbConfig.GetInterfaces();
        InterfaceConvert(usbInterfaces, usbAlt);
        size_t bNumInterfaces = usbAlt.size();
        if (bNumInterfaces == 0 || bNumInterfaces > maxUsbInterfaceNum) {
            SANE_HILOG_ERROR("%s: bNumInterfaces(%u) is invalid.", __func__, bNumInterfaces);
            return USB_MANAGER_ERROR_OTHER;
        }
        retConfig = new (std::nothrow) usb_manager_config_descriptor();
        if (retConfig == nullptr) {
            SANE_HILOG_ERROR("%s: Not enough memory.", __func__);
            return USB_MANAGER_ERROR_NO_MEM;
        }
        if (memset_s(retConfig, sizeof(usb_manager_config_descriptor), 0, sizeof(usb_manager_config_descriptor)) != 0) {
            SANE_HILOG_ERROR("%s: memset retConfig error.", __func__);
            delete retConfig;
            retConfig = nullptr;
            return USB_MANAGER_ERROR_OTHER;
        }
        retConfig->bNumInterfaces = static_cast<uint8_t>(bNumInterfaces);
        retConfig->bConfigurationValue = static_cast<uint8_t>(usbConfig.GetId());
        retInterface = new (std::nothrow) usb_manager_interface[bNumInterfaces]{};
        if (retInterface == nullptr) {
            SANE_HILOG_ERROR("%s: Not enough memory.", __func__);
            delete retConfig;
            return USB_MANAGER_ERROR_NO_MEM;
        }
        retConfig->interface = retInterface;
        return USB_MANAGER_SUCCESS;
    }

    bool SetInterfaceDescriptor(std::vector<UsbAltInterface> &usbAlt, usb_manager_interface* retInterface)
    {
        if (retInterface == nullptr) {
            return false;
        }
        for (int i = 0; i < usbAlt.size(); i++) {
            size_t altSet = usbAlt[i].altDes.size();
            if (altSet > maxUsbInterfaceNum) {
                return false;
            }
            auto retAltsetting = new (std::nothrow) usb_manager_interface_descriptor[altSet]{};
            if (retAltsetting == nullptr) {
                return false;
            }
            retInterface[i].altsetting = retAltsetting;
            retInterface[i].num_altsetting = static_cast<int>(altSet);
            for (size_t j = 0; j < altSet; j++) {
                retAltsetting[j].bInterfaceClass = usbAlt[i].altDes[j].bInterfaceClass;
                size_t endPointsNum = usbAlt[i].altDes[j].endpointDes.size();
                if (endPointsNum > maxUsbEndpointNum) {
                    return false;
                }
                retAltsetting[j].bNumEndpoints = endPointsNum;
                retAltsetting[j].endpoint = new (std::nothrow) usb_manager_endpoint_descriptor[endPointsNum]{};
                if (retAltsetting[j].endpoint == nullptr) {
                    return false;
                }
                for (int k = 0 ; k < endPointsNum; k++) {
                    uint8_t bmAttr = usbAlt[i].altDes[j].endpointDes[k].bmAttributes;
                    uint8_t bEndpointAddress = usbAlt[i].altDes[j].endpointDes[k].bEndpointAddress;
                    retAltsetting[j].endpoint[k].bmAttributes = bmAttr;
                    retAltsetting[j].endpoint[k].bEndpointAddress = bEndpointAddress;
                }
            }
        }
        return true;
    }
};

int usb_manager_init(usb_manager_context **ctx)
{
    SANE_HILOG_INFO("%s: begin", __func__);
    if (ctx == nullptr) {
        SANE_HILOG_ERROR("%s: ctx is a nullptr.", __func__);
        return USB_MANAGER_ERROR_INVALID_PARAM;
    }
    usb_manager_context *usbmanagerContext = new (std::nothrow) usb_manager_context();
    if (usbmanagerContext == nullptr) {
        SANE_HILOG_ERROR("%s: Not enough memory.", __func__);
        return USB_MANAGER_ERROR_NO_MEM;
    }
    if (memset_s(usbmanagerContext, sizeof(usb_manager_context), 0 ,sizeof(usb_manager_context)) != 0) {
        SANE_HILOG_ERROR("%s: memset_s usbmanagerContext error.", __func__);
        delete usbmanagerContext;
        return USB_MANAGER_ERROR_OTHER;
    }
    *ctx = usbmanagerContext;
    SANE_HILOG_INFO("%s: end successful", __func__);
    return USB_MANAGER_SUCCESS;
}

int usb_manager_exit(usb_manager_context *ctx)
{
    SANE_HILOG_INFO("%s: begin", __func__);
    if (ctx == nullptr) {
        SANE_HILOG_ERROR("%s: ctx is a nullptr.", __func__);
        return USB_MANAGER_ERROR_INVALID_PARAM;
    }
    delete ctx;
    ctx = nullptr;
    for (const auto& refDevice : g_refDevices) {
        if (refDevice != nullptr) {
            delete refDevice;
        }
    }
    g_refDevices.clear();
    SANE_HILOG_INFO("%s: end successful", __func__);
    return USB_MANAGER_SUCCESS;
}

ssize_t usb_manager_get_device_list(usb_manager_context *ctx, usb_manager_device ***list)
{
    SANE_HILOG_INFO("%s: begin", __func__);
    if (ctx == nullptr || list == nullptr) {
        SANE_HILOG_ERROR("%s: ctx or list is a nullptr.", __func__);
        return USB_MANAGER_ERROR_INVALID_PARAM;
    }
    g_deviceList.clear();
    auto ret = UsbSrvClient::GetInstance().GetDevices(g_deviceList);
    if (ret != USB_MANAGER_SUCCESS) {
        SANE_HILOG_ERROR("%s: GetDevices exception.", __func__);
        return USB_MANAGER_ERROR_IO;
    }
    g_deviceCount = static_cast<ssize_t>(g_deviceList.size());
    SANE_HILOG_DEBUG("%s: deviceCount : %d", __func__, g_deviceCount);
    if (g_deviceCount == 0) {
        SANE_HILOG_DEBUG("%s: not found device.", __func__);
        return USB_MANAGER_SUCCESS;
    }
    usb_manager_device** usbManagerDeviceList = new (std::nothrow) usb_manager_device* [g_deviceCount]{};
    if (usbManagerDeviceList == nullptr) {
        SANE_HILOG_ERROR("%s: Not enough memory.", __func__);
        return USB_MANAGER_ERROR_NO_MEM;
    }
    for (ssize_t i = 0; i < g_deviceCount; i++) {
        usb_manager_device* usbDevice = new (std::nothrow) usb_manager_device();
        if (usbDevice == nullptr) {
            SANE_HILOG_ERROR("%s: Not enough memory.", __func__);
            MemoryFreeDeviceList(usbManagerDeviceList);
            return USB_MANAGER_ERROR_NO_MEM;
        }
        usbManagerDeviceList[i] = usbDevice;
        usbDevice->device = g_deviceList[i];
    }
    *list = usbManagerDeviceList;
    SANE_HILOG_INFO("%s: end successful", __func__);
    return g_deviceCount;
}

int usb_manager_get_bus_number(usb_manager_device *dev)
{
    SANE_HILOG_INFO("%s: start", __func__);
    if (dev == nullptr) {
        SANE_HILOG_ERROR("%s: dev is a nullptr.", __func__);
        return USB_MANAGER_ERROR_INVALID_PARAM;
    }
    SANE_HILOG_INFO("%s: end successful", __func__);
    return dev->device.GetBusNum();
}

int usb_manager_get_device_address(usb_manager_device *dev)
{
    SANE_HILOG_INFO("%s: start", __func__);
    if (dev == nullptr) {
        SANE_HILOG_ERROR("%s: dev is a nullptr.", __func__);
        return USB_MANAGER_ERROR_INVALID_PARAM;
    }
    SANE_HILOG_INFO("%s: end successful", __func__);
    return dev->device.GetDevAddr();
}

int usb_manager_get_device_descriptor(usb_manager_device *dev, usb_manager_device_descriptor *desc)
{
    SANE_HILOG_INFO("%s: start", __func__);
    if (dev == nullptr || desc == nullptr) {
        SANE_HILOG_ERROR("%s: dev or desc is a nullptr.", __func__);
        return USB_MANAGER_ERROR_INVALID_PARAM;
    }
    UsbDevice &device = dev->device;
    desc->idVendor = static_cast<uint16_t>(device.GetVendorId());
    desc->idProduct = static_cast<uint16_t>(device.GetProductId());
    desc->bDeviceClass = static_cast<uint8_t>(device.GetClass());
    desc->bNumConfigurations = static_cast<uint8_t>(device.GetConfigCount());
    desc->bDeviceSubClass = static_cast<uint8_t>(device.GetSubclass());
    desc->bDeviceProtocol = static_cast<uint8_t>(device.GetProtocol());
    SANE_HILOG_INFO("%s: end successful", __func__);
    return USB_MANAGER_SUCCESS;
}

int usb_manager_open(usb_manager_device *dev, usb_manager_device_handle **dev_handle)
{
    SANE_HILOG_INFO("%s: start", __func__);
    if (dev == nullptr || dev_handle == nullptr) {
        SANE_HILOG_ERROR("%s: dev or dev_handle is a nullptr.", __func__);
        return USB_MANAGER_ERROR_INVALID_PARAM;
    }
    auto &usbSrvClient = UsbSrvClient::GetInstance();
    USBDevicePipe pipe;
    UsbDevice &device = dev->device;
    auto ret = usbSrvClient.RequestRight(device.GetName());
    if (ret != USB_MANAGER_SUCCESS) {
        SANE_HILOG_ERROR("request right failed ret=%{public}d", ret);
        return USB_MANAGER_ERROR_ACCESS;
    }
    ret = usbSrvClient.OpenDevice(device, pipe);
    if (ret != USB_MANAGER_SUCCESS) {
        SANE_HILOG_ERROR("open device failed ret=%{public}d", ret);
        return USB_MANAGER_ERROR_ACCESS;
    }
    auto handle = new (std::nothrow) usb_manager_device_handle();
    if (handle == nullptr) {
        SANE_HILOG_ERROR("%s: Not enough memory.", __func__);
        usbSrvClient.Close(pipe);
        return USB_MANAGER_ERROR_NO_MEM;
    }
    handle->device = device;
    handle->pipe = pipe;
    *dev_handle = handle;
    SANE_HILOG_INFO("%s: end successful", __func__);
    return USB_MANAGER_SUCCESS;
}

void usb_manager_close(usb_manager_device_handle *dev_handle)
{
    SANE_HILOG_INFO("%s: start", __func__);
    if (dev_handle == nullptr) {
        SANE_HILOG_ERROR("%s: dev_handle is a nullptr", __func__);
        return;
    }
    auto &usbSrvClient = UsbSrvClient::GetInstance();
    usbSrvClient.Close(dev_handle->pipe);
    delete dev_handle;
    dev_handle = nullptr;
    SANE_HILOG_INFO("%s: end", __func__);
    return;
}

int usb_manager_get_configuration(usb_manager_device_handle *dev, int *config)
{
    SANE_HILOG_INFO("%s: start", __func__);
    if (dev == nullptr || config == nullptr) {
        SANE_HILOG_ERROR("%s: dev or config is a nullptr", __func__);
        return USB_MANAGER_ERROR_INVALID_PARAM;
    }
    UsbDevice &device = dev->device;
    auto configCount = device.GetConfigCount();
    SANE_HILOG_DEBUG("%s: configCount = %d", __func__, configCount);
    *config = configCount;
    return USB_MANAGER_SUCCESS;
}

int usb_manager_get_config_descriptor(usb_manager_device *dev, uint8_t config_index,
    usb_manager_config_descriptor **config)
{
    SANE_HILOG_INFO("%s: begin", __func__);
    if (dev == nullptr || config == nullptr || config_index < 0) {
        SANE_HILOG_ERROR("%s: input param is invalid!", __func__);
        return USB_MANAGER_ERROR_INVALID_PARAM;
    }
    usb_manager_config_descriptor* retConfig = nullptr;
    usb_manager_interface* retInterface = nullptr;
    std::vector<UsbAltInterface> usbAlt;
    auto retStatus = GetRetConfigAndInterface(dev, config_index, retConfig, retInterface, usbAlt);
    if (retConfig == nullptr || retInterface == nullptr || retStatus != USB_MANAGER_SUCCESS) {
        SANE_HILOG_ERROR("%s: GetRetConfigAndInterface error!", __func__);
        return retStatus;
    }
    retConfig->interface = retInterface;
    if (!SetInterfaceDescriptor(usbAlt, retInterface)) {
        usb_manager_free_config_descriptor(retConfig);
        return USB_MANAGER_ERROR_NO_MEM;
    }
    *config = retConfig;
    g_deviceConfig = retConfig;
    SANE_HILOG_INFO("%s: end", __func__);
    return USB_MANAGER_SUCCESS;
}


void usb_manager_free_config_descriptor(usb_manager_config_descriptor *config)
{
    SANE_HILOG_INFO("%s: start", __func__);
    if (config == nullptr) {
        SANE_HILOG_ERROR("%s: config is a nullptr", __func__);
        return;
    }
    usb_manager_interface *interface = config->interface;
    if (interface == nullptr) {
        SANE_HILOG_ERROR("%s: interface is a nullptr", __func__);
        delete config;
        config = nullptr;
        return;
    }
    // release all interface
    for (int i = 0; i < config->bNumInterfaces; i++) {
        // release alternate setting
        size_t altSet = interface[i].num_altsetting;
        auto altsetting = interface[i].altsetting;
        if (altsetting == nullptr) {
            continue;
        }
        for (size_t j = 0; j < altSet; j++) {
            auto endpoints = altsetting[j].endpoint;
            if (endpoints != nullptr) {
                // release endpoint array
                delete[] endpoints;
            }
        }
        delete[] altsetting;
    }

    delete[] interface;
    delete config;
    config = nullptr;
    SANE_HILOG_INFO("%s: end successful", __func__);
    return;
}

void usb_manager_free_device_list(usb_manager_device **list, int unref_devices)
{
    SANE_HILOG_INFO("%s: start", __func__);
    if (list == nullptr) {
        SANE_HILOG_ERROR("%s: list is a nullptr", __func__);
        return;
    }
    MemoryFreeDeviceList(list);
    SANE_HILOG_INFO("%s: end successful", __func__);
}

int usb_manager_set_configuration(usb_manager_device_handle *dev_handle, int configuration)
{
    SANE_HILOG_INFO("%s: start", __func__);
    if (dev_handle == nullptr) {
        SANE_HILOG_ERROR("%s: dev_handle is a nullptr", __func__);
        return USB_MANAGER_ERROR_INVALID_PARAM;
    }
    UsbDevice &device = dev_handle->device;
    USBDevicePipe pip;
    pip.SetBusNum(device.GetBusNum());
    pip.SetDevAddr(device.GetDevAddr());
    USBConfig config;
    config.SetId(configuration);
    auto ret = UsbSrvClient::GetInstance().SetConfiguration(pip, config);
    if (ret != USB_MANAGER_SUCCESS) {
        SANE_HILOG_ERROR("SetConfiguration failed ret=%{public}d", ret);
        return USB_MANAGER_ERROR_IO;
    }
    SANE_HILOG_INFO("%s: end successful", __func__);
    return USB_MANAGER_SUCCESS;
}

int usb_manager_claim_interface(usb_manager_device_handle *dev_handle, int interface_number)
{
    SANE_HILOG_INFO("%s: start", __func__);
    if (dev_handle == nullptr) {
        SANE_HILOG_ERROR("%s: dev_handle is a nullptr", __func__);
        return USB_MANAGER_ERROR_INVALID_PARAM;
    }
    auto &usbSrvClient = UsbSrvClient::GetInstance();
    UsbInterface interface;
    interface.SetId(interface_number);
    auto ret = usbSrvClient.ClaimInterface(dev_handle->pipe, interface, true);
    if (ret != USB_MANAGER_SUCCESS) {
        SANE_HILOG_ERROR("ClaimInterface failed ret=%{public}d", ret);
        return USB_MANAGER_ERROR_IO;
    }
    g_interfaceNumber = interface_number;
    SANE_HILOG_INFO("%s: end successful", __func__);
    return USB_MANAGER_SUCCESS;
}

int usb_manager_release_interface(usb_manager_device_handle *dev_handle, int interface_number)
{
    SANE_HILOG_INFO("%s: start", __func__);
    if (dev_handle == nullptr) {
        SANE_HILOG_ERROR("%s: dev_handle is a nullptr", __func__);
        return USB_MANAGER_ERROR_INVALID_PARAM;
    }
    auto &usbSrvClient = UsbSrvClient::GetInstance();
    USBDevicePipe &pipe = dev_handle->pipe;
    SANE_HILOG_DEBUG("%s: interface_number : %d", __func__, interface_number);
    UsbInterface interface;
    interface.SetId(interface_number);
    auto ret = usbSrvClient.ReleaseInterface(pipe, interface);
    if (ret != USB_MANAGER_SUCCESS) {
        SANE_HILOG_ERROR("ReleaseInterface failed ret=%{public}d", ret);
        return USB_MANAGER_ERROR_IO;
    }
    g_interfaceNumber = 0;
    SANE_HILOG_INFO("%s: end successful", __func__);
    return USB_MANAGER_SUCCESS;
}

int usb_manager_bulk_transfer(usb_manager_device_handle *dev_handle, unsigned char endpoint,
    unsigned char *data, int length, int *transferred, unsigned int timeout)
{
    SANE_HILOG_INFO("%s: start", __func__);
    if (dev_handle == nullptr || data == nullptr || length <= 0 || transferred == nullptr) {
        SANE_HILOG_ERROR("%s: Invalid input parameter.", __func__);
        return USB_MANAGER_ERROR_INVALID_PARAM;
    }
    USBDevicePipe &pipe = dev_handle->pipe;
    USBEndpoint usbEndpoint;
    usbEndpoint.SetInterfaceId(g_interfaceNumber);
    usbEndpoint.SetAddr(endpoint);
    auto &usbSrvClient = UsbSrvClient::GetInstance();
    if (endpoint & USB_MANAGER_ENDPOINT_IN) {
        // read
        std::vector<uint8_t> bufferData(length);
        auto ret = usbSrvClient.BulkTransfer(pipe, usbEndpoint, bufferData, timeout);
        if (ret != USB_MANAGER_SUCCESS) {
            SANE_HILOG_INFO("%s: error", __func__);
            return USB_MANAGER_ERROR_IO;
        }
        for (size_t i = 0; i < bufferData.size(); i++) {
            data[i] = bufferData[i];
        }
    } else {
        // write
        std::vector<uint8_t> bufferData(length);
        for (int i = 0; i < length; i++) {
            bufferData[i] = data[i];
        }
        auto ret = usbSrvClient.BulkTransfer(pipe, usbEndpoint, bufferData, timeout);
        if (ret != USB_MANAGER_SUCCESS) {
            SANE_HILOG_INFO("%s: error", __func__);
            return USB_MANAGER_ERROR_IO;
        }
    }
    *transferred = length;
    SANE_HILOG_INFO("%s: end successful", __func__);
    return USB_MANAGER_SUCCESS;
}

int usb_manager_control_transfer(usb_manager_device_handle *dev_handle, uint8_t request_type,
    uint8_t bRequest, uint16_t wValue, uint16_t wIndex, unsigned char *data, uint16_t wLength,
    unsigned int timeout)
{
    SANE_HILOG_INFO("%s: start", __func__);
    if (dev_handle == nullptr || data == nullptr || wLength == 0) {
        return USB_MANAGER_ERROR_INVALID_PARAM;
    }
    USBDevicePipe &pipe = dev_handle->pipe;
    OHOS::HDI::Usb::V1_0::UsbCtrlTransfer ctrl;
    ctrl.requestType = static_cast<int32_t>(request_type);
    ctrl.requestCmd = static_cast<int32_t>(bRequest);
    ctrl.value = static_cast<int32_t>(wValue);
    ctrl.index = static_cast<int32_t>(wIndex);
    ctrl.timeout = static_cast<int32_t>(timeout);

    auto &usbSrvClient = UsbSrvClient::GetInstance();
    std::vector<uint8_t> bufferData(wLength);
    for (uint16_t i = 0; i < wLength; i++) {
        bufferData[i] = data[i];
    }
    auto ret = usbSrvClient.ControlTransfer(pipe, ctrl, bufferData);
    if (ret != USB_MANAGER_SUCCESS) {
        SANE_HILOG_ERROR("ControlTransfer failed ret=%{public}d", ret);
        return USB_MANAGER_ERROR_IO;
    }
    SANE_HILOG_INFO("%s: end successful", __func__);
    return USB_MANAGER_SUCCESS;
}

int usb_manager_set_interface_alt_setting(usb_manager_device_handle *dev_handle, int interface_number,
    int alternate_setting)
{
    SANE_HILOG_INFO("%s: start", __func__);
    if (dev_handle == nullptr) {
        return USB_MANAGER_ERROR_INVALID_PARAM;
    }
    auto &usbSrvClient = UsbSrvClient::GetInstance();
    USBDevicePipe pipe = dev_handle->pipe;
    UsbInterface interface;
    interface.SetId(interface_number);
    interface.SetAlternateSetting(alternate_setting);
    auto ret = usbSrvClient.SetInterface(pipe, interface);
    if (ret != USB_MANAGER_SUCCESS) {
        SANE_HILOG_ERROR("SetInterface failed ret=%{public}d", ret);
        return USB_MANAGER_ERROR_IO;
    }
    SANE_HILOG_INFO("%s: end successful", __func__);
    return USB_MANAGER_SUCCESS;
}

usb_manager_device* usb_manager_ref_device(usb_manager_device *dev)
{
    SANE_HILOG_INFO("%s: start", __func__);
    if (dev == nullptr) {
        SANE_HILOG_ERROR("%s: dev is a nullptr!", __func__);
        return nullptr;
    }
    usb_manager_device* refDevice = new (std::nothrow) usb_manager_device();
    if (refDevice == nullptr) {
        SANE_HILOG_ERROR("%s: create refDevice error!", __func__);
        return nullptr;
    }
    refDevice->device = dev->device;
    g_refDevices.push_back(refDevice);
    SANE_HILOG_INFO("%s: end successful", __func__);
    return refDevice;
}

// stub
int usb_manager_interrupt_transfer(usb_manager_device_handle *dev_handle, unsigned char endpoint,
    unsigned char *data, int length, int *actual_length, unsigned int timeout)
{
    SANE_HILOG_INFO("%s: start", __func__);
    SANE_HILOG_INFO("%s: end successful", __func__);
    return USB_MANAGER_SUCCESS;
}

// stub
int usb_manager_clear_halt(usb_manager_device_handle *dev_handle, unsigned char endpoint)
{
    SANE_HILOG_INFO("%s: start", __func__);
    SANE_HILOG_INFO("%s: end successful", __func__);
    return USB_MANAGER_SUCCESS;
}

// stub
int usb_manager_reset_device(usb_manager_device_handle *dev_handle)
{
    SANE_HILOG_INFO("%s: start", __func__);
    SANE_HILOG_INFO("%s: end successful", __func__);
    return USB_MANAGER_SUCCESS;
}