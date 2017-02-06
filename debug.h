#ifndef _DEBUG_H
#define _DEBUG_H

#include <Arduino.h>

#define DEBUG(...) debug(__FILE__, __LINE__, __VA_ARGS__)

void debug(const char *, int, const char *);
void debug(const char *, int, const char *, const char *);
void debug(const char *, int, String &);
void debug(const char *, int, String &, String &);
#endif
