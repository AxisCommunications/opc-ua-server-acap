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
#include <stdlib.h>

#include "opcua_tempsensors.h"

void tempsensors_init(tempsensors_t **tempsensors, const size_t size)
{
    assert(NULL != tempsensors);
    assert(NULL != *tempsensors);
    (*tempsensors)->size = size;
    (*tempsensors)->subid = calloc(size, sizeof(uint32_t));
    (*tempsensors)->labels = calloc(size, sizeof(char *));
    for (int i = 0; i < (*tempsensors)->size; i++)
    {
        (*tempsensors)->labels[i] = calloc(TEMP_LABEL_LEN, sizeof(char));
    }
}

void tempsensors_free(tempsensors_t **tempsensors)
{
    if (NULL != tempsensors)
    {
        if (NULL != *tempsensors)
        {
            for (int i = 0; i < (*tempsensors)->size; i++)
            {
                free((*tempsensors)->labels[i]);
            }
            free((*tempsensors)->labels);
            free((*tempsensors)->subid);
        }
    }
}

char *tempsensors_get_label_from_subscription(tempsensors_t *tempsensors, const uint32_t subscription_id)
{
    assert(NULL != tempsensors);
    assert(NULL != tempsensors->labels);
    assert(NULL != tempsensors->subid);

    for (size_t i = 0; i < tempsensors->size; i++)
    {
        if (tempsensors->subid[i] == subscription_id)
        {
            // We have the match!
            return tempsensors->labels[i];
        }
    }
    return NULL;
}
