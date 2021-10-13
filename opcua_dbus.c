/**
 * Copyright (C) 2021 Axis Communications AB, Lund, Sweden
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

#define TEMPERATURE_DBUS_SERVICE "com.axis.TemperatureController"
#define TEMPERATURE_DBUS_OBJECT "/com/axis/TemperatureController"
#define TEMPERATURE_DBUS_INTERFACE "com.axis.TemperatureController"

static GDBusProxy *dbusproxy;

bool dbus_temp_init(void)
{
    assert(NULL == dbusproxy);
    GError *error = NULL;
    dbusproxy = g_dbus_proxy_new_for_bus_sync(
        G_BUS_TYPE_SYSTEM,
        G_DBUS_PROXY_FLAGS_NONE,
        NULL,
        TEMPERATURE_DBUS_SERVICE,
        TEMPERATURE_DBUS_OBJECT,
        TEMPERATURE_DBUS_INTERFACE,
        NULL,
        &error);
    if (NULL == dbusproxy)
    {
        LOG_E(
            "%s/%s: Failed to create a proxy for accessing %s (%s)",
            __FILE__,
            __FUNCTION__,
            TEMPERATURE_DBUS_OBJECT,
            error->message);
        g_error_free(error);
        return false;
    }

    return true;
}

void dbus_temp_cleanup(void)
{
    if (NULL != dbusproxy)
    {
        g_object_unref(dbusproxy);
        dbusproxy = NULL;
    }
}

bool dbus_temp_get_number_of_sensors(uint32_t *count)
{
    assert(NULL != count);
    assert(NULL != dbusproxy);
    GError *error = NULL;

    GVariant *result =
        g_dbus_proxy_call_sync(dbusproxy, "GetNbrOfTemperatureSensors", NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);
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
    assert(NULL != dbusproxy);
    GError *error = NULL;

    GVariant *result = g_dbus_proxy_call_sync(
        dbusproxy, "GetTemperature", g_variant_new("(is)", id, "celsius"), G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);
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
    assert(NULL != dbusproxy);
    assert(NULL != subscription_id);
    GError *error = NULL;

    GVariant *result = g_dbus_proxy_call_sync(
        dbusproxy,
        "RegisterForTemperatureChangeSignal",
        g_variant_new("(id)", sensor_id, d),
        G_DBUS_CALL_FLAGS_NONE,
        -1,
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

void dbus_connect_g_signal(GCallback func)
{
    assert(NULL != dbusproxy);
    assert(NULL != func);
    g_signal_connect(dbusproxy, "g-signal", func, NULL);
}
