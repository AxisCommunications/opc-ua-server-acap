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

#include <assert.h>
#include <axparameter.h>
#include <libgen.h>
#include <open62541/server_config_default.h>
#include <pthread.h>

#include "opcua_common.h"
#include "opcua_dbus.h"
#include "opcua_open62541.h"
#include "opcua_portsio.h"
#include "opcua_tempsensors.h"

#define SIGNALTEMPCHANGE "TemperatureChangeSignal"
#define SIGNALPORTIOCHANGE "PortChanged"

static GMainLoop *main_loop = NULL;
static AXParameter *axparameter = NULL;
static tempsensors_t tempsensors;
static ports_t ports;
static UA_Server *server = NULL;
static guint port = 0;
static UA_Boolean ua_server_running = false;
static pthread_t ua_server_thread_id;

static void open_syslog(const char *app_name)
{
    openlog(app_name, LOG_PID, LOG_LOCAL4);
}

static void close_syslog(void)
{
    LOG_I("%s/%s: Exiting!", __FILE__, __FUNCTION__);
    closelog();
}

static void on_dbus_signal(
    G_GNUC_UNUSED GDBusProxy *proxy,
    const gchar *sender_name,
    const gchar *signal_name,
    GVariant *parameters,
    G_GNUC_UNUSED gpointer user_data)
{
    uint32_t sub_id;
    double value;
    char *label;

    // Check which signal
    // TemperatureChangeSignal
    if (0 == strcmp(signal_name, SIGNALTEMPCHANGE))
    {
        if (!dbus_temp_unpack_signal(parameters, &sub_id, &value))
        {
            LOG_E(
                "%s/%s: Failed to get values from signal %s sent by %s",
                __FILE__,
                __FUNCTION__,
                signal_name,
                sender_name);
            return;
        }
        label = tempsensors_get_label_from_subscription(&tempsensors, sub_id);
        assert(NULL != label);
        ua_server_update_temp(label, value);
        LOG_I("%s/%s: New value for %s is %f", __FILE__, __FUNCTION__, label, value);
    }

    gint port;
    gboolean virtual;
    gboolean hidden;
    gboolean input;
    gboolean virtual_trig;
    gboolean state;
    gboolean activelow;

    // PortChanged
    if (0 == strcmp(signal_name, SIGNALPORTIOCHANGE))
    {
        if (!dbus_port_unpack_signal(
                parameters, &sub_id, &port, &virtual, &hidden, &input, &virtual_trig, &state, &activelow))
        {
            LOG_E(
                "%s/%s: Failed to get values from signal %s sent by %s",
                __FILE__,
                __FUNCTION__,
                signal_name,
                sender_name);
            return;
        }

        label = ports_get_label_from_subscription(&ports, sub_id);
        assert(NULL != label);

        ua_server_update_port(label, state);
        LOG_I(
            "%s/%s: Port status change. port:%d, virtual:%d, hidden:%d, input:%d, virtual_trig:%d, state:%d, "
            "activelow:%d",
            __FILE__,
            __FUNCTION__,
            port,
            virtual,
            hidden,
            input,
            virtual_trig,
            state,
            activelow)
    }
}

static void add_tempsensors(void)
{
    uint32_t count = 0;
    if (!dbus_temp_get_number_of_sensors(&count))
    {
        LOG_E("%s/%s: Failed to get number of temperature sensors", __FILE__, __FUNCTION__);
    }
    else
    {
        LOG_I("%s/%s: This device has %u temperature sensors", __FILE__, __FUNCTION__, count);
    }
    tempsensors_t *tempsensors_p = &tempsensors;
    tempsensors_init(&tempsensors_p, count);
    assert(NULL != tempsensors_p);
    for (uint32_t i = 0; i < count; i++)
    {
        snprintf(tempsensors.labels[i], TEMP_LABEL_LEN, TEMP_LABEL_FMT, i);
        double value;
        if (!dbus_temp_get_value(i, &value))
        {
            LOG_E("%s/%s: Failed to get temperature", __FILE__, __FUNCTION__);
        }
        else
        {
            LOG_I("%s/%s: Got temperature for sensor %i: %f", __FILE__, __FUNCTION__, i, value);
            ua_server_add_double(tempsensors.labels[i], value);
        }
        assert(NULL != tempsensors.subid);
        if (!dbus_temp_subscribe_to_change(&tempsensors.subid[i], i, 0.1))
        {
            LOG_E("%s/%s: Failed to subscribe to changes for sensor with id %i", __FILE__, __FUNCTION__, i);
        }
    }
}

