/*
 * Copyright (c) 2019
 * IoTech Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include "cc2650.h"

void convert_cc2650_humidity_temperature (void *data, edgex_device_resultvalue *converted_value)
{
  uint16_t raw_data = *(uint16_t *) data;
  converted_value->f32_result = ((double) (int16_t) raw_data / 65536) * 165 - 40;
}

void convert_cc2650_humidity (void *data, edgex_device_resultvalue *converted_value)
{
  uint16_t raw_data = *(uint16_t *) data;
  raw_data &= ~0x0003u;

  converted_value->f32_result = ((float) (double) raw_data / 65536) * 100;
}

void convert_cc2650_barometric_temperature_sensor (void *data, edgex_device_resultvalue *converted_value)
{
  uint32_t raw_data = *(uint32_t *) data;

  raw_data &= 0x00FFFFFFu;

  converted_value->f32_result = raw_data / 100.0f;
}

void convert_cc2650_barometric_sensor (void *data, edgex_device_resultvalue *converted_value)
{
  uint32_t raw_data = *(uint32_t *) data;

  raw_data &= 0xFFFFFF00u;
  raw_data >>= 8u;

  converted_value->f32_result = raw_data / 100.0f;
}

void convert_cc2650_optical_sensor (void *data, edgex_device_resultvalue *converted_value)
{
  uint16_t raw_data = *(uint16_t *) data;
  uint16_t e, m;

  m = raw_data & 0x0FFFu;
  e = (raw_data & 0xF000u) >> 12u;

  e = (e == 0) ? 1 : 2u << (e - 1u);

  converted_value->f32_result = m * (0.01 * e);
}

void convert_cc2650_gyroscope (void *data, edgex_device_resultvalue *converted_value)
{
  int16_t raw_data = *(int16_t *) data;
  converted_value->f32_result = (raw_data * 1.0) / (65536.0 / 500.0);
}

void convert_cc2650_magnetometer (void *data, edgex_device_resultvalue *converted_value)
{
  int16_t raw_data = *(int16_t *) data;
  converted_value->f32_result = raw_data * 1.0;
}

void convert_cc2650_accelerometer_axis (void *data, edgex_device_resultvalue *converted_value)
{
  int16_t raw_data = *(int16_t *) data;
  converted_value->f32_result = (raw_data * 1.0) / (32768.0 / 2);
}