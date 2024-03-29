/**
 * Copyright (C) 2022, Axis Communications AB, Lund, Sweden
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _OPCUA_DBUS_H_
#define _OPCUA_DBUS_H_

#include <gio/gio.h>

#include <stdbool.h>
#include <stdint.h>

bool dbus_all_init(void);
void dbus_all_cleanup(void);

bool dbus_temp_get_number_of_sensors(uint32_t *count);
bool dbus_temp_get_value(int id, double *value);
bool dbus_temp_subscribe_to_change(uint32_t *subscription_id, uint32_t sensor_id, double d);
bool dbus_temp_unpack_signal(GVariant *parameters, uint32_t *subscription_id, double *value);
void dbus_connect_temp_g_signal(GCallback func);

bool dbus_get_number_of_ioports(uint32_t *inputs, uint32_t *outputs);
bool dbus_port_get_state(const int id, bool *state);
bool dbus_port_unpack_signal(
    GVariant *parameters,
    uint32_t *subscription_id,
    gint *port,
    gboolean *virtual,
    gboolean *hidden,
    gboolean *input,
    gboolean *virtual_trig,
    gboolean *state,
    gboolean *activelow);
void dbus_connect_ports_g_signal(GCallback func);

#endif /* _OPCUA_DBUS_H_ */
