// network.h
#ifndef NETWORK_H
#define NETWORK_H

#include <3ds.h>
#include "config.h"
#include "savefile.h"

typedef struct {
    u8* data;
    u32 size;
    char filename[256];
} FileUploadPayload;

Result http_download_json(const char *url);
Result http_download(const char *url, const char *saveFilename);
Result load_file_to_upload(const char* filepath, FileUploadPayload* outPayload);
Result http_upload_buffer(const char *url, const FileUploadPayload* payload);
Result http_upload_file(const char *url, const char *filepath);
Result http_upload_file_simple(const char *url, const char *filepath);

#endif // NETWORK_H
