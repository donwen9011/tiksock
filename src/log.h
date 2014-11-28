
int log_open(char *logfile, char *level_str);
int log_close();

void DEBUG(const char *fmt, ...);
void INFO(const char *fmt, ...);
void NOTICE(const char *fmt, ...);
void WARNING(const char *fmt, ...);
void ERROR(const char *fmt, ...);
void CRIT(const char *fmt, ...);
void ALERT(const char *fmt, ...);
void PANIC(const char *fmt, ...);

