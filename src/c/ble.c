/*
 * Copyright (c) 2019
 * IoTech Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include "ble.h"

static char *ble_dbus_base_path;
static unsigned int ble_dbus_base_path_length;

static void replace_char (char *dst, char find, char replace)
{
  char *current_char = strchr (dst, find);
  while (current_char)
  {
    *current_char = replace;
    current_char = strchr (current_char, find);
  }
}

static DBusMessage *dbus_get_property (iot_logger_t *lc, DBusConnection *dbus_conn, void *dst, char *dbus_path,
                                       char *interface, char *property_name)
{
  DBusError dbus_error;
  dbus_error_init (&dbus_error);

  DBusMessage *dbus_msg = dbus_message_new_method_call ("org.bluez", dbus_path, "org.freedesktop.DBus.Properties",
                                                        "Get");

  DBusMessageIter dbus_msg_args;
  dbus_message_iter_init_append (dbus_msg, &dbus_msg_args);
  dbus_message_iter_append_basic (&dbus_msg_args, DBUS_TYPE_STRING, &interface);
  dbus_message_iter_append_basic (&dbus_msg_args, DBUS_TYPE_STRING, &property_name);

  DBusMessage *dbus_reply = dbus_connection_send_with_reply_and_block (dbus_conn, dbus_msg, DBUS_TIMEOUT_USE_DEFAULT,
                                                                       &dbus_error);
  dbus_message_unref (dbus_msg);

  if (dbus_error_is_set (&dbus_error))
  {
    iot_log_error (lc, "[D-Bus] %s %s", dbus_error.name, dbus_error.message);
    return NULL;
  }
  DBusMessageIter dbus_reply_iterator, dbus_reply_iterator_value;
  dbus_message_iter_init (dbus_reply, &dbus_reply_iterator);

  dbus_message_iter_recurse (&dbus_reply_iterator, &dbus_reply_iterator_value);
  dbus_message_iter_get_basic (&dbus_reply_iterator_value, dst);

  return dbus_reply;
}

int ble_initialize (iot_logger_t *lc, char *interface_name)
{
  iot_log_info (lc, "Using \"%s\" as ble interface", interface_name);

  DBusError dbus_error;
  dbus_error_init (&dbus_error);

  if (dbus_error_is_set (&dbus_error))
  {
    iot_log_error (lc, "[D-Bus] %s %s", dbus_error.name, dbus_error.message);
    return -1;
  }

  ble_dbus_base_path_length = strlen ("/org/bluez/") + strlen (interface_name) + 1;
  ble_dbus_base_path = calloc (ble_dbus_base_path_length, sizeof (char));
  strcpy (ble_dbus_base_path, "/org/bluez/");
  strcat (ble_dbus_base_path, interface_name);

  /* Setup private DBus connection */
  DBusConnection *dbus_conn = dbus_bus_get_private (DBUS_BUS_SYSTEM, &dbus_error);
  if (dbus_error_is_set (&dbus_error))
  {
    iot_log_error (lc, "[D-Bus] %s %s", dbus_error.name, dbus_error.message);
    return -1;
  }

  int successful_init = 0;
  if (ble_disconnect_all_devices (lc, dbus_conn) == -1)
  {
    successful_init = -1;
  }

  /* Close and unref the private DBus connection */
  dbus_connection_close (dbus_conn);
  dbus_connection_unref (dbus_conn);
  return successful_init;
}

void ble_stop (iot_logger_t *lc)
{
  DBusError dbus_error;
  dbus_error_init (&dbus_error);

  /* Setup a private DBus connection to
   * be used for disconnecting the devices */
  DBusConnection *dbus_conn = dbus_bus_get_private (DBUS_BUS_SYSTEM, &dbus_error);
  if (dbus_error_is_set (&dbus_error))
  {
    iot_log_error (lc, "[D-Bus] %s %s", dbus_error.name, dbus_error.message);
    return;
  }
  if (ble_disconnect_all_devices (lc, dbus_conn) == -1)
  {
    iot_log_error (lc, "Couldn't disconnect all devices");
  }
  free (ble_dbus_base_path);

  /* Close and unref the private DBus connection */
  dbus_connection_close (dbus_conn);
  dbus_connection_unref (dbus_conn);
}

