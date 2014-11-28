
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
#include <pthread.h>
#include "log.h"

static FILE *logfd;
static int   level;
static pthread_mutex_t log_lock = PTHREAD_MUTEX_INITIALIZER;
static char *timeformat = "%Z %b %e %T";

enum LOG_LEVEL {
	LOG_PANIC,
	LOG_ALERT,
	LOG_CRIT,
	LOG_ERROR,
	LOG_WARNING,
	LOG_NOTICE,
	LOG_INFO,
	LOG_DEBUG
};

char * log_level_str[] = {
	"PANIC",
	"ALERT",
	"CRIT",
	"ERROR",
	"WARNING",
	"NOTICE",
	"INFO",
	"DEBUG"
};

static char *timestamp(char *ts, int size)
{
	time_t now;
	struct tm tm;

	time(&now);
	localtime_r(&now, &tm);

	memset(ts, 0, size);
	strftime(ts, size, timeformat, &tm); 

	return ts;
}

int log_open(char *log_file, char* log_level)
{
	int i;

	logfd = fopen(log_file, "a");
	if (!logfd) {
		return 0;
	}

	level = LOG_INFO;

	if (log_level) {
		for (i = 0; i < sizeof(log_level_str) / sizeof(char *); i++) {
			if (strcasecmp(log_level, log_level_str[i]) == 0) {
				level = i;
				break;
			}
		}
	}

	return 1;
}

int log_close()
{
	fclose(logfd);
}

static void log_print(int l, const char *fmt, va_list ap)
{
	char ts[32];

	if (l > level)
		return;

	pthread_mutex_lock(&log_lock);

	fprintf(logfd, "[%s] %s : ",  timestamp(ts, 32), log_level_str[l]);
	vfprintf(logfd, fmt, ap);
	fprintf(logfd, "\n");
	fflush(logfd);

	pthread_mutex_unlock(&log_lock);
}

void DEBUG(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	log_print(LOG_DEBUG, fmt, ap);
	va_end(ap);
}


void INFO(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	log_print(LOG_INFO, fmt, ap);
	va_end(ap);
}

void NOTICE(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	log_print(LOG_NOTICE, fmt, ap);
	va_end(ap);
}

void WARNING(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	log_print(LOG_WARNING, fmt, ap);
	va_end(ap);
}

void ERROR(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	log_print(LOG_ERROR, fmt, ap);
	va_end(ap);
}

void CRIT(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	log_print(LOG_CRIT, fmt, ap);
	va_end(ap);
}

void ALERT(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	log_print(LOG_ALERT, fmt, ap);
	va_end(ap);
}

void PANIC(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	log_print(LOG_PANIC, fmt, ap);
	va_end(ap);

	exit(-1);
}


