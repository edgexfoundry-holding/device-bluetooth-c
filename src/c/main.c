/*
 * Copyright (c) 2019
 * IoTech Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */
#include "edgex/devsdk.h"
#include "ble.h"
#include "conversion.h"

#include <signal.h>
#include <semaphore.h>

#define BLE_ERR_CHECK(x) if (x.code) { fprintf (stderr, "Error: %d: %s\n", x.code, x.reason); return x.code; }

static sem_t ble_sem;

typedef struct ble_driver
{
  iot_logger_t *lc;
} ble_driver;

static void ble_signal_handler (int i)
{
  sem_post (&ble_sem);
}

static char *ble_get_protocol_property (const edgex_protocols *protocols, char *protocol_name, char *name)
{
  const edgex_protocols *protocol;
  for (protocol = protocols; protocol; protocol = protocol->next)
  {
    if (strcmp (protocol->name, protocol_name) == 0)
    {
      const edgex_nvpairs *property;
      for (property = protocol->properties; property; property = property->next)
      {
        if (strcmp (property->name, name) == 0)
        {
          return property->value;
        }
      }
    }
  }
  return NULL;
}

static bool ble_init_handler (void *impl, struct iot_logger_t *lc, const edgex_nvpairs *config)
{
  ble_driver *driver = (ble_driver *) impl;
  driver->lc = lc;
  char *ble_interface_name = NULL;
  unsigned int ble_discovery_duration = 10;
  const edgex_nvpairs *current_config = config;

  iot_log_info (driver->lc, "Start Init Process");

  while (current_config != NULL)
  {
    if (strcmp (current_config->name, "BLE_Interface") == 0)
    {
      ble_interface_name = current_config->value;
    }
    else if (strcmp (current_config->name, "BLE_DiscoveryDuration") == 0)
    {
      char *invalid_chars = NULL;
      long int converted_value = strtol (current_config->value, &invalid_chars, 10);

      if (strcmp (invalid_chars, "") != 0)
      {
        iot_log_error (lc, "%s [Driver] option should be a positive number, \"%s\" are invalid characters",
                       current_config->name, current_config->value);
        return false;
      }

      if (converted_value < 0)
      {
        iot_log_error (lc, "%s [Driver] option should be a positive number, \"%s\"", current_config->name,
                       current_config->value);
        return false;
      }

      ble_discovery_duration = converted_value;
    }
    current_config = current_config->next;
  }

  if (ble_interface_name == NULL)
  {
    iot_log_error (driver->lc, "BLE_Interface [Driver] option is missing from config file");
    return false;
  }

  if (ble_initialize (lc, ble_interface_name) == -1)
  {
    return false;
  }

  iot_log_info (driver->lc, "Discovering devices for %d seconds", ble_discovery_duration);
  ble_discovery (driver->lc, ble_discovery_duration);
  iot_log_info (driver->lc, "Finished discovering devices");

  initialize_conversions ();
  iot_log_info (driver->lc, "Init Process Complete");
  return true;
}

static bool is_out_of_bounds (iot_logger_t *lc, unsigned int start_byte, unsigned int type_size,
                              unsigned int byte_array_size)
{
  bool out_of_bounds = false;

  if (start_byte + type_size > byte_array_size)
  {
    iot_log_error (lc, "The value your trying to collect is out of bounds!");
    out_of_bounds = true;
  }

  return out_of_bounds;
}

