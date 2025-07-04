#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <3ds.h>
#include "config.h"
#include "savefile.h"
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
	printf("----FINISHED DOWNLOADINGSS-----\n");
	return 0;
}
