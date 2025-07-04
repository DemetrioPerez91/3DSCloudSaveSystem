// ============================
// download.h
// ============================
#ifndef DOWNLOAD_H
#define DOWNLOAD_h

/// Shared HTTP download logic
static Result http_download_to_buffer(const char *url, u8 **outBuf, u32 *outSize);
// JSON download and parse
Result http_download_json(const char *url);
// File download
Result http_download(const char *url, const char *saveFilename);

#endif