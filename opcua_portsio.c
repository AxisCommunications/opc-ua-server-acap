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
#include <stdlib.h>

#include "opcua_portsio.h"

void ports_init(ports_t **ports, const size_t size)
{
    assert(NULL != ports);
    assert(NULL != *ports);
    (*ports)->size = size;
    (*ports)->subid = calloc(size, sizeof(uint32_t));
    (*ports)->labels = calloc(size, sizeof(char *));
    for (int i = 0; i < (*ports)->size; i++)
    {
        (*ports)->labels[i] = calloc(PORT_LABEL_LEN, sizeof(char));
    }
}

void ports_free(ports_t **ports)
{
    if (NULL != ports)
    {
        if (NULL != *ports)
        {
            for (int i = 0; i < (*ports)->size; i++)
            {
                free((*ports)->labels[i]);
            }
            free((*ports)->labels);
            free((*ports)->subid);
        }
    }
}

char *ports_get_label_from_subscription(ports_t *ports, const uint32_t subscription_id)
{
    assert(NULL != ports);
    assert(NULL != ports->labels);
    assert(NULL != ports->subid);

    for (size_t i = 0; i < ports->size; i++)
    {
        // Remember: Device IO Port name (referenced in the manual) starts at 1
        if ( ports->subid[i] == subscription_id)
        {
            // We have a match!
            return ports->labels[i];
        }
    }
    return NULL;
}