static bool ble_get_handler (void *impl, const char *devname, const edgex_protocols *protocols, uint32_t nreadings,
                            const edgex_device_commandrequest *requests, edgex_device_commandresult *readings)
{
  bool successful_get_request = true;

  ble_driver *driver = (ble_driver *) impl;
  char *mac_address = ble_get_protocol_property (protocols, "BLE", "MAC");

  if (mac_address == NULL)
  {
    iot_log_error (driver->lc, "%s [DeviceList.Protocols.BLE], MAC option is missing", devname);
    return false;
  }

  /* Create a private connection to use
   * throughout this request. This allows
   * for multiple DBus messages to be sent
   * concurrently, instead of a having to
   * do DBus calls sequentially using the
   * same connection. */

  DBusError dbus_error;
  dbus_error_init (&dbus_error);

  DBusConnection *dbus_conn = dbus_bus_get_private (DBUS_BUS_SYSTEM, &dbus_error);
  if (dbus_error_is_set (&dbus_error))
  {
    iot_log_error (driver->lc, "[D-Bus] %s %s", dbus_error.name, dbus_error.message);
    return false;
  }

  for (uint32_t i = 0; i < nreadings; i++)
  {
    void *byte_array = NULL;
    unsigned int byte_array_size = 0;

    unsigned int start_byte = 0;
    char *raw_type = NULL;
    char *characteristic_uuid = NULL;
    const edgex_nvpairs *attribute = requests[i].attributes;
    while (attribute != NULL)
    {
      if (strcmp (attribute->name, "characteristicUuid") == 0)
      {
        characteristic_uuid = attribute->value;
      }
      else if (strcmp (attribute->name, "rawType") == 0)
      {
        raw_type = attribute->value;
      }
      else if (strcmp (attribute->name, "startByte") == 0)
      {
        char *invalid_chars = NULL;
        long int converted_value = strtol (attribute->value, &invalid_chars, 10);

        if (strcmp (invalid_chars, "") != 0)
        {
          iot_log_error (driver->lc, "%s attribute should be a positive number, \"%s\" are invalid characters",
                         attribute->name, attribute->value);
          return false;
        }

        if (converted_value < 0)
        {
          iot_log_error (driver->lc, "%s attribute should be a positive number, \"%s\"", attribute->name,
                         attribute->value);
          return false;
        }

        start_byte = (unsigned int) converted_value;
      }
      attribute = attribute->next;
    }

    if (characteristic_uuid == NULL)
    {
      iot_log_error (driver->lc, "Profile missing \"characteristicUuid\" attribute in \"%s\" deviceResource",
                     requests[i].resname);
      successful_get_request = false;
      break;
    }

    if (ble_is_device_connected (driver->lc, dbus_conn, mac_address) == false)
    {
      if (ble_connect_device (driver->lc, dbus_conn, mac_address) == -1)
      {
        successful_get_request = false;
        break;
      }
    }

    int successful_read = ble_read_gatt_characteristic (driver->lc, dbus_conn, mac_address, characteristic_uuid,
                                                              &byte_array, &byte_array_size);

    if (successful_read == -1)
    {
      successful_get_request = false;
      break;
    }

    readings[i].type = requests[i].type;

    if (raw_type != NULL)
    {
      void *data = NULL;

      if (start_byte > 0)
      {
        start_byte--;
      }

      uint8_t *offset_byte_array = ((uint8_t *) byte_array + start_byte);

      if (strcmp (raw_type, "Uint8") == 0)
      {
        if (is_out_of_bounds (driver->lc, start_byte, sizeof (uint8_t), byte_array_size) == true)
        {
          successful_get_request = false;
        }
        else
        {
          data = (uint8_t *) offset_byte_array;
        }
      }
      else if (strcmp (raw_type, "Uint16") == 0)
      {
        if (is_out_of_bounds (driver->lc, start_byte, sizeof (uint16_t), byte_array_size) == true)
        {
          successful_get_request = false;
        }
        else
        {
          data = (uint16_t *) offset_byte_array;
        }
      }
      else if (strcmp (raw_type, "Uint32") == 0)
      {
        if (is_out_of_bounds (driver->lc, start_byte, sizeof (uint32_t), byte_array_size) == true)
        {
          successful_get_request = false;
        }
        else
        {
          data = (uint32_t *) offset_byte_array;
        }
      }
      else if (strcmp (raw_type, "Uint64") == 0)
      {
        if (is_out_of_bounds (driver->lc, start_byte, sizeof (uint64_t), byte_array_size) == true)
        {
          successful_get_request = false;
        }
        else
        {
          data = (uint64_t *) offset_byte_array;
        }
      }
      else if (strcmp (raw_type, "Int8") == 0)
      {
        if (is_out_of_bounds (driver->lc, start_byte, sizeof (int8_t), byte_array_size) == true)
        {
          successful_get_request = false;
        }
        else
        {
          data = (int8_t *) offset_byte_array;
        }
      }
      else if (strcmp (raw_type, "Int16") == 0)
      {
        if (is_out_of_bounds (driver->lc, start_byte, sizeof (int16_t), byte_array_size) == true)
        {
          successful_get_request = false;
        }
        else
        {
          data = (int16_t *) offset_byte_array;
        }
      }
      else if (strcmp (raw_type, "Int32") == 0)
      {
        if (is_out_of_bounds (driver->lc, start_byte, sizeof (int32_t), byte_array_size) == true)
        {
          successful_get_request = false;
        }
        else
        {
          data = (int32_t *) offset_byte_array;
        }
      }
      else if (strcmp (raw_type, "Int64") == 0)
      {
        if (is_out_of_bounds (driver->lc, start_byte, sizeof (int64_t), byte_array_size) == true)
        {
          successful_get_request = false;
        }
        else
        {
          data = (int64_t *) offset_byte_array;
        }
      }
      else if (strcmp (raw_type, "Float32") == 0)
      {
        if (is_out_of_bounds (driver->lc, start_byte, sizeof (float), byte_array_size) == true)
        {
          successful_get_request = false;
        }
        else
        {
          data = (float *) offset_byte_array;
        }
      }
      else if (strcmp (raw_type, "Float64") == 0)
      {
        if (is_out_of_bounds (driver->lc, start_byte, sizeof (double), byte_array_size) == true)
        {
          successful_get_request = false;
        }
        else
        {
          data = (double *) offset_byte_array;
        }
      }
      else
      {
          iot_log_error (driver->lc, "raw type attribute \"%s\" currently isn't supported", raw_type);
          successful_get_request = false;
      }

      if (successful_get_request == false)
      {
        free (byte_array);
        break;
      }

      bool successful_convert_value = apply_conversion_to_value (driver->lc, requests[i].resname, data,
                                                                 &readings[i].value);

      if (successful_convert_value == false)
      {
        free (byte_array);
        successful_get_request = false;
        break;
      }
    }
    else
    {
      switch (requests[i].type)
      {
        case Bool:
          readings[i].value.bool_result = *(uint8_t *) byte_array;
          break;

        case Binary:
        {
          uint8_t *binary_value = calloc (byte_array_size, 1);
          memcpy (binary_value, (uint8_t *) byte_array, byte_array_size);

          readings[i].value.binary_result.size = byte_array_size;
          readings[i].value.binary_result.bytes = binary_value;
          break;
        }
        case String:
        {
          char *string_value = byte_array;

          if (string_value[byte_array_size - 1] != '\0')
          {
            unsigned int string_length = byte_array_size + 1;

            string_value = calloc (string_length, 1);
            memcpy (string_value, (char *) byte_array, byte_array_size);
          }
          else
          {
            string_value = strdup (string_value);
          }

          readings[i].value.string_result = string_value;
          break;
        }
        case Uint8:
          readings[i].value.ui8_result = *(uint8_t *) byte_array;
          break;

        case Uint16:
          readings[i].value.ui16_result = *(uint16_t *) byte_array;
          break;

        case Uint32:
          readings[i].value.ui32_result = *(uint32_t *) byte_array;
          break;

        case Uint64:
          readings[i].value.ui64_result = *(uint64_t *) byte_array;
          break;

        case Int8:
          readings[i].value.i8_result = *(int8_t *) byte_array;
          break;

        case Int16:
          readings[i].value.i16_result = *(int16_t *) byte_array;
          break;

        case Int32:
          readings[i].value.i32_result = *(int32_t *) byte_array;
          break;

        case Int64:
          readings[i].value.i64_result = *(int64_t *) byte_array;
          break;

        case Float32:
          readings[i].value.f32_result = *(float *) byte_array;
          break;

        case Float64:
          readings[i].value.f64_result = *(double *) byte_array;
          break;

        default:
          iot_log_error (driver->lc, "%s value type specified in the device profile isn't supported",
                         requests[i].resname);
          successful_get_request = false;
          break;
      }
    }
    free (byte_array);
  }
  /* The DBus connection has to be closed
   * and unreferenced as it is a private
   * connection */
  dbus_connection_close (dbus_conn);
  dbus_connection_unref (dbus_conn);
  return successful_get_request;
}

