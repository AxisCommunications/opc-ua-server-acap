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

#ifndef _OPCUA_PORTSIO_H_
#define _OPCUA_PORTSIO_H_

#include <stdint.h>
#include <stdio.h>

#define PORT_LABEL_LEN 8
#define PORT_LABEL_FMT "port %i"

typedef struct
{
    size_t size;
    uint32_t *subid;
    char **labels;
} ports_t;

void ports_init(ports_t **ports, const size_t size);
void ports_free(ports_t **ports);
char *ports_get_label_from_subscription(ports_t *ports, const uint32_t subscription_id);

#endif /* _OPCUA_PORTSIO_H_ */
