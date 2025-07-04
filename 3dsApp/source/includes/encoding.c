// ============================
// encoding.h
// ============================
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Takes a string and encodes it so that it can be used as part of an URL
void urlEncode(const char *src, char *dest, size_t destSize) {
    static const char hex[] = "0123456789ABCDEF";
    size_t i = 0;

    while (*src && i + 3 < destSize) {
        char c = *src++;
        if (('a' <= c && c <= 'z') ||
            ('A' <= c && c <= 'Z') ||
            ('0' <= c && c <= '9') ||
            c == '-' || c == '_' || c == '.' || c == '~') {
            dest[i++] = c;
        } else {
            dest[i++] = '%';
            dest[i++] = hex[(c >> 4) & 0xF];
            dest[i++] = hex[c & 0xF];
        }
    }

    dest[i] = '\0';
}