static bool ble_put_handler (void *impl, const char *devname, const edgex_protocols *protocols, uint32_t nvalues,
                            const edgex_device_commandrequest *requests, const edgex_device_commandresult *values)
{
  ble_driver *driver = (ble_driver *) impl;
  char *mac_address = ble_get_protocol_property (protocols, "BLE", "MAC");
  bool successful_put_request = true;

  if (mac_address == NULL)
  {
    iot_log_error (driver->lc, "%s [DeviceList.Protocols.BLE], MAC option is missing", devname);
    return false;
  }

  /* Create a private connection to use
   * throughout this request. This allows
   * for multiple DBus messages to be sent
   * concurrently, instead of a having to
   * do DBus calls sequentially using the
   * same connection. */

  DBusError dbus_error;
  dbus_error_init (&dbus_error);

  DBusConnection *dbus_conn = dbus_bus_get_private (DBUS_BUS_SYSTEM, &dbus_error);
  if (dbus_error_is_set (&dbus_error))
  {
    iot_log_error (driver->lc, "[D-Bus] %s %s", dbus_error.name, dbus_error.message);
    return false;
  }

  for (uint32_t i = 0; i < nvalues; i++)
  {
    unsigned int bit_shift = 0;
    char *raw_type = NULL;
    char *characteristic_uuid = NULL;
    const edgex_nvpairs *attribute = requests[i].attributes;
    while (attribute != NULL)
    {
      if (strcmp (attribute->name, "characteristicUuid") == 0)
      {
        characteristic_uuid = attribute->value;
      }
      else if (strcmp (attribute->name, "rawType") == 0)
      {
        raw_type = attribute->value;
      }
      else if (strcmp (attribute->name, "bitShift") == 0)
      {
        char *invalid_chars = NULL;
        long int converted_value = strtol (attribute->value, &invalid_chars, 10);

        if (strcmp (invalid_chars, "") != 0)
        {
          iot_log_error (driver->lc, "%s attribute should be a positive number, \"%s\" are invalid characters",
                         attribute->name, attribute->value);
          return false;
        }

        if (converted_value < 0)
        {
          iot_log_error (driver->lc, "%s attribute should be a positive number, \"%s\"", attribute->name,
                         attribute->value);
          return false;
        }

        bit_shift = (unsigned int) converted_value;
      }
      attribute = attribute->next;
    }

    if (characteristic_uuid == NULL)
    {
      iot_log_error (driver->lc, "Profile missing \"characteristicUuid\" attribute in \"%s\" deviceResource",
                     requests[i].resname);
      successful_put_request = false;
      break;
    }

    if (ble_is_device_connected (driver->lc, dbus_conn, mac_address) == false)
    {
      if (ble_connect_device (driver->lc, dbus_conn, mac_address) == -1)
      {
        successful_put_request = false;
        break;
      }
    }

    int successful_write = 0;

    if (requests[i].type == Bool && raw_type != NULL)
    {
      if (strcmp (raw_type, "Uint8") == 0)
      {
        uint8_t data = (uint8_t) values[i].value.bool_result;
        data <<= bit_shift;

        successful_write = ble_write_gatt_characteristic (driver->lc, dbus_conn, mac_address, characteristic_uuid,
                                                                &data, sizeof (uint8_t));
      }
      else if (strcmp (raw_type, "Uint16") == 0)
      {
        uint16_t data = (uint16_t) values[i].value.bool_result;
        data <<= bit_shift;

        successful_write = ble_write_gatt_characteristic (driver->lc, dbus_conn, mac_address, characteristic_uuid,
                                                                &data, sizeof (uint16_t));
      }
      else if (strcmp (raw_type, "Uint32") == 0)
      {
        uint32_t data = (uint32_t) values[i].value.bool_result;
        data <<= bit_shift;

        successful_write = ble_write_gatt_characteristic (driver->lc, dbus_conn, mac_address, characteristic_uuid,
                                                                &data, sizeof (uint32_t));
      }
      else if (strcmp (raw_type, "Uint64") == 0)
      {
        uint64_t data = (uint64_t) values[i].value.bool_result;
        data <<= bit_shift;

        successful_write = ble_write_gatt_characteristic (driver->lc, dbus_conn, mac_address, characteristic_uuid,
                                                                &data, sizeof (uint64_t));
      }
      else
      {
        iot_log_error (driver->lc, "%s raw_type attribute \"%s\" isn't supported", requests[i].resname, raw_type);
      }
    }
    else
    {
      switch (requests[i].type)
      {
        case Bool:
        {
          bool bool_value = values[i].value.bool_result;
          successful_write = ble_write_gatt_characteristic (driver->lc, dbus_conn, mac_address,
                                                                  characteristic_uuid,
                                                                  &bool_value, sizeof (bool));
          break;
        }
        case Binary:
          successful_write = ble_write_gatt_characteristic (driver->lc, dbus_conn, mac_address,
                                                                  characteristic_uuid,
                                                                  values[i].value.binary_result.bytes,
                                                                  values[i].value.binary_result.size);
          break;

        case String:
        {
          char *string_value = values[i].value.string_result;
          successful_write = ble_write_gatt_characteristic (driver->lc, dbus_conn, mac_address,
                                                                  characteristic_uuid,
                                                                  string_value,
                                                                  (strlen (string_value) + 1) * sizeof (char));
          break;
        }
        case Uint8:
        {
          uint8_t uint8_value = values[i].value.ui8_result;
          successful_write = ble_write_gatt_characteristic (driver->lc, dbus_conn, mac_address,
                                                                  characteristic_uuid,
                                                                  &uint8_value, sizeof (uint8_t));
          break;
        }
        case Uint16:
        {
          uint16_t uint16_value = values[i].value.ui16_result;
          successful_write = ble_write_gatt_characteristic (driver->lc, dbus_conn, mac_address,
                                                                  characteristic_uuid,
                                                                  &uint16_value, sizeof (uint16_t));
          break;
        }
        case Uint32:
        {
          uint32_t uint32_value = values[i].value.ui32_result;
          successful_write = ble_write_gatt_characteristic (driver->lc, dbus_conn, mac_address,
                                                                  characteristic_uuid,
                                                                  &uint32_value, sizeof (uint32_t));
          break;
        }
        case Uint64:
        {
          uint64_t uint64_value = values[i].value.ui64_result;
          successful_write = ble_write_gatt_characteristic (driver->lc, dbus_conn, mac_address,
                                                                  characteristic_uuid,
                                                                  &uint64_value, sizeof (uint64_t));
          break;
        }
        case Int8:
        {
          int8_t int8_value = values[i].value.i8_result;
          successful_write = ble_write_gatt_characteristic (driver->lc, dbus_conn, mac_address,
                                                                  characteristic_uuid,
                                                                  &int8_value, sizeof (int8_t));
          break;
        }
        case Int16:
        {
          int16_t int16_value = values[i].value.i16_result;
          successful_write = ble_write_gatt_characteristic (driver->lc, dbus_conn, mac_address,
                                                                  characteristic_uuid,
                                                                  &int16_value, sizeof (int16_t));
          break;
        }
        case Int32:
        {
          int32_t int32_value = values[i].value.i32_result;
          successful_write = ble_write_gatt_characteristic (driver->lc, dbus_conn, mac_address,
                                                                  characteristic_uuid,
                                                                  &int32_value, sizeof (int32_t));
          break;
        }
        case Int64:
        {
          int64_t int64_value = values[i].value.i64_result;
          successful_write = ble_write_gatt_characteristic (driver->lc, dbus_conn, mac_address,
                                                                  characteristic_uuid,
                                                                  &int64_value, sizeof (int64_t));
          break;
        }
        case Float32:
        {
          float float_value = values[i].value.f32_result;
          successful_write = ble_write_gatt_characteristic (driver->lc, dbus_conn, mac_address,
                                                                  characteristic_uuid,
                                                                  &float_value, sizeof (float));
          break;
        }
        case Float64:
        {
          double double_value = values[i].value.f64_result;
          successful_write = ble_write_gatt_characteristic (driver->lc, dbus_conn, mac_address,
                                                                  characteristic_uuid,
                                                                  &double_value, sizeof (double));
          break;
        }
        default:
          iot_log_error (driver->lc, "%s value type specified in the device profile isn't supported",
                         requests[i].resname);
          successful_write = -1;
          break;
      }
    }

    if (successful_write == -1)
    {
      successful_put_request = false;
      break;
    }
  }

  /* The DBus connection has to be closed and unreferenced as it is a private connection */
  dbus_connection_close (dbus_conn);
  dbus_connection_unref (dbus_conn);
  return successful_put_request;
}

