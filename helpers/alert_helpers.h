#include <stdio.h>
#include <stdarg.h>

#define KRED  "\x1B[31m"
#define KGRN  "\x1B[32m"
#define KYELL  "\x1b[33m"
#define KNRM  "\x1B[0m"

extern void get_error_alert(const char *, ...);
extern void get_success_alert(const char *, ...);
extern void get_warning_alert(const char *, ...);
void get_colored_alert(const char *, const char *, va_list);