static void add_ports(void)
{
    uint32_t count_all = 0;
    uint32_t count_in = 0;
    uint32_t count_out = 0;

    if (!dbus_get_number_of_ioports(&count_in, &count_out))
    {
        LOG_E("%s/%s: Failed to get number of ports", __FILE__, __FUNCTION__);
    }
    else
    {
        count_all = count_in + count_out;
        LOG_I(
            "%s/%s: This device has %u input ports and %u output ports. (%u)",
            __FILE__,
            __FUNCTION__,
            count_in,
            count_out,
            count_all);
    }

    ports_t *ports_p = &ports;
    ports_init(&ports_p, count_all);
    assert(NULL != ports_p);

    for (uint32_t i = 0; i < count_all; i++)
    {
        snprintf(ports.labels[i], PORT_LABEL_LEN, PORT_LABEL_FMT, i);
        LOG_I("%s/%s: Added label (%s) for port:%i", __FILE__, __FUNCTION__, ports.labels[i], i);

        bool state;
        if (!dbus_port_get_state(i, &state))
        {
            LOG_E("%s/%s: Failed to get port state", __FILE__, __FUNCTION__);
        }
        else
        {
            LOG_I("%s/%s: Got state for port %i: %d", __FILE__, __FUNCTION__, i, state);
            ua_server_add_bool(ports.labels[i], state);
        }

        assert(NULL != ports.subid);
        ports.subid[i] = i;
    }
}

static gboolean launch_ua_server(const guint serverport)
{
    assert(NULL == server);
    assert(0 < serverport);
    assert(!ua_server_running);
    assert(1024 <= serverport && 65535 >= serverport);

    // Create an OPC UA server
    LOG_I("%s/%s: Create UA server serving on port %u", __FILE__, __FUNCTION__, serverport);
    ua_server_init(serverport);

    // Add temperature sensors to OPA UA server
    add_tempsensors();

    // Add IO ports to OPC UA Server
    add_ports();

    ua_server_running = true;
    LOG_I("%s/%s: Starting UA server on port %u ...", __FILE__, __FUNCTION__, serverport);
    if (!ua_server_run(&ua_server_thread_id, &ua_server_running))
    {
        LOG_E("%s/%s: Failed to launch UA server", __FILE__, __FUNCTION__);
        return FALSE;
    }

    return TRUE;
}

static void shutdown_ua_server(void)
{
    assert(ua_server_running);
    ua_server_running = false;
    pthread_join(ua_server_thread_id, NULL);
}

static void port_callback(const gchar *name, const gchar *value, void *data)
{
    (void)data;
    /* Translate parameter value to number; atoi can handle NULL */
    int newport = atoi(value);
    /* Only allow non-privileged ports */
    if (1024 > newport || 65535 < newport)
    {
        LOG_E("%s/%s: illegal value for %s: '%s'", __FILE__, __FUNCTION__, name, value);
        return;
    }
    port = newport;
    LOG_I("%s/%s: OPC UA server %s is %u", __FILE__, __FUNCTION__, name, port);

    if (ua_server_running)
    {
        shutdown_ua_server();
    }
    (void)launch_ua_server(port);
}

static gboolean setup_param(const gchar *name, AXParameterCallback callbackfn)
{
    GError *error = NULL;
    gchar *value = NULL;

    assert(NULL != name);
    assert(NULL != axparameter);
    assert(NULL != callbackfn);

    if (!ax_parameter_register_callback(axparameter, name, callbackfn, NULL, &error))
    {
        LOG_E("%s/%s: failed to register %s callback", __FILE__, __FUNCTION__, name);
        if (NULL != error)
        {
            LOG_E("%s/%s: %s", __FILE__, __FUNCTION__, error->message);
            g_error_free(error);
        }
        return FALSE;
    }
    if (!ax_parameter_get(axparameter, name, &value, &error))
    {
        LOG_E("%s/%s: failed to get %s parameter", __FILE__, __FUNCTION__, name);
        if (NULL != error)
        {
            LOG_E("%s/%s: %s", __FILE__, __FUNCTION__, error->message);
            g_error_free(error);
        }
        return FALSE;
    }
    LOG_I("%s/%s: Got %s value: %s", __FILE__, __FUNCTION__, name, value);
    callbackfn(name, value, NULL);
    g_free(value);

    return TRUE;
}

