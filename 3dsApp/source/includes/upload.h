// network.h
#ifndef UPLOAD_H
#define UPLOAD_H

#include <3ds.h>
#include "config.h"
#include "fileops.h"

typedef struct {
    u8* data;
    u32 size;
    char filename[256];
} FileUploadPayload;


Result uploadFileToServer(const char *url, const char* filename, const char* fileData, u32 fileSize);
Result httpUploadFile(const char *url, const char *filepath);

#endif // NETWORK_H
