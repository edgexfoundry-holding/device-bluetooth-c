/*
 * Copyright (c) 2019
 * IoTech Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#ifndef DEVICE_BLUETOOTH_C_CC2650_H
#define DEVICE_BLUETOOTH_C_CC2650_H

#include <edgex/edgex-base.h>

void convert_cc2650_humidity_temperature (void *, edgex_device_resultvalue *);

void convert_cc2650_humidity (void *, edgex_device_resultvalue *);

void convert_cc2650_barometric_temperature_sensor (void *, edgex_device_resultvalue *);

void convert_cc2650_barometric_sensor (void *, edgex_device_resultvalue *);

void convert_cc2650_optical_sensor (void *, edgex_device_resultvalue *);

void convert_cc2650_gyroscope (void *, edgex_device_resultvalue *);

void convert_cc2650_magnetometer (void *, edgex_device_resultvalue *);

void convert_cc2650_accelerometer_axis (void *, edgex_device_resultvalue *);

#endif //DEVICE_BLUETOOTH_C_CC2650_H