int ble_set_discovery_mode (iot_logger_t *lc, enum ble_discovery_mode discovery_mode)
{
  DBusError dbus_error;
  dbus_error_init (&dbus_error);

  char dbus_cmd[] = "StartDiscovery";

  if (discovery_mode == ble_DISCOVERY_OFF)
  {
    strcpy (dbus_cmd, "StopDiscovery");
  }

  DBusMessage *dbus_msg = dbus_message_new_method_call ("org.bluez", ble_dbus_base_path, "org.bluez.Adapter1",
                                                        dbus_cmd);
  if (dbus_msg == NULL)
  {
    iot_log_error (lc, "[D-Bus] Couldn't setup bus for %s", dbus_cmd);
    return -1;
  }

  DBusConnection *dbus_conn = dbus_bus_get (DBUS_BUS_SYSTEM, &dbus_error);
  if (dbus_error_is_set (&dbus_error))
  {
    iot_log_error (lc, "[D-Bus] %s %s", dbus_error.name, dbus_error.message);
  }

  DBusMessage *dbus_reply = dbus_connection_send_with_reply_and_block (dbus_conn, dbus_msg, DBUS_TIMEOUT_USE_DEFAULT,
                                                                       &dbus_error);
  dbus_message_unref (dbus_msg);

  if (dbus_error_is_set (&dbus_error))
  {
    iot_log_error (lc, "[D-Bus] %s %s", dbus_error.name, dbus_error.message);
    return -1;
  }

  if (dbus_reply == NULL)
  {
    iot_log_error (lc, "Failed to StartDiscovery");
    return -1;
  }

  dbus_message_unref (dbus_reply);
  return 0;
}

int ble_is_device_connected (iot_logger_t *lc, DBusConnection *dbus_conn, char *mac)
{
  dbus_bool_t returned_bool;
  unsigned int dbus_path_length = ble_dbus_base_path_length + DBUS_DEVICE_MAC_LENGTH + 1;
  char mac_formatted[strlen (mac) + 1];
  char dbus_path[dbus_path_length];

  strcpy (mac_formatted, mac);
  replace_char (mac_formatted, ':', '_');
  snprintf (dbus_path, dbus_path_length, "%s/dev_%s", ble_dbus_base_path, mac_formatted);

  DBusMessage *dbus_property_reply = dbus_get_property (lc, dbus_conn, &returned_bool, dbus_path, "org.bluez.Device1",
                                                        "Connected");
  if (dbus_property_reply == NULL)
  {
    return 0;
  }
  dbus_message_unref (dbus_property_reply);

  return (int) returned_bool;
}

int ble_connect_device (iot_logger_t *lc, DBusConnection *dbus_conn, char *mac)
{
  DBusError dbus_error;
  dbus_error_init (&dbus_error);

  unsigned int dbus_path_length = ble_dbus_base_path_length + DBUS_DEVICE_MAC_LENGTH + 1;
  char mac_formatted[strlen (mac) + 1];
  char dbus_path[dbus_path_length];

  strcpy (mac_formatted, mac);
  replace_char (mac_formatted, ':', '_');
  snprintf (dbus_path, dbus_path_length, "%s/dev_%s", ble_dbus_base_path, mac_formatted);

  DBusMessage *dbus_msg = dbus_message_new_method_call ("org.bluez", dbus_path, "org.bluez.Device1", "Connect");
  if (dbus_msg == NULL)
  {
    iot_log_error (lc, "[D-Bus] Couldn't setup bus for Connect");
    return -1;
  }

  DBusMessage *dbus_reply = dbus_connection_send_with_reply_and_block (dbus_conn, dbus_msg, DBUS_TIMEOUT_USE_DEFAULT,
                                                                       &dbus_error);
  dbus_message_unref (dbus_msg);
  if (dbus_error_is_set (&dbus_error))
  {
    iot_log_error (lc, "[D-Bus] %s %s", dbus_error.name, dbus_error.message);
    return -1;
  }

  if (dbus_reply == NULL)
  {
    iot_log_error (lc, "Failed to connect to device %s", mac);
    return -1;
  }
  /* Wait until D-Bus has appended all the
     characteristic paths for the target
     device before trying to read/write. Not
     waiting will cause a path not found error */
  sleep (2);

  iot_log_info (lc, "Connected to device %s", mac);
  dbus_message_unref (dbus_reply);
  return 0;
}

