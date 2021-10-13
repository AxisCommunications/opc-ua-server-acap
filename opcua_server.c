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
#include <libgen.h>
#include <open62541/server_config_default.h>
#include <pthread.h>

#include "opcua_common.h"
#include "opcua_dbus.h"
#include "opcua_open62541.h"
#include "opcua_tempsensors.h"

static tempsensors_t tempsensors;
static UA_Server *server = NULL;

static void open_syslog(const char *app_name)
{
    openlog(app_name, LOG_PID, LOG_LOCAL4);
}

static void close_syslog(void)
{
    LOG_I("Exiting!");
}

static void on_dbus_signal(
    G_GNUC_UNUSED GDBusProxy *proxy,
    const gchar *sender_name,
    const gchar *signal_name,
    GVariant *parameters,
    G_GNUC_UNUSED gpointer user_data)
{
    uint32_t id;
    double value;
    if (!dbus_temp_unpack_signal(parameters, &id, &value))
    {
        LOG_E(
            "%s/%s: Failed to get values from signal %s sent by %s", __FILE__, __FUNCTION__, signal_name, sender_name);
        return;
    }
    char *label = tempsensors_get_label_from_subscription(&tempsensors, id);
    assert(NULL != label);
    ua_server_update_temp(label, value);
    LOG_I("%s/%s: New value for %s is %f", __FILE__, __FUNCTION__, label, value);
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

static void init_signals(void)
{
    struct sigaction sa;

    sigemptyset(&sa.sa_mask);
    sigaddset(&sa.sa_mask, SIGPIPE);
    sa.sa_flags = 0;
    sa.sa_handler = SIG_IGN;
    sigaction(SIGPIPE, &sa, NULL);
}

int main(int argc, char **argv)
{
    init_signals();

    char *app_name = basename(argv[0]);
    open_syslog(app_name);

    LOG_I("%s/%s: Setup D-Bus", __FILE__, __FUNCTION__);
    if (!dbus_temp_init())
    {
        LOG_E("%s/%s: Failed to setup D-Bus", __FILE__, __FUNCTION__);
    }

    // Create an OPC UA server listening on port 4840
    LOG_I("%s/%s: Create UA server", __FILE__, __FUNCTION__);
    server = UA_Server_new();
    ua_server_init(server);

    // Add temperature sensors to OPA UA server
    add_tempsensors();

    // Run OPC UA server
    UA_Boolean running = true;
    pthread_t thread_id;
    LOG_I("%s/%s: Starting UA server ...", __FILE__, __FUNCTION__);
    if (!ua_server_run(&thread_id, &running))
    {
        LOG_E("%s/%s: Failed to launch UA server", __FILE__, __FUNCTION__);
    }

    // Connect to D-Bus signal
    LOG_I("%s/%s: Connect to D-Bus signal ...", __FILE__, __FUNCTION__);
    dbus_connect_g_signal(G_CALLBACK(on_dbus_signal));

    // Main loop
    LOG_I("%s/%s: Ready", __FILE__, __FUNCTION__);
    GMainLoop *loop = g_main_loop_new(NULL, FALSE);
    g_main_loop_run(loop);

    // Cleanup and controlled shutdown
    LOG_I("%s/%s: Clean up DBus ...", __FILE__, __FUNCTION__);
    dbus_temp_cleanup();

    LOG_I("%s/%s: Shut down UA server ...", __FILE__, __FUNCTION__);
    running = false;
    pthread_join(thread_id, NULL);
    LOG_I("%s/%s: Delete UA server ...", __FILE__, __FUNCTION__);
    UA_Server_delete(server);

    LOG_I("%s/%s: Free data structures ...", __FILE__, __FUNCTION__);
    tempsensors_t *tempsensors_p = &tempsensors;
    tempsensors_free(&tempsensors_p);

    LOG_I("%s/%s: Closing syslog ...", __FILE__, __FUNCTION__);
    close_syslog();

    return 0;
}
