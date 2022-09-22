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

#ifndef _OPCUA_OPEN62541_H_
#define _OPCUA_OPEN62541_H_

#include <open62541/server.h>

void ua_server_init(const UA_UInt16 port);
bool ua_server_run(pthread_t *thread_id, UA_Boolean *running);

void ua_server_add_bool(char *label, UA_Boolean state);
void ua_server_add_double(char *label, UA_Double value);
void ua_server_update_port(char *label, UA_Boolean state);
void ua_server_update_temp(char *label, UA_Double value);

#endif /* _OPCUA_OPEN62541_H_ */
