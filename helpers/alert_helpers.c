#include "alert_helpers.h"


void get_error_alert(const char *format, ...){
    va_list args;
    va_start(args, format);

    get_colored_alert(format, KRED, args);
}

void get_success_alert(const char *format, ...){
    va_list args;
    va_start(args, format);

    get_colored_alert(format, KGRN, args);
}

void get_warning_alert(const char *format, ...){
    va_list args;
    va_start(args, format);

    get_colored_alert(format, KYELL, args);
};

void get_colored_alert(const char *format, const char *color_code, va_list args){

  printf("%s", color_code);
  vprintf(format, args);
  printf("%s", KNRM);
}