static int ble_disconnect_device (iot_logger_t *lc, DBusConnection *dbus_conn, char *dbus_device_path)
{
  DBusError dbus_error;
  dbus_error_init (&dbus_error);

  DBusMessage *dbus_msg = dbus_message_new_method_call ("org.bluez", dbus_device_path, "org.bluez.Device1",
                                                        "Disconnect");
  if (dbus_msg == NULL)
  {
    iot_log_error (lc, "[D-Bus] Couldn't setup message for Disconnect");
    return -1;
  }

  DBusMessage *dbus_reply = dbus_connection_send_with_reply_and_block (dbus_conn, dbus_msg, DBUS_TIMEOUT_USE_DEFAULT,
                                                                       &dbus_error);
  dbus_message_unref (dbus_msg);
  if (dbus_error_is_set (&dbus_error))
  {
    iot_log_error (lc, "[D-Bus] %s %s", dbus_error.name, dbus_error.message);
    return -1;
  }

  if (dbus_reply == NULL)
  {
    iot_log_error (lc, "[D-Bus] Failed to connect to device %s", dbus_device_path);
    return -1;
  }

  iot_log_info (lc, "Disconnected %s", dbus_device_path);
  dbus_message_unref (dbus_reply);
  return 0;
}

int ble_disconnect_all_devices (iot_logger_t *lc, DBusConnection *dbus_conn)
{
  DBusError dbus_error;
  dbus_error_init (&dbus_error);
  DBusMessage *dbus_msg = dbus_message_new_method_call ("org.bluez", "/", "org.freedesktop.DBus.ObjectManager",
                                                        "GetManagedObjects");

  if (dbus_msg == NULL)
  {
    iot_log_error (lc, "[D-Bus] Couldn't setup bus for GetManagedObjects");
    return -1;
  }

  DBusMessage *dbus_reply = dbus_connection_send_with_reply_and_block (dbus_conn, dbus_msg, DBUS_TIMEOUT_USE_DEFAULT,
                                                                       &dbus_error);
  dbus_message_unref (dbus_msg);

  if (dbus_error_is_set (&dbus_error))
  {
    iot_log_error (lc, "[D-Bus] %s %s", dbus_error.name, dbus_error.message);
    return -1;
  }

  char *dbus_object_path = NULL;
  DBusMessageIter dbus_paths_list, dbus_path_iter, dictIter;

  dbus_message_iter_init (dbus_reply, &dbus_paths_list);
  dbus_message_iter_recurse (&dbus_paths_list, &dbus_path_iter);

  while (dbus_message_iter_has_next (&dbus_path_iter))
  {
    dbus_message_iter_next (&dbus_path_iter);
    dbus_message_iter_recurse (&dbus_path_iter, &dictIter);
    dbus_message_iter_get_basic (&dictIter, &dbus_object_path);

    if (strcmp (dbus_object_path, ble_dbus_base_path) == 0)
    {
      continue;
    }

    if (strlen (dbus_object_path) != strlen (ble_dbus_base_path) + DBUS_DEVICE_MAC_LENGTH)
    {
      continue;
    }

    dbus_bool_t is_device_connected;
    DBusMessage *dbus_property_reply = dbus_get_property (lc, dbus_conn, &is_device_connected, dbus_object_path,
                                                          "org.bluez.Device1", "Connected");

    if (dbus_property_reply == NULL)
    {
      return -1;
    }
    dbus_message_unref (dbus_property_reply);

    if (is_device_connected)
    {
      ble_disconnect_device (lc, dbus_conn, dbus_object_path);
    }
  }

  dbus_message_unref (dbus_reply);
  return 0;
}

