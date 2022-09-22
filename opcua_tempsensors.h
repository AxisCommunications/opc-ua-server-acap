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

#ifndef _OPCUA_TEMPSENSORS_H_
#define _OPCUA_TEMPSENSORS_H_

#include <stdint.h>
#include <stdio.h>

#define TEMP_LABEL_LEN 16
#define TEMP_LABEL_FMT "temperature %i"

typedef struct
{
    size_t size;
    uint32_t *subid;
    char **labels;
} tempsensors_t;

void tempsensors_init(tempsensors_t **tempsensors, const size_t size);
void tempsensors_free(tempsensors_t **tempsensors);
char *tempsensors_get_label_from_subscription(tempsensors_t *tempsensors, const uint32_t subscription_id);

#endif /* _OPCUA_TEMPSENSORS_H_ */