static gboolean setup_params(const char *appname)
{
    GError *error = NULL;

    assert(NULL != appname);
    assert(NULL == axparameter);
    axparameter = ax_parameter_new(appname, &error);
    if (NULL != error)
    {
        LOG_E("%s/%s: ax_parameter_new failed (%s)", __FILE__, __FUNCTION__, error->message);
        g_error_free(error);
        return FALSE;
    }

    if (!setup_param("port", port_callback))
    {
        ax_parameter_free(axparameter);
        return FALSE;
    }

    return TRUE;
}

static void signal_handler(gint signal_num)
{
    switch (signal_num)
    {
    case SIGTERM:
    case SIGABRT:
    case SIGINT:
        g_main_loop_quit(main_loop);
        break;
    default:
        break;
    }
}

static gboolean signal_handler_init(void)
{
    struct sigaction sa = {0};

    if (-1 == sigemptyset(&sa.sa_mask))
    {
        LOG_E("%s/%s: Failed to initialize signal handler: %s", __FILE__, __FUNCTION__, strerror(errno));
        return FALSE;
    }

    sa.sa_handler = signal_handler;

    if (0 > sigaction(SIGTERM, &sa, NULL) || 0 > sigaction(SIGABRT, &sa, NULL) || 0 > sigaction(SIGINT, &sa, NULL))
    {
        LOG_E("%s/%s: Failed to install signal handler: %s", __FILE__, __FUNCTION__, strerror(errno));
        return FALSE;
    }

    return TRUE;
}

int main(int argc, char **argv)
{
    char *app_name = basename(argv[0]);
    open_syslog(app_name);

    if (!signal_handler_init())
    {
        return EXIT_FAILURE;
    }

    // Setup D-Bus
    LOG_I("%s/%s: Setup D-Bus", __FILE__, __FUNCTION__);
    if (!dbus_all_init())
    {
        LOG_E("%s/%s: Failed to setup D-Bus", __FILE__, __FUNCTION__);
    }

    // Connect to D-Bus signals
    LOG_I("%s/%s: Connect to D-Bus signal ...", __FILE__, __FUNCTION__);
    dbus_connect_temp_g_signal(G_CALLBACK(on_dbus_signal));

    LOG_I("%s/%s: Connect to D-Bus signal ...", __FILE__, __FUNCTION__);
    dbus_connect_ports_g_signal(G_CALLBACK(on_dbus_signal));

    // Setup parameters (will also launch OPC UA server)
    LOG_I("%s/%s: Setup parameters", __FILE__, __FUNCTION__);
    if (!setup_params(app_name))
    {
        LOG_E("%s/%s: Failed to setup parameters", __FILE__, __FUNCTION__);
    }

    // Main loop
    LOG_I("%s/%s: Ready", __FILE__, __FUNCTION__);
    assert(NULL == main_loop);
    main_loop = g_main_loop_new(NULL, FALSE);
    g_main_loop_run(main_loop);

    // Cleanup and controlled shutdown
    LOG_I("%s/%s: Free parameter handler ...", __FILE__, __FUNCTION__);
    ax_parameter_free(axparameter);
    LOG_I("%s/%s: Clean up DBus ...", __FILE__, __FUNCTION__);
    dbus_all_cleanup();

    LOG_I("%s/%s: Shut down UA server ...", __FILE__, __FUNCTION__);
    shutdown_ua_server();

    LOG_I("%s/%s: Free data structures ...", __FILE__, __FUNCTION__);
    tempsensors_t *tempsensors_p = &tempsensors;
    tempsensors_free(&tempsensors_p);
    ports_t *ports_p = &ports;
    ports_free(&ports_p);

    LOG_I("%s/%s: Unreference main loop ...", __FILE__, __FUNCTION__);
    g_main_loop_unref(main_loop);

    LOG_I("%s/%s: Closing syslog ...", __FILE__, __FUNCTION__);
    close_syslog();

    return EXIT_SUCCESS;
}
