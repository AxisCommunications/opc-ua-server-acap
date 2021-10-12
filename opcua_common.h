/*
 * Copyright (C) 2021 Axis Communications AB
 */

#ifndef _OPCUA_COMMON_H_
#define _OPCUA_COMMON_H_

#include <stdio.h>
#include <syslog.h>

// clang-format off
#define LOG(type, fmt, args...) { syslog(type, fmt, ##args); printf(fmt, ##args); printf("\n"); }
#define LOG_I(fmt, args...) { LOG(LOG_INFO, fmt, ##args) }
#define LOG_E(fmt, args...) { LOG(LOG_ERR, fmt, ##args) }
// clang-format on

#endif /* _OPCUA_COMMON_H_ */
