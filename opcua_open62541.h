/*
 * Copyright (C) 2021 Axis Communications AB
 */

#ifndef _OPCUA_OPEN62541_H_
#define _OPCUA_OPEN62541_H_

#include <open62541/server.h>

void ua_server_init(UA_Server *s);
bool ua_server_run(pthread_t *thread_id, UA_Boolean *running);
void ua_server_add_double(char *label, UA_Double value);
void ua_server_update_temp(char *label, UA_Double value);

#endif /* _OPCUA_OPEN62541_H_ */
