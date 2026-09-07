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
#include <open62541/server_config_default.h>
#include <pthread.h>

#include "opcua_common.h"
#include "opcua_open62541.h"

static UA_Server *server_;
static pthread_mutex_t server_mutex_ = PTHREAD_MUTEX_INITIALIZER;

static void *run_ua_server(void *running)
{
    UA_StatusCode shutdown_status;

    assert(NULL != running);

    pthread_mutex_lock(&server_mutex_);
    assert(NULL != server_);
    UA_StatusCode status = UA_Server_run_startup(server_);
    if (UA_STATUSCODE_GOOD != status)
    {
        *((UA_Boolean *)running) = false;
        UA_Server_delete(server_);
        server_ = NULL;
        pthread_mutex_unlock(&server_mutex_);
        LOG_E("%s/%s: OPC UA server startup failed (%s)", __FILE__, __FUNCTION__, UA_StatusCode_name(status));
        return NULL;
    }
    pthread_mutex_unlock(&server_mutex_);

    while (UA_STATUSCODE_GOOD == status)
    {
        pthread_mutex_lock(&server_mutex_);
        if (!*((UA_Boolean *)running))
        {
            pthread_mutex_unlock(&server_mutex_);
            break;
        }
        pthread_mutex_unlock(&server_mutex_);
        UA_Server_run_iterate(server_, true);
    }

    pthread_mutex_lock(&server_mutex_);
    shutdown_status = UA_Server_run_shutdown(server_);
    if (UA_STATUSCODE_GOOD != shutdown_status)
    {
        LOG_E("%s/%s: OPC UA server shutdown failed (%s)", __FILE__, __FUNCTION__, UA_StatusCode_name(shutdown_status));
    }
    *((UA_Boolean *)running) = false;
    UA_Server_delete(server_);
    server_ = NULL;
    pthread_mutex_unlock(&server_mutex_);

    LOG_I("🚪 UA Server exit status is '%s'", UA_StatusCode_name(status));
    return NULL;
}

bool ua_server_init(const UA_UInt16 port)
{
    pthread_mutex_lock(&server_mutex_);
    assert(NULL == server_);
    server_ = UA_Server_new();
    if (NULL == server_)
    {
        LOG_E("%s/%s: Failed to create OPC UA server", __FILE__, __FUNCTION__);
        pthread_mutex_unlock(&server_mutex_);
        return false;
    }
    UA_StatusCode status = UA_ServerConfig_setMinimal(UA_Server_getConfig(server_), port, NULL);
    if (UA_STATUSCODE_GOOD != status)
    {
        LOG_E(
            "%s/%s: Failed to configure OPC UA server on port %u (%s)",
            __FILE__,
            __FUNCTION__,
            port,
            UA_StatusCode_name(status));
        UA_Server_delete(server_);
        server_ = NULL;
        pthread_mutex_unlock(&server_mutex_);
        return false;
    }
    pthread_mutex_unlock(&server_mutex_);
    return true;
}

bool ua_server_run(pthread_t *thread_id, UA_Boolean *running)
{
    assert(NULL != thread_id);
    assert(NULL != running);

    int result = pthread_create(thread_id, NULL, run_ua_server, running);
    if (0 != result)
    {
        LOG_E("%s/%s: Failed to create thread (%s)", __FILE__, __FUNCTION__, strerror(result));
        pthread_mutex_lock(&server_mutex_);
        UA_Server_delete(server_);
        server_ = NULL;
        pthread_mutex_unlock(&server_mutex_);
        return false;
    }
    return true;
}

void ua_server_stop(UA_Boolean *running)
{
    assert(NULL != running);

    pthread_mutex_lock(&server_mutex_);
    *running = false;
    pthread_mutex_unlock(&server_mutex_);
}

void ua_server_add_bool(char *label, UA_Boolean state)
{
    UA_VariableAttributes attr = UA_VariableAttributes_default;

    assert(NULL != label);

    pthread_mutex_lock(&server_mutex_);
    assert(NULL != server_);
    UA_Variant_setScalar(&attr.value, &state, &UA_TYPES[UA_TYPES_BOOLEAN]);
    attr.description = UA_LOCALIZEDTEXT("en-US", label);
    attr.displayName = UA_LOCALIZEDTEXT("en-US", label);
    attr.dataType = UA_TYPES[UA_TYPES_BOOLEAN].typeId;
    attr.accessLevel = UA_ACCESSLEVELMASK_READ;
    UA_NodeId node_id = UA_NODEID_STRING(1, label);
    UA_QualifiedName name = UA_QUALIFIEDNAME(1, label);
    UA_NodeId parent_node_id = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    UA_NodeId parent_ref_node_id = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    UA_Server_addVariableNode(
        server_,
        node_id,
        parent_node_id,
        parent_ref_node_id,
        name,
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
        attr,
        NULL,
        NULL);
    pthread_mutex_unlock(&server_mutex_);
}

void ua_server_add_double(char *label, UA_Double value)
{
    UA_VariableAttributes attr = UA_VariableAttributes_default;

    assert(NULL != label);

    pthread_mutex_lock(&server_mutex_);
    assert(NULL != server_);
    UA_Variant_setScalar(&attr.value, &value, &UA_TYPES[UA_TYPES_DOUBLE]);
    attr.description = UA_LOCALIZEDTEXT("en-US", label);
    attr.displayName = UA_LOCALIZEDTEXT("en-US", label);
    attr.dataType = UA_TYPES[UA_TYPES_DOUBLE].typeId;
    attr.accessLevel = UA_ACCESSLEVELMASK_READ;
    UA_NodeId node_id = UA_NODEID_STRING(1, label);
    UA_QualifiedName name = UA_QUALIFIEDNAME(1, label);
    UA_NodeId parent_node_id = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    UA_NodeId parent_ref_node_id = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    UA_Server_addVariableNode(
        server_,
        node_id,
        parent_node_id,
        parent_ref_node_id,
        name,
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
        attr,
        NULL,
        NULL);
    pthread_mutex_unlock(&server_mutex_);
}

void ua_server_update_port(char *label, UA_Boolean state)
{
    UA_Variant newvalue;
    UA_NodeId current_node_id;

    assert(NULL != label);

    pthread_mutex_lock(&server_mutex_);
    assert(NULL != server_);
    UA_Variant_setScalar(&newvalue, &state, &UA_TYPES[UA_TYPES_BOOLEAN]);
    current_node_id = UA_NODEID_STRING(1, label);
    UA_Server_writeValue(server_, current_node_id, newvalue);
    pthread_mutex_unlock(&server_mutex_);
}

void ua_server_update_temp(char *label, UA_Double value)
{
    UA_Variant newvalue;
    UA_NodeId current_node_id;

    assert(NULL != label);

    pthread_mutex_lock(&server_mutex_);
    assert(NULL != server_);
    UA_Variant_setScalar(&newvalue, &value, &UA_TYPES[UA_TYPES_DOUBLE]);
    current_node_id = UA_NODEID_STRING(1, label);
    UA_Server_writeValue(server_, current_node_id, newvalue);
    pthread_mutex_unlock(&server_mutex_);
}