static bool ble_disconnect_handler (void *impl, edgex_protocols *device)
{
  return true;
}

static void ble_stop_handler (void *impl, bool force)
{
  ble_driver *driver = (ble_driver *) impl;
  ble_stop (driver->lc);
  free_conversions ();
}

int main (int argc, char *argv[])
{
  edgex_device_svcparams params = {"device-ble",
                                   NULL,
                                   NULL,
                                   NULL};

  if (!edgex_device_service_processparams (&argc, argv, &params))
  {
      return false;
  }

  int n = 1;
  while (n < argc)
  {
      if (strcmp (argv[n], "-h") == 0 || strcmp (argv[n], "--help") == 0)
      {
          printf ("Options:\n");
          printf ("  -h, --help\t\t: Show this text\n");
          edgex_device_service_usage ();
          return false;

      }else{
          printf ("%s: Unrecognized option %s\n", argv[0], argv[n]);
          return false;
      }
  }

  ble_driver *impl = malloc (sizeof (ble_driver));
  memset (impl, 0, sizeof (ble_driver));

  edgex_error e;
  e.code = 0;

  edgex_device_callbacks ble_impls =
    {
      ble_init_handler,
      NULL,
      ble_get_handler,
      ble_put_handler,
      ble_disconnect_handler,
      ble_stop_handler
    };

  edgex_device_service *service = edgex_device_service_new (params.svcname, VERSION, impl, ble_impls, &e);
  BLE_ERR_CHECK (e)

  edgex_device_service_start (service, params.regURL, params.profile, params.confdir, &e);
  BLE_ERR_CHECK (e)

  signal (SIGINT, ble_signal_handler);
  signal (SIGTERM, ble_signal_handler);

  sem_init (&ble_sem, 0, 0);
  sem_wait (&ble_sem);

  edgex_device_service_stop (service, true, &e);
  BLE_ERR_CHECK (e)

  sem_destroy (&ble_sem);
  edgex_device_service_free (service);
  free (impl);
  return 0;
}
