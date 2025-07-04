// ============================
// encoding.h
// ============================
#ifndef ENCODING_H
#define ENCODING_H

// Takes a string and encodes it so that it can be used as part of an URL
void urlEncode(const char *src, char *dest, size_t destSize);

#endif