/**
 * Copyright (C) 2022 Axis Communications AB, Lund, Sweden
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

#include <assert.h>

#include "opcua_common.h"
#include "opcua_dbus.h"

#define NO_TIMEOUT -1
#define TEMP_DBUS_SERVICE "com.axis.TemperatureController"
#define TEMP_DBUS_OBJECT "/com/axis/TemperatureController"
#define TEMP_DBUS_INTERFACE "com.axis.TemperatureController"

#define PORTS_DBUS_SERVICE "com.axis.IOControl.State"
#define PORTS_DBUS_OBJECT "/com/axis/IOControl/State"
#define PORTS_DBUS_INTERFACE "com.axis.IOControl.State"
#define PORT_STATE_TRUE "true"
#define PORT_STATE_FALSE "false"

static GDBusProxy *dbusproxy_temp;
static GDBusProxy *dbusproxy_ports;

static bool dbus_init(GDBusProxy **dbusproxy, const gchar *name, const gchar *object_path, const gchar *interface_name)
{
    assert(NULL != dbusproxy);
    assert(NULL == *dbusproxy);
    GError *error = NULL;

    *dbusproxy = g_dbus_proxy_new_for_bus_sync(
        G_BUS_TYPE_SYSTEM, G_DBUS_PROXY_FLAGS_NONE, NULL, name, object_path, interface_name, NULL, &error);
    if (NULL == *dbusproxy)
    {
        LOG_E("%s/%s: Failed to create a proxy for accessing %s (%s)", __FILE__, __FUNCTION__, name, error->message);
        g_error_free(error);
        return false;
    }

    return true;
}

bool dbus_all_init(void)
{
    return dbus_init(&dbusproxy_temp, TEMP_DBUS_SERVICE, TEMP_DBUS_OBJECT, TEMP_DBUS_INTERFACE) &&
           dbus_init(&dbusproxy_ports, PORTS_DBUS_SERVICE, PORTS_DBUS_OBJECT, PORTS_DBUS_INTERFACE);
}

void dbus_all_cleanup(void)
{
    // dbusproxy_temp
    if (NULL != dbusproxy_temp)
    {
        g_object_unref(dbusproxy_temp);
        dbusproxy_temp = NULL;
    }

    // dbusproxy_ports
    if (NULL != dbusproxy_ports)
    {
        g_object_unref(dbusproxy_ports);
        dbusproxy_ports = NULL;
    }
}

bool dbus_temp_get_number_of_sensors(uint32_t *count)
{
    assert(NULL != count);
    assert(NULL != dbusproxy_temp);
    GError *error = NULL;

    GVariant *result = g_dbus_proxy_call_sync(
        dbusproxy_temp, "GetNbrOfTemperatureSensors", NULL, G_DBUS_CALL_FLAGS_NONE, NO_TIMEOUT, NULL, &error);
    if (NULL == result)
    {
        LOG_E(
            "%s/%s: Failed to get number of temperature sensors from D-Bus (%s)",
            __FILE__,
            __FUNCTION__,
            error->message);
        g_error_free(error);
        g_variant_unref(result);
        return false;
    }
    LOG_I("%s/%s: Got number of temperature sensors response from D-Bus!", __FILE__, __FUNCTION__);
    GVariant *value = g_variant_get_child_value(result, 0);
    if (!value)
    {
        LOG_E("%s/%s: Failed to extract response value", __FILE__, __FUNCTION__);
        g_variant_unref(result);
        return false;
    }

    *count = g_variant_get_int32(value);
    g_variant_unref(value);
    g_variant_unref(result);
    return true;
}

bool dbus_temp_get_value(int id, double *value)
{
    assert(NULL != value);
    assert(NULL != dbusproxy_temp);
    GError *error = NULL;

    GVariant *result = g_dbus_proxy_call_sync(
        dbusproxy_temp,
        "GetTemperature",
        g_variant_new("(is)", id, "celsius"),
        G_DBUS_CALL_FLAGS_NONE,
        NO_TIMEOUT,
        NULL,
        &error);
    if (NULL == result)
    {
        LOG_E("%s/%s: Failed to get sensor %i temperature from D-Bus (%s)", __FILE__, __FUNCTION__, id, error->message);
        g_error_free(error);
        g_variant_unref(result);
        return false;
    }
    LOG_I("%s/%s: Got temperature response from D-Bus for sensor %i!", __FILE__, __FUNCTION__, id);
    GVariant *gvalue = g_variant_get_child_value(result, 0);
    if (!gvalue)
    {
        LOG_E("%s/%s: Failed to extract response value for sensor %i", __FILE__, __FUNCTION__, id);
        g_variant_unref(result);
        return false;
    }

    *value = g_variant_get_double(gvalue);
    g_variant_unref(gvalue);
    g_variant_unref(result);
    return true;
}

bool dbus_temp_subscribe_to_change(uint32_t *subscription_id, uint32_t sensor_id, double d)
{
    assert(NULL != dbusproxy_temp);
    assert(NULL != subscription_id);
    GError *error = NULL;

    GVariant *result = g_dbus_proxy_call_sync(
        dbusproxy_temp,
        "RegisterForTemperatureChangeSignal",
        g_variant_new("(id)", sensor_id, d),
        G_DBUS_CALL_FLAGS_NONE,
        NO_TIMEOUT,
        NULL,
        &error);
    if (NULL == result)
    {
        LOG_E(
            "%s/%s: Failed to subscribe to temperature change signal for sensor %i (%s)",
            __FILE__,
            __FUNCTION__,
            sensor_id,
            error->message);
        g_error_free(error);
        return false;
    }
    LOG_I("%s/%s: Subscribed to temperature change signal for sensor %i!", __FILE__, __FUNCTION__, sensor_id);
    GVariant *value = g_variant_get_child_value(result, 0);
    if (!value)
    {
        LOG_E("%s/%s: Failed to extract response value for sensor %i", __FILE__, __FUNCTION__, sensor_id);
        g_variant_unref(result);
        return false;
    }
    *subscription_id = g_variant_get_int32(value);
    g_variant_unref(value);
    LOG_I(
        "%s/%s: Subscribed sensor %i got register response id %i", __FILE__, __FUNCTION__, sensor_id, *subscription_id);
    return true;
}

bool dbus_temp_unpack_signal(GVariant *parameters, uint32_t *subscription_id, double *value)
{
    assert(NULL != parameters);
    assert(NULL != subscription_id);
    assert(NULL != value);

    GVariant *gvalue = g_variant_get_child_value(parameters, 0);
    if (!gvalue)
    {
        LOG_E("%s/%s: Failed to extract id", __FILE__, __FUNCTION__);
        return false;
    }
    *subscription_id = g_variant_get_int32(gvalue);
    g_variant_unref(gvalue);

    gvalue = g_variant_get_child_value(parameters, 1);
    if (!gvalue)
    {
        LOG_E("%s/%s: Failed to extract value", __FILE__, __FUNCTION__);
        return false;
    }
    *value = g_variant_get_double(gvalue);
    g_variant_unref(gvalue);
    return true;
}

void dbus_connect_temp_g_signal(GCallback func)
{
    assert(NULL != dbusproxy_temp);
    assert(NULL != func);
    g_signal_connect(dbusproxy_temp, "g-signal", func, NULL);
}

void dbus_connect_ports_g_signal(GCallback func)
{
    assert(NULL != dbusproxy_ports);
    assert(NULL != func);
    g_signal_connect(dbusproxy_ports, "g-signal", func, NULL);
}

bool dbus_get_number_of_ioports(uint32_t *inputs, uint32_t *outputs)
{
    assert(NULL != inputs);
    assert(NULL != outputs);
    assert(NULL != dbusproxy_ports);
    GError *error = NULL;
    GVariant *ret_val = NULL;

    ret_val =
        g_dbus_proxy_call_sync(dbusproxy_ports, "GetNbrPorts", NULL, G_DBUS_CALL_FLAGS_NONE, NO_TIMEOUT, NULL, &error);
    if (NULL == ret_val)
    {
        LOG_E("%s/%s: Failed to get number of ports from D-Bus (%s)", __FILE__, __FUNCTION__, error->message);

        g_error_free(error);
        g_variant_unref(ret_val);
        return false;
    }
    LOG_I("%s/%s: Got number of ports response from D-Bus!", __FILE__, __FUNCTION__);

    if (ret_val)
    {
        g_variant_get(ret_val, "(uu)", inputs, outputs);
        g_variant_unref(ret_val);
    }

    return true;
}

bool dbus_port_get_state(const int id, bool *state)
{
    assert(NULL != state);
    assert(NULL != dbusproxy_ports);
    GError *error = NULL;

    GVariant *result = g_dbus_proxy_call_sync(
        dbusproxy_ports, "GetState", g_variant_new("(u)", id), G_DBUS_CALL_FLAGS_NONE, NO_TIMEOUT, NULL, &error);

    if (NULL == result)
    {
        LOG_E("%s/%s: Failed to get port %i state from D-Bus (%s)", __FILE__, __FUNCTION__, id, error->message);
        g_error_free(error);
        g_variant_unref(result);
        return false;
    }

    LOG_I("%s/%s: Got state response from D-Bus for port %i!", __FILE__, __FUNCTION__, id);

    GVariant *gvalue = g_variant_get_child_value(result, 0);
    if (!gvalue)
    {
        LOG_E("%s/%s: Failed to extract response value for port %i", __FILE__, __FUNCTION__, id);
        g_variant_unref(result);
        return false;
    }

    *state = g_variant_get_boolean(gvalue);

    g_variant_unref(gvalue);
    g_variant_unref(result);
    return true;
}

bool dbus_port_unpack_signal(
    GVariant *parameters,
    uint32_t *subscription_id,
    gint *port,
    gboolean *virtual,
    gboolean *hidden,
    gboolean *input,
    gboolean *virtual_trig,
    gboolean *state,
    gboolean *activelow)
{
    assert(NULL != parameters);
    assert(NULL != subscription_id);
    assert(NULL != port);
    assert(NULL != virtual);
    assert(NULL != hidden);
    assert(NULL != input);
    assert(NULL != virtual_trig);
    assert(NULL != state);
    assert(NULL != activelow);

    // subscription or port
    GVariant *gvalue = g_variant_get_child_value(parameters, 0);
    if (!gvalue)
    {
        LOG_E("%s/%s: Failed to extract id", __FILE__, __FUNCTION__);
        return false;
    }
    *subscription_id = g_variant_get_int32(gvalue);
    *port = g_variant_get_int32(gvalue);
    g_variant_unref(gvalue);

    // virtual
    gvalue = g_variant_get_child_value(parameters, 1);
    if (!gvalue)
    {
        LOG_E("%s/%s: Failed to extract value", __FILE__, __FUNCTION__);
        return false;
    }
    *virtual = g_variant_get_boolean(gvalue);
    g_variant_unref(gvalue);

    // hidden
    gvalue = g_variant_get_child_value(parameters, 2);
    if (!gvalue)
    {
        LOG_E("%s/%s: Failed to extract value", __FILE__, __FUNCTION__);
        return false;
    }
    *hidden = g_variant_get_boolean(gvalue);
    g_variant_unref(gvalue);

    // input
    gvalue = g_variant_get_child_value(parameters, 3);
    if (!gvalue)
    {
        LOG_E("%s/%s: Failed to extract value", __FILE__, __FUNCTION__);
        return false;
    }
    *input = g_variant_get_boolean(gvalue);
    g_variant_unref(gvalue);

    // virtual_trig
    gvalue = g_variant_get_child_value(parameters, 4);
    if (!gvalue)
    {
        LOG_E("%s/%s: Failed to extract value", __FILE__, __FUNCTION__);
        return false;
    }
    *virtual_trig = g_variant_get_boolean(gvalue);
    g_variant_unref(gvalue);

    // state
    gvalue = g_variant_get_child_value(parameters, 5);
    if (!gvalue)
    {
        LOG_E("%s/%s: Failed to extract value", __FILE__, __FUNCTION__);
        return false;
    }
    *state = g_variant_get_boolean(gvalue);
    g_variant_unref(gvalue);

    // activelow
    gvalue = g_variant_get_child_value(parameters, 6);
    if (!gvalue)
    {
        LOG_E("%s/%s: Failed to extract value", __FILE__, __FUNCTION__);
        return false;
    }
    *activelow = g_variant_get_boolean(gvalue);
    g_variant_unref(gvalue);

    return true;
}