static char *get_gatt_characteristic_dbus_path (iot_logger_t *lc, DBusConnection *dbus_conn, char *mac,
                                                char *characteristic_uuid)
{
  DBusError dbus_error;
  dbus_error_init (&dbus_error);
  DBusMessage *dbus_msg = dbus_message_new_method_call ("org.bluez", "/", "org.freedesktop.DBus.ObjectManager",
                                                        "GetManagedObjects");

  if (dbus_msg == NULL)
  {
    iot_log_error (lc, "[D-Bus] Couldn't setup bus message for GetManagedObjects");
    return NULL;
  }

  DBusMessage *dbus_reply = dbus_connection_send_with_reply_and_block (dbus_conn, dbus_msg, DBUS_TIMEOUT_USE_DEFAULT,
                                                                       &dbus_error);
  dbus_message_unref (dbus_msg);

  if (dbus_error_is_set (&dbus_error))
  {
    iot_log_error (lc, "[D-Bus] %s %s", dbus_error.name, dbus_error.message);
    return NULL;
  }

  char *dbus_object_path = NULL;
  DBusMessageIter dbus_paths_list, dbus_path_iter, dbus_dictionary_iter;

  dbus_message_iter_init (dbus_reply, &dbus_paths_list);
  dbus_message_iter_recurse (&dbus_paths_list, &dbus_path_iter);

  bool found_correct_uuid = false;

  do
  {
    dbus_message_iter_next (&dbus_path_iter);
    dbus_message_iter_recurse (&dbus_path_iter, &dbus_dictionary_iter);
    dbus_message_iter_get_basic (&dbus_dictionary_iter, &dbus_object_path);

    //Skip paths that don't include target device mac address
    if (strstr (dbus_object_path, mac) == NULL)
    {
      continue;
    }

    //Skip non GattCharacteristic paths e.g Gatt services & descriptors
    if (strstr (dbus_object_path, "char") == NULL)
    {
      continue;
    }

    //Skip GattDescriptors paths
    if (strstr (dbus_object_path, "desc") != NULL)
    {
      continue;
    }

    char *returned_uuid = NULL;
    DBusMessage *dbus_property_reply = dbus_get_property (lc, dbus_conn, &returned_uuid, dbus_object_path,
                                                          "org.bluez.GattCharacteristic1", "UUID");

    if (dbus_property_reply == NULL)
    {
      continue;
    }

    if (returned_uuid)
    {
      if (strcmp (returned_uuid, characteristic_uuid) == 0)
      {
        found_correct_uuid = true;
      }
    }
    dbus_message_unref (dbus_property_reply);
  } while (dbus_message_iter_has_next (&dbus_path_iter) && found_correct_uuid == false);

  if (found_correct_uuid == true)
  {
    dbus_object_path = strdup (dbus_object_path);
  }
  else
  {
    dbus_object_path = NULL;
  }
  dbus_message_unref (dbus_reply);

  return dbus_object_path;
}

