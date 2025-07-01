// network.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include "savefile.h"
#include "network.h"
#include "parser.h"

/// Shared HTTP download logic
static Result http_download_to_buffer(const char *url, u8 **outBuf, u32 *outSize) {
	Result ret = 0;
	httpcContext context;
	char *newurl = NULL;
	u32 statuscode = 0, contentsize = 0, readsize = 0, size = 0;
	u8 *buf = NULL, *lastbuf = NULL;

retry:
	ret = httpcOpenContext(&context, HTTPC_METHOD_GET, url, 1);
	if (ret) return ret;

	httpcSetSSLOpt(&context, SSLCOPT_DisableVerify);
	httpcSetKeepAlive(&context, HTTPC_KEEPALIVE_ENABLED);
	httpcAddRequestHeaderField(&context, "User-Agent", "3ds-client/1.0");
	httpcAddRequestHeaderField(&context, "Connection", "Keep-Alive");

	ret = httpcBeginRequest(&context);
	if (ret) goto cleanup;

	ret = httpcGetResponseStatusCode(&context, &statuscode);
	if (ret) goto cleanup;

	// Handle redirect
	if ((statuscode >= 301 && statuscode <= 303) || (statuscode >= 307 && statuscode <= 308)) {
		if (!newurl) newurl = (char*)malloc(0x1000);
		if (!newurl) { ret = -1; goto cleanup; }

		ret = httpcGetResponseHeader(&context, "Location", newurl, 0x1000);
		if (ret) goto cleanup;

		url = newurl;
		httpcCloseContext(&context);
		goto retry;
	}

	if (statuscode != 200) {
		printf("HTTP error: %lu\n", statuscode);
		ret = -2;
		goto cleanup;
	}

	ret = httpcGetDownloadSizeState(&context, NULL, &contentsize);
	if (ret) goto cleanup;

	buf = (u8*)malloc(0x1000);
	if (!buf) { ret = -1; goto cleanup; }

	do {
		ret = httpcDownloadData(&context, buf + size, 0x1000, &readsize);
		size += readsize;

		if (ret == (s32)HTTPC_RESULTCODE_DOWNLOADPENDING) {
			lastbuf = buf;
			buf = (u8*)realloc(buf, size + 0x1000);
			if (!buf) {
				free(lastbuf);
				ret = -1;
				goto cleanup;
			}
		}
	} while (ret == (s32)HTTPC_RESULTCODE_DOWNLOADPENDING);

	if (ret) goto cleanup;

	// Final resize + null-terminate
	lastbuf = buf;
	buf = (u8*)realloc(buf, size + 1);
	if (!buf) {
		free(lastbuf);
		ret = -1;
		goto cleanup;
	}
	buf[size] = '\0';

	*outBuf = buf;
	*outSize = size;

	httpcCloseContext(&context);
	if (newurl) free(newurl);
	return 0;

cleanup:
	httpcCloseContext(&context);
	if (buf) free(buf);
	if (newurl) free(newurl);
	return ret;
}

/// JSON download and parse
Result http_download_json(const char *url) {
	u8 *buf = NULL;
	u32 size = 0;

	printf("Requesting JSON from: \n%s\n", url);
	Result ret = http_download_to_buffer(url, &buf, &size);
	if (ret != 0) {
		printf("buffer download failed\n");
		return ret; 
	}

	// Print, store, parse
	formatAndStoreJson((char*)buf);
	parseSaveFilesFromTextBuffer();

	char *line = strtok((char*)buf, "\n");
	totalLines = 0;
	while (line && totalLines < MAX_LINES) {
		snprintf(textBuffer[totalLines], LINE_LENGTH, "%s", line);
		line = strtok(NULL, "\n");
		totalLines++;
	}

	free(buf);
	return 0;
}

