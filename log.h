#ifndef LOG_H_
#define LOG_H_

#include <stdio.h>
#include <stdarg.h>

#define L_INFO 0
#define L_ERR 1
#define L_DBG 2

int log_start(const char* filename);
void log_msg(int type, const char* format, ... );
void log_end();

#endif