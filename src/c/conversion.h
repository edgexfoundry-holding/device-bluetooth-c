/*
 * Copyright (c) 2019
 * IoTech Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#ifndef DEVICE_BLUETOOTH_C_CONVERSION_H
#define DEVICE_BLUETOOTH_C_CONVERSION_H

#include "cc2650.h"
#include <edgex/edgex-base.h>
#include <iot/logger.h>

void initialize_conversions (void);

void register_conversion (const char *conversion_name,
                          void (*conversion_function) (void *, edgex_device_resultvalue *));

bool apply_conversion_to_value (iot_logger_t *, const char *, void *, edgex_device_resultvalue *);

void free_conversions (void);

#endif //DEVICE_BLUETOOTH_C_CONVERSION_H