/// File download
Result http_download(const char *url, const char *saveFilename) {
	u8 *buf = NULL;
	u32 size = 0;
	Result ret = http_download_to_buffer(url, &buf, &size);
	if (ret != 0) return ret;

	char path[256];
	snprintf(path, sizeof(path), "sdmc:/roms/gba/%s", saveFilename);

	FILE *f = fopen(path, "wb");
	if (!f) {
		printf("Failed to open %s for writing\n", path);
		free(buf);
		return -3;
	}
	fwrite(buf, 1, size, f);
	fclose(f);
	printf("Saved to %s (%lu bytes)\n", path, size);

	// Optional: Show on screen
	gfxFlushBuffers();
	gfxSwapBuffers();
	u8* framebuf_top = gfxGetFramebuffer(GFX_TOP, GFX_LEFT, NULL, NULL);
	memcpy(framebuf_top, buf, (size > 240 * 400 * 3 * 2) ? 240 * 400 * 3 * 2 : size);
	gfxFlushBuffers();
	gfxSwapBuffers();
	gspWaitForVBlank();

	free(buf);
	return 0;
}


/// Load file from SD card into memory
Result load_file_to_upload(const char* filepath, FileUploadPayload* outPayload) {
    FILE *f = fopen(filepath, "rb");
    if (!f) {
        printf("Failed to open file: %s\n", filepath);
        return -1;
    }

    fseek(f, 0, SEEK_END);
    long fileSize = ftell(f);
    rewind(f);

    u8 *data = (u8*)malloc(fileSize);
    if (!data) {
        fclose(f);
        printf("Failed to allocate file buffer\n");
        return -2;
    }

    fread(data, 1, fileSize, f);
    fclose(f);

    const char *filename = strrchr(filepath, '/');
    strncpy(outPayload->filename, filename ? filename + 1 : filepath, sizeof(outPayload->filename));
    outPayload->data = data;
    outPayload->size = fileSize;

    return 0;
}

/// Upload a buffer as a file to the HTTP server
Result http_upload_buffer(const char *url, const FileUploadPayload* payload) {
    Result ret = 0;
    httpcContext context;
    u32 statuscode = 0;
    const char *boundary = "------------------------3dsBoundary";

    // Build multipart body header and footer
    char header[512], footer[128];
    snprintf(header, sizeof(header),
        "--%s\r\n"
        "Content-Disposition: form-data; name=\"file\"; filename=\"%s\"\r\n"
        "Content-Type: application/octet-stream\r\n\r\n",
        boundary, payload->filename);
    snprintf(footer, sizeof(footer), "\r\n--%s--\r\n", boundary);

    u32 headerLen = (u32)strlen(header);
    u32 footerLen = (u32)strlen(footer);
    u32 bodySize = headerLen + payload->size + footerLen;

    // Round bodySize up to next multiple of 4
    u32 paddedBodySize = (bodySize + 3) & ~3;

    printf("Original body size: %u bytes\n", bodySize);
    printf("Padded body size: %u bytes\n", paddedBodySize);

    // Allocate 4-byte aligned buffer
    u8 *body = (u8*)memalign(4, paddedBodySize);
    if (!body) {
        printf("Failed to allocate aligned upload body\n");
        return -3;
    }

    // Copy header, payload data, footer into body buffer
    memcpy(body, header, headerLen);
    memcpy(body + headerLen, payload->data, payload->size);
    memcpy(body + headerLen + payload->size, footer, footerLen);

    // Zero pad remaining bytes if any
    if (paddedBodySize > bodySize) {
        memset(body + bodySize, 0, paddedBodySize - bodySize);
        printf("Added %u bytes of zero padding\n", paddedBodySize - bodySize);
    }

    // Create HTTP context
    ret = httpcOpenContext(&context, HTTPC_METHOD_POST, url, 1);
    if (ret) {
        printf("httpcOpenContext failed: 0x%08lX\n", ret);
        free(body);
        return ret;
    }

    httpcSetSSLOpt(&context, SSLCOPT_DisableVerify);
    httpcAddRequestHeaderField(&context, "User-Agent", "3ds-client/1.0");
    httpcAddRequestHeaderField(&context, "Connection", "Keep-Alive");

    // char contentTypeHeader[128];
    // snprintf(contentTypeHeader, sizeof(contentTypeHeader), "multipart/form-data; boundary=%s", boundary);
    // httpcAddRequestHeaderField(&context, "Content-Type", contentTypeHeader);
    httpcAddRequestHeaderField(&context, "Expect", "");

    ret = httpcBeginRequest(&context);
    if (ret) {
        printf("httpcBeginRequest failed: 0x%08lX\n", ret);
        httpcCloseContext(&context);
        free(body);
        return ret;
    }

    // Add post data (passing pointer cast to u32*, size must be multiple of 4)
    ret = httpcAddPostDataRaw(&context, (const u32*)body, paddedBodySize);
    free(body);

    if (ret) {
        printf("httpcAddPostDataRaw failed: 0x%08lX\n", ret);
        httpcCloseContext(&context);
        return ret;
    }

    ret = httpcGetResponseStatusCode(&context, &statuscode);
    if (ret) {
        printf("httpcGetResponseStatusCode failed: 0x%08lX\n", ret);
        httpcCloseContext(&context);
        return ret;
    }

    if (statuscode != 200) {
        printf("Upload failed with HTTP status code: %lu\n", statuscode);
    } else {
        printf("File '%s' uploaded successfully.\n", payload->filename);
    }

    httpcCloseContext(&context);
    return ret;
}


