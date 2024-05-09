#ifndef _STR_H_
#define _STR_H_

#include "str.h"
void str_trim_crlf(char *str);
void str_splist(const char *str, char *left, char *right,char c);
int str_all_space(const char *str);
void str_upper(char *str);
long long str_to_longlong(const char *str);
unsigned int str_octal_to_uint(const char *str);

#endif