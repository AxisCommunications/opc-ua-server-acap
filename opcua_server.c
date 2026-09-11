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

static GMainLoop *main_loop_ = NULL;
static AXParameter *axparameter_ = NULL;
static tempsensors_t tempsensors_;
static ports_t ports_;
static guint port_ = 0;
static UA_Boolean ua_server_running_ = false;
static pthread_t ua_server_thread_id_;

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
        label = tempsensors_get_label_from_subscription(&tempsensors_, sub_id);
        assert(NULL != label);
        ua_server_update_temp(label, value);
        LOG_I("ⓘ New value for %s is %f", label, value);
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

        label = ports_get_label_from_subscription(&ports_, sub_id);
        assert(NULL != label);

        ua_server_update_port(label, state);
        LOG_I(
            "ⓘ Port status change. port:%d, virtual:%d, hidden:%d, input:%d, virtual_trig:%d, state:%d, "
            "activelow:%d",
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
        LOG_I("ⓘ This device has %u temperature sensor%s", count, 1 == count ? "" : "s");
    }
    tempsensors_init(&tempsensors_, count);
    for (uint32_t i = 0; i < count; i++)
    {
        snprintf(tempsensors_.labels[i], TEMP_LABEL_LEN, TEMP_LABEL_FMT, i);
        double value;
        if (!dbus_temp_get_value(i, &value))
        {
            LOG_E("%s/%s: Failed to get temperature", __FILE__, __FUNCTION__);
        }
        else
        {
            LOG_I("ⓘ Temperature for sensor %i is %f", i, value);
            ua_server_add_double(tempsensors_.labels[i], value);
        }
        assert(NULL != tempsensors_.subid);
        if (!dbus_temp_subscribe_to_change(&tempsensors_.subid[i], i, 0.1))
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
            "ⓘ This device has %u input port%s and %u output port%s (%u ports in total)",
            count_in,
            1 == count_in ? "" : "s",
            count_out,
            1 == count_out ? "" : "s",
            count_all);
    }

    ports_init(&ports_, count_all);

    bool state;
    for (uint32_t i = 0; i < count_all; i++)
    {
        snprintf(ports_.labels[i], PORT_LABEL_LEN, PORT_LABEL_FMT, i);
        LOG_I("ⓘ Added label '%s' for port %i", ports_.labels[i], i);

        if (!dbus_port_get_state(i, &state))
        {
            LOG_E("%s/%s: Failed to get port state", __FILE__, __FUNCTION__);
        }
        else
        {
            LOG_I("ⓘ Got state for port %i: %d", i, state);
            ua_server_add_bool(ports_.labels[i], state);
        }

        assert(NULL != ports_.subid);
        ports_.subid[i] = i;
    }
}

static gboolean launch_ua_server(const guint serverport)
{
    assert(0 < serverport);
    assert(!ua_server_running_);
    assert(1024 <= serverport && 65535 >= serverport);

    // Create an OPC UA server
    LOG_I("⏳ Creating UA server serving on port %u ...", serverport);
    if (!ua_server_init(serverport))
    {
        LOG_E("%s/%s: Failed to create OPC UA server", __FILE__, __FUNCTION__);
        return FALSE;
    }

    // Add temperature sensors to OPA UA server
    add_tempsensors();

    // Add IO ports to OPC UA Server
    add_ports();

    LOG_I("⏳ Starting UA server on port %u ...", serverport);
    ua_server_running_ = true;
    if (!ua_server_run(&ua_server_thread_id_, &ua_server_running_))
    {
        ua_server_running_ = false;
        LOG_E("%s/%s: Failed to launch OPC UA server", __FILE__, __FUNCTION__);
        return FALSE;
    }

    return TRUE;
}

static void shutdown_ua_server(void)
{
    assert(ua_server_running_);
    ua_server_stop(&ua_server_running_);
    pthread_join(ua_server_thread_id_, NULL);
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
    port_ = newport;
    LOG_I("✅ Parameter '%s' now updated to %u", name, port_);

    if (ua_server_running_)
    {
        shutdown_ua_server();
    }
    if (!launch_ua_server(port_))
    {
        LOG_E("%s/%s: Failed to restart OPC UA server", __FILE__, __FUNCTION__);
    }
}

