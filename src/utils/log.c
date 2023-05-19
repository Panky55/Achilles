#include <time.h>
#include <stdio.h>
#include <stdarg.h>
#include <stddef.h>
#include <pthread.h>
#include <assert.h>

#include <utils/log.h>
#include <utils/ANSI-color-codes.h>

// mostly taken from https://github.com/palera1n/palera1n-c - thanks!

int loaderLog(log_level_t loglevel, const char *fname, int lineno, const char *fxname, const char *__restrict format, ...)
{
    pthread_mutex_t log_mutex;
    pthread_mutex_init(&log_mutex, NULL);
	int ret = 0;
	char type[0x10];
	char colour[0x10];
	char colour_bold[0x10];
	va_list logArgs;
	va_start(logArgs, format);
	switch (loglevel) {
	case LOG_FATAL:
		snprintf(type, 0x10, "%s", "Fatal");
		snprintf(colour, 0x10, "%s", RED);
		snprintf(colour_bold, 0x10, "%s", BRED);
		break;
	case LOG_ERROR:
		snprintf(type, 0x10, "%s", "Error");
		snprintf(colour, 0x10, "%s", RED);
		snprintf(colour_bold, 0x10, "%s", BRED);
		break;
	case LOG_WARNING:
		snprintf(type, 0x10, "%s", "Warning");
		snprintf(colour, 0x10, "%s", YEL);
		snprintf(colour_bold, 0x10, "%s", BYEL);
		break;
	case LOG_INFO:
		snprintf(type, 0x10, "%s", "Info");
		snprintf(colour, 0x10, "%s", CYN);
		snprintf(colour_bold, 0x10, "%s", BCYN);
		break;
    case LOG_SUCCESS:
        snprintf(type, 0x10, "%s", "Success");
        snprintf(colour, 0x10, "%s", GRN);
        snprintf(colour_bold, 0x10, "%s", BGRN);
        break;
	case LOG_DEBUG:
		// check if debug is enabled
		if (args[1].intVal == 0) {
			return 0;
		}
		snprintf(type, 0x10, "%s", "Debug");
		snprintf(colour, 0x10, "%s", MAG);
		snprintf(colour_bold, 0x10, "%s", BMAG);
		break;
	default:
		assert(loglevel >= 0);
		snprintf(type, 0x10, "%s", "Message");
		snprintf(colour, 0x10, "%s", WHT);
		snprintf(colour_bold, 0x10, "%s", BWHT);
		break;
	}
	{
		pthread_mutex_lock(&log_mutex);
		char timestring[0x80];
		time_t curtime;
		time(&curtime);
		struct tm* timeinfo = localtime(&curtime);
		snprintf(timestring, 0x80, "%s[%s%02d/%02d/%d %02d:%02d:%02d%s]", CRESET, HBLK, timeinfo->tm_mon + 1, timeinfo->tm_mday, timeinfo->tm_year - 100, timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec, CRESET);
		if (args[0].intVal == 3) {
			printf("%s%s%s <%s> " CRESET "%s" HBLU "%s"  CRESET ":" RED "%d" CRESET ":" BGRN "%s()" CRESET ":%s ", colour_bold, timestring, colour_bold, type, WHT, fname, lineno, fxname, colour_bold);
		} else if (args[0].intVal == 2) {
			printf("%s%s%s <%s>" CRESET ":%s ", colour_bold, timestring, colour_bold, type, colour_bold);
		} else {
			printf("%s<%s>%s: ", colour_bold, type, CRESET);
		}
		printf("%s", colour);
		ret = vprintf(format, logArgs);
		va_end(logArgs);
        printf(CRESET "\n");
		fflush(stdout);
	}
	pthread_mutex_unlock(&log_mutex);
	return ret;
}
