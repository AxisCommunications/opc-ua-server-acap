/*
 * Copyright (C) 2021 Axis Communications AB
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
