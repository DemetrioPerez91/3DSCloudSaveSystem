// network.h
#ifndef NETWORK_H
#define NETWORK_H

#include <3ds.h>

Result http_download_json(const char *url);
Result http_download(const char *url, const char *saveFilename);

#endif // NETWORK_H
