/*
 * Copyright (C) 2021 Axis Communications AB
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