Result http_upload_file_simple(const char *url, const char *filepath) {
    Result ret = 0;
    httpcContext context;
    u32 statuscode = 0;
    FILE *file = NULL;
    u8 *fileData = NULL;
    size_t fileSize = 0;

    printf("Opening file: %s\n", filepath);
    file = fopen(filepath, "rb");
    if (!file) {
        printf("Failed to open file: %s\n", filepath);
        return -1;
    }

    fseek(file, 0, SEEK_END);
    fileSize = ftell(file);
    rewind(file);

    fileData = (u8*)malloc(fileSize);
    if (!fileData) {
        printf("Failed to allocate buffer\n");
        fclose(file);
        return -2;
    }

    fread(fileData, 1, fileSize, file);
    fclose(file);

    // Extract just the filename
    const char *filename = strrchr(filepath, '/');
    if (filename) filename++; else filename = filepath;

    printf("Uploading %s (%lu bytes) to %s\n", filename, fileSize, url);

    ret = httpcOpenContext(&context, HTTPC_METHOD_POST, url, 0);
    printf("httpcOpenContext: 0x%08lX\n", ret); if (ret) goto cleanup;

    httpcSetSSLOpt(&context, SSLCOPT_DisableVerify);
    httpcSetKeepAlive(&context, HTTPC_KEEPALIVE_ENABLED);
    httpcAddRequestHeaderField(&context, "User-Agent", "3ds-uploader/1.0");
    httpcAddRequestHeaderField(&context, "Connection", "Keep-Alive");
    httpcAddRequestHeaderField(&context, "Content-Type", "application/octet-stream");

    // Here's the actual binary file POST (field name is "file")
    ret = httpcAddPostDataBinary(&context, "file", fileData, fileSize);

    printf("httpcAddPostDataBinary: 0x%08lX\n", ret); if (ret) goto cleanup;

    ret = httpcBeginRequest(&context);
    printf("httpcBeginRequest: 0x%08lX\n", ret); if (ret) goto cleanup;

    ret = httpcGetResponseStatusCode(&context, &statuscode);
    printf("Status code: %lu (0x%08lX)\n", statuscode, statuscode);
    if (ret || statuscode != 200) {
        printf("Upload failed.\n");
        goto cleanup;
    }

    printf("Upload successful!\n");

cleanup:
    httpcCloseContext(&context);
    if (fileData) free(fileData);
    return ret;
}


/// Convenience wrapper for loading + uploading
Result http_upload_file(const char *url, const char *filepath) {
    FileUploadPayload payload;
    Result ret = http_upload_file_simple(url,filepath);
    //load_file_to_upload(filepath, &payload);
    if (ret != 0) return ret;

    // ret = http_upload_buffer(url, &payload);
    free(payload.data);
    return ret;
}
