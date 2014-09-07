#include "log.h"

FILE* logfile;

int log_start(const char* filename)
{
	logfile = fopen(filename, "w");

	if(logfile == NULL) return 1;
	return 0;
} 

void log_msg(int type, const char* format, ... )
{
	va_list args;

	if(type == L_ERR) fprintf(logfile, "[ERROR] ");
	if(type == L_DBG) fprintf(logfile, "[DEBUG] ");

	va_start(args, format);
	vfprintf(logfile, format, args);
	va_end(args);
	return;
}

void log_end()
{
	fclose(logfile);
	return;
}