static gboolean setup_param(const gchar *name, AXParameterCallback callbackfn)
{
    GError *error = NULL;
    gchar *value = NULL;

    assert(NULL != name);
    assert(NULL != axparameter_);
    assert(NULL != callbackfn);

    if (!ax_parameter_register_callback(axparameter_, name, callbackfn, NULL, &error))
    {
        LOG_E("%s/%s: failed to register %s callback", __FILE__, __FUNCTION__, name);
        if (NULL != error)
        {
            LOG_E("%s/%s: %s", __FILE__, __FUNCTION__, error->message);
            g_error_free(error);
        }
        return FALSE;
    }
    if (!ax_parameter_get(axparameter_, name, &value, &error))
    {
        LOG_E("%s/%s: failed to get %s parameter", __FILE__, __FUNCTION__, name);
        if (NULL != error)
        {
            LOG_E("%s/%s: %s", __FILE__, __FUNCTION__, error->message);
            g_error_free(error);
        }
        return FALSE;
    }
    LOG_I("✅ Got '%s' value: %s", name, value);
    callbackfn(name, value, NULL);
    g_free(value);

    return TRUE;
}

static gboolean setup_params(const char *appname)
{
    GError *error = NULL;

    assert(NULL != appname);
    assert(NULL == axparameter_);
    axparameter_ = ax_parameter_new(appname, &error);
    if (NULL != error)
    {
        LOG_E("%s/%s: ax_parameter_new failed (%s)", __FILE__, __FUNCTION__, error->message);
        g_error_free(error);
        return FALSE;
    }

    if (!setup_param("port", port_callback))
    {
        ax_parameter_free(axparameter_);
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
        g_main_loop_quit(main_loop_);
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

static gboolean ready_callback(gpointer user_data)
{
    (void)user_data;
    LOG_I("✅ Main loop started");
    return G_SOURCE_REMOVE;
}

int main(int argc, char **argv)
{
    char *app_name = basename(argv[0]);
    openlog(app_name, LOG_PID, LOG_LOCAL4);

    if (!signal_handler_init())
    {
        return EXIT_FAILURE;
    }

    // Setup D-Bus
    LOG_I("⏳ Set up D-Bus ...");
    if (!dbus_all_init())
    {
        LOG_E("%s/%s: Failed to setup D-Bus", __FILE__, __FUNCTION__);
    }

    // Connect to D-Bus signals
    LOG_I("⏳ Connect to D-Bus signal for temperatures ...");
    dbus_connect_temp_g_signal(G_CALLBACK(on_dbus_signal));

    LOG_I("⏳ Connect to D-Bus signal for ports ...");
    dbus_connect_ports_g_signal(G_CALLBACK(on_dbus_signal));

    // Setup parameters (will also launch OPC UA server)
    LOG_I("⏳ Set up parameters ...");
    if (!setup_params(app_name))
    {
        LOG_E("%s/%s: Failed to setup parameters", __FILE__, __FUNCTION__);
    }

    // Main loop
    assert(NULL == main_loop_);
    LOG_I("⏳ Create main loop ...");
    main_loop_ = g_main_loop_new(NULL, FALSE);
    (void)g_idle_add_full(G_PRIORITY_HIGH, ready_callback, NULL, NULL);
    LOG_I("⏳ Start main loop ...");
    g_main_loop_run(main_loop_);

    // Cleanup and controlled shutdown
    LOG_I("⏳ Free parameter handler ...");
    ax_parameter_free(axparameter_);
    LOG_I("🧹 Clean up D-Bus ...");
    dbus_all_cleanup();

    LOG_I("🧹 Shut down UA server ...");
    if (ua_server_running_)
    {
        shutdown_ua_server();
    }

    LOG_I("🧹 Free data structures ...");
    tempsensors_t *tempsensors_p = &tempsensors_;
    tempsensors_free(&tempsensors_p);
    ports_t *ports_p = &ports_;
    ports_free(&ports_p);

    LOG_I("🧹 Unreference main loop ...");
    g_main_loop_unref(main_loop_);

    LOG_I("✅ Exiting");
    closelog();

    return EXIT_SUCCESS;
}
