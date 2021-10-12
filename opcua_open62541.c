/*
 * Copyright (C) 2021 Axis Communications AB
 */

#include <open62541/server_config_default.h>
#include <pthread.h>

#include "opcua_common.h"
#include "opcua_open62541.h"

static UA_Server *server;

static void *run_ua_server(void *running)
{
    assert(NULL != server);
    assert(NULL != running);

    LOG_I("%s/%s: Starting UA server ...", __FILE__, __FUNCTION__);
    UA_StatusCode status = UA_Server_run(server, running);
    LOG_I("%s/%s: UA Server exit status: %s", __FILE__, __FUNCTION__, UA_StatusCode_name(status));
    return NULL;
}

void ua_server_init(UA_Server *s)
{
    assert(NULL != s);
    server = s;
    UA_ServerConfig_setDefault(UA_Server_getConfig(server));
}

bool ua_server_run(pthread_t *thread_id, UA_Boolean *running)
{
    assert(NULL != server);
    assert(NULL != thread_id);
    assert(NULL != running);

    int result = pthread_create(thread_id, NULL, run_ua_server, (void *)running);
    LOG_I("%s/%s: pthread_create result is %i", __FILE__, __FUNCTION__, result);
    if (0 != result)
    {
        LOG_E("%s/%s: Failed to create thread (%s)", __FILE__, __FUNCTION__, strerror(result));
        return false;
    }
    return true;
}

void ua_server_add_double(char *label, UA_Double value)
{
    assert(NULL != server);
    assert(NULL != label);

    // Define attributes
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    UA_Variant_setScalar(&attr.value, &value, &UA_TYPES[UA_TYPES_DOUBLE]);
    attr.description = UA_LOCALIZEDTEXT("en-US", label);
    attr.displayName = UA_LOCALIZEDTEXT("en-US", label);
    attr.dataType = UA_TYPES[UA_TYPES_DOUBLE].typeId;
    attr.accessLevel = UA_ACCESSLEVELMASK_READ;

    // Add the variable node to the information model
    UA_NodeId node_id = UA_NODEID_STRING(1, label);
    UA_QualifiedName name = UA_QUALIFIEDNAME(1, label);
    UA_NodeId parent_node_id = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    UA_NodeId parent_ref_node_id = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    UA_Server_addVariableNode(
        server,
        node_id,
        parent_node_id,
        parent_ref_node_id,
        name,
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
        attr,
        NULL,
        NULL);
}

void ua_server_update_temp(char *label, UA_Double value)
{
    assert(NULL != server);
    UA_Variant newvalue;
    UA_Variant_setScalar(&newvalue, &value, &UA_TYPES[UA_TYPES_DOUBLE]);
    UA_NodeId currentNodeId = UA_NODEID_STRING(1, label);
    UA_Server_writeValue(server, currentNodeId, newvalue);
}