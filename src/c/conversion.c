/*
 * Copyright (c) 2019
 * IoTech Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include "conversion.h"

struct conversion
{
  char *conversion_name;
  unsigned long int conversion_hash;

  void (*conversion_function) (void *, edgex_device_resultvalue *);

  struct conversion *next_conversion;
};

static struct conversion *head_conversion_node = NULL;

static unsigned long int djb2_hash (const char *string)
{
  unsigned long int hash = 5381;
  int character;

  while ((character = *string++))
  {
    hash = ((hash << 5u) + hash) + character;
  }

  return hash;
}

void initialize_conversions ()
{
  register_conversion ("CC2650TemperatureData", convert_cc2650_humidity_temperature);
  register_conversion ("CC2650HumidityData", convert_cc2650_humidity);
  register_conversion ("CC2650BarometricTemperatureData", convert_cc2650_barometric_temperature_sensor);
  register_conversion ("CC2650BarometricData", convert_cc2650_barometric_sensor);
  register_conversion ("CC2650OpticalSensorData", convert_cc2650_optical_sensor);
  register_conversion ("CC2650GyroscopeXData", convert_cc2650_gyroscope);
  register_conversion ("CC2650GyroscopeYData", convert_cc2650_gyroscope);
  register_conversion ("CC2650GyroscopeZData", convert_cc2650_gyroscope);
  register_conversion ("CC2650AccelerometerXData", convert_cc2650_accelerometer_axis);
  register_conversion ("CC2650AccelerometerYData", convert_cc2650_accelerometer_axis);
  register_conversion ("CC2650AccelerometerZData", convert_cc2650_accelerometer_axis);
  register_conversion ("CC2650MagnetometerXData", convert_cc2650_magnetometer);
  register_conversion ("CC2650MagnetometerYData", convert_cc2650_magnetometer);
  register_conversion ("CC2650MagnetometerZData", convert_cc2650_magnetometer);
}

void register_conversion (const char *conversion_name,
                          void (*conversion_function) (void *, edgex_device_resultvalue *))
{
  struct conversion *new_conversion_node = malloc (sizeof (struct conversion));

  new_conversion_node->conversion_name = malloc (strlen (conversion_name) + 1);
  new_conversion_node->conversion_function = conversion_function;
  new_conversion_node->conversion_hash = djb2_hash (conversion_name);

  strcpy (new_conversion_node->conversion_name, conversion_name);

  new_conversion_node->next_conversion = head_conversion_node;
  head_conversion_node = new_conversion_node;
}

bool apply_conversion_to_value (iot_logger_t *lc, const char *conversion_name, void *data,
                                edgex_device_resultvalue *reading)
{
  bool found_conversion = false;
  struct conversion *conversion_node = head_conversion_node;

  while (conversion_node != NULL)
  {
    if (djb2_hash (conversion_name) == conversion_node->conversion_hash &&
        strcmp (conversion_node->conversion_name, conversion_name) == 0)
    {
      found_conversion = true;
      conversion_node->conversion_function (data, reading);
      break;
    }
    conversion_node = conversion_node->next_conversion;
  }

  if (found_conversion == false)
  {
    iot_log_error (lc, "Unable to convert %s value as there isn't a conversion function", conversion_name);
  }

  return found_conversion;
}

void free_conversions ()
{
  struct conversion *prev_conversion_node = NULL;

  while (head_conversion_node != NULL)
  {
    prev_conversion_node = head_conversion_node;
    head_conversion_node = head_conversion_node->next_conversion;

    free (prev_conversion_node->conversion_name);
    free (prev_conversion_node);
  }
}