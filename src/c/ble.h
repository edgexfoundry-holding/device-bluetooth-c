/*
 * Copyright (c) 2019
 * IoTech Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#ifndef DEVICE_BLE_C_BLE_H
#define DEVICE_BLE_C_BLE_H

#include <dbus/dbus.h>
#include <pthread.h>
#include <iot/logger.h>

#if defined (__cplusplus)
extern "C" {
#endif

#define DBUS_DEVICE_MAC_LENGTH 22 // strlen ("/dev_XX_XX_XX_XX_XX_XX");

enum ble_discovery_mode
{
  BLE_DISCOVERY_ON,
  BLE_DISCOVERY_OFF
};

int ble_initialize (iot_logger_t *, char *);

void ble_stop (iot_logger_t *);

int ble_discovery (iot_logger_t *, unsigned int discovery_duration);

int ble_is_device_connected (iot_logger_t *, DBusConnection *, char *);

int ble_connect_device (iot_logger_t *, DBusConnection *, char *);

int ble_disconnect_all_devices (iot_logger_t *lc, DBusConnection *);

int ble_read_gatt_characteristic (iot_logger_t *lc, DBusConnection *, char *, char *, void **, unsigned int *);

int
ble_write_gatt_characteristic (iot_logger_t *lc, DBusConnection *, char *, char *, const void *, unsigned int);

#if defined (__cplusplus)
}
#endif

#endif //DEVICE_BLE_C_BLE_H