int ble_read_gatt_characteristic (iot_logger_t *lc, DBusConnection *dbus_conn, char *mac,
                                        char *characteristic_uuid, void **byte_array_dst,
                                        unsigned int *byte_array_dst_size)
{
  char mac_formatted[strlen (mac) + 1];

  strcpy (mac_formatted, mac);
  replace_char (mac_formatted, ':', '_');

  char *gatt_characteristic_dbus_path = get_gatt_characteristic_dbus_path (lc, dbus_conn, mac_formatted,
                                                                           characteristic_uuid);

  if (gatt_characteristic_dbus_path == NULL)
  {
    return -1;
  }
  DBusMessage *dbus_msg = dbus_message_new_method_call ("org.bluez", gatt_characteristic_dbus_path,
                                                        "org.bluez.GattCharacteristic1", "ReadValue");

  if (dbus_msg == NULL)
  {
    iot_log_error (lc, "[D-Bus] Couldn't setup bus for ReadValue");
    return -1;
  }

  DBusMessageIter dbus_msg_args, dbus_dictionary_iter;
  dbus_message_iter_init_append (dbus_msg, &dbus_msg_args);
  dbus_message_iter_open_container (&dbus_msg_args, DBUS_TYPE_ARRAY,
                                    DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING
                                    DBUS_TYPE_STRING_AS_STRING
                                    DBUS_TYPE_VARIANT_AS_STRING
                                    DBUS_DICT_ENTRY_END_CHAR_AS_STRING, &dbus_dictionary_iter);
  dbus_message_iter_close_container (&dbus_msg_args, &dbus_dictionary_iter);

  DBusError dbus_error;
  dbus_error_init (&dbus_error);

  DBusMessage *dbus_reply = dbus_connection_send_with_reply_and_block (dbus_conn, dbus_msg, DBUS_TIMEOUT_USE_DEFAULT,
                                                                       &dbus_error);
  dbus_message_unref (dbus_msg);
  free (gatt_characteristic_dbus_path);

  if (dbus_error_is_set (&dbus_error))
  {
    iot_log_error (lc, "[D-Bus] %s %s", dbus_error.name, dbus_error.message);
    return -1;
  }

  uint8_t *byte_array = NULL;
  unsigned int byte_array_size = 0;

  dbus_message_get_args (dbus_reply, &dbus_error, DBUS_TYPE_ARRAY, DBUS_TYPE_BYTE, &byte_array, &byte_array_size,
                         DBUS_TYPE_INVALID);

  if (dbus_error_is_set (&dbus_error))
  {
    iot_log_error (lc, "[D-Bus] %s %s", dbus_error.name, dbus_error.message);
    return -1;
  }

  if (byte_array_size == 0)
  {
    return -1;
  }

  *byte_array_dst_size = byte_array_size;
  *byte_array_dst = malloc (sizeof (uint8_t) * byte_array_size);
  memcpy (*byte_array_dst, byte_array, sizeof (uint8_t) * byte_array_size);

  dbus_message_unref (dbus_reply);
  return 0;
}

int ble_write_gatt_characteristic (iot_logger_t *lc, DBusConnection *dbus_conn, char *mac,
                                         char *characteristic_uuid, const void *data, unsigned int data_size)
{
  char mac_formatted[strlen (mac) + 1];

  strcpy (mac_formatted, mac);
  replace_char (mac_formatted, ':', '_');

  DBusError dbus_error;
  dbus_error_init (&dbus_error);

  char *gatt_characteristic_dbus_path = get_gatt_characteristic_dbus_path (lc, dbus_conn, mac_formatted,
                                                                           characteristic_uuid);

  if (gatt_characteristic_dbus_path == NULL)
  {
    return -1;
  }

  DBusMessage *dbus_msg = dbus_message_new_method_call ("org.bluez", gatt_characteristic_dbus_path,
                                                        "org.bluez.GattCharacteristic1", "WriteValue");
  if (dbus_msg == NULL)
  {
    iot_log_error (lc, "[D-Bus] Couldn't set up a bus for WriteValue");
    return -1;
  }

  DBusMessageIter dbus_msg_args, dbus_byte_array_iter, dbus_dictionary_iter;
  dbus_message_iter_init_append (dbus_msg, &dbus_msg_args);
  dbus_message_iter_open_container (&dbus_msg_args, DBUS_TYPE_ARRAY, DBUS_TYPE_BYTE_AS_STRING,
                                    &dbus_byte_array_iter);
  dbus_message_iter_append_fixed_array (&dbus_byte_array_iter, DBUS_TYPE_BYTE, &data, data_size);
  dbus_message_iter_close_container (&dbus_msg_args, &dbus_byte_array_iter);
  dbus_message_iter_open_container (&dbus_msg_args, DBUS_TYPE_ARRAY, DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING
                                                                     DBUS_TYPE_STRING_AS_STRING
                                                                     DBUS_TYPE_VARIANT_AS_STRING
                                                                     DBUS_DICT_ENTRY_END_CHAR_AS_STRING,
                                    &dbus_dictionary_iter);
  dbus_message_iter_close_container (&dbus_msg_args, &dbus_dictionary_iter);
  dbus_connection_send_with_reply_and_block (dbus_conn, dbus_msg, DBUS_TIMEOUT_USE_DEFAULT, &dbus_error);

  dbus_message_unref (dbus_msg);
  free (gatt_characteristic_dbus_path);

  if (dbus_error_is_set (&dbus_error))
  {
    iot_log_error (lc, "[D-Bus] %s %s", dbus_error.name, dbus_error.message);
    return -1;
  }

  return 0;
}