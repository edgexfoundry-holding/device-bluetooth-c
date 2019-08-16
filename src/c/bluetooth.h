/*
 * Copyright (c) 2019
 * IoTech Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#ifndef DEVICE_BLUETOOTH_C_BLUETOOTH_H
#define DEVICE_BLUETOOTH_C_BLUETOOTH_H

#include <dbus/dbus.h>
#include <pthread.h>
#include <iot/logger.h>

#if defined (__cplusplus)
extern "C" {
#endif

#define DBUS_DEVICE_MAC_LENGTH 22 // strlen ("/dev_XX_XX_XX_XX_XX_XX");

enum bluetooth_discovery_mode
{
  BLUETOOTH_DISCOVERY_ON,
  BLUETOOTH_DISCOVERY_OFF
};

int bluetooth_initialize (iot_logger_t *, char *);

void bluetooth_stop (iot_logger_t *);

int bluetooth_set_discovery_mode (iot_logger_t *, enum bluetooth_discovery_mode);

int bluetooth_is_device_connected (iot_logger_t *, DBusConnection *, char *);

int bluetooth_connect_device (iot_logger_t *, DBusConnection *, char *);

int bluetooth_disconnect_all_devices (iot_logger_t *lc, DBusConnection *);

int bluetooth_read_gatt_characteristic (iot_logger_t *lc, DBusConnection *, char *, char *, void **, unsigned int *);

int
bluetooth_write_gatt_characteristic (iot_logger_t *lc, DBusConnection *, char *, char *, const void *, unsigned int);

#if defined (__cplusplus)
}
#endif

#endif //DEVICE_BLUETOOTH_C_BLUETOOTH_H