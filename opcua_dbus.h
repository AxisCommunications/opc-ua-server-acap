/*
 * Copyright (C) 2021 Axis Communications AB
 */

#ifndef _OPCUA_DBUS_H_
#define _OPCUA_DBUS_H_

#include <gio/gio.h>

#include <stdbool.h>
#include <stdint.h>

bool dbus_temp_init(void);
void dbus_temp_cleanup(void);
bool dbus_temp_get_number_of_sensors(uint32_t *count);
bool dbus_temp_get_value(int id, double *value);
bool dbus_temp_subscribe_to_change(uint32_t *subscription_id, uint32_t sensor_id, double d);
bool dbus_temp_unpack_signal(GVariant *parameters, uint32_t *subscription_id, double *value);
void dbus_connect_g_signal(GCallback func);

#endif /* _OPCUA_DBUS_H_ */
