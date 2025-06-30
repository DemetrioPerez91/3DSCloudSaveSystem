#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <inttypes.h>
#include <sys/stat.h>
#include <unistd.h>
#include <3ds.h>

#define MAX_LINES 512
#define SCREEN_LINES 25
#define LINE_LENGTH 128
#define MAX_SAVES 128

// Save file structure
typedef struct {
	char filename[128];
	double timestamp;
	char lastModified[64];
	int size;
} SaveFile;

SaveFile saveFiles[MAX_SAVES];
int saveFileCount = 0;

char textBuffer[MAX_LINES][LINE_LENGTH];
int totalLines = 0;
int scrollOffset = 0;

void ensureSaveFolderExists()
{
    const char *folder = "sdmc:/SaveFiles";
    struct stat st = {0};

    if (stat(folder, &st) == -1) {
        int res = mkdir(folder, 0777);
        if (res == 0) {
            printf("Created folder: %s\n", folder);
        } else {
            printf("Failed to create folder: %s\n", folder);
        }
    } else {
        printf("Folder already exists: %s\n", folder);
    }
}


void formatAndStoreJson(char *json)
{
	int indent = 0;
	char line[LINE_LENGTH] = {0};
	int linelen = 0;

	totalLines = 0;

	for (size_t i = 0; json[i] != '\0'; ++i) {
		char c = json[i];

		if (c == '{' || c == '[') {
			if (linelen > 0) {
				line[linelen] = '\0';
				snprintf(textBuffer[totalLines++], LINE_LENGTH, "%s", line);
				linelen = 0;
			}
			memset(line, 0, LINE_LENGTH);
			snprintf(textBuffer[totalLines++], LINE_LENGTH, "%*s%c", indent * 2, "", c);
			indent++;
		} else if (c == '}' || c == ']') {
			if (linelen > 0) {
				line[linelen] = '\0';
				snprintf(textBuffer[totalLines++], LINE_LENGTH, "%s", line);
				linelen = 0;
			}
			indent--;
			memset(line, 0, LINE_LENGTH);
			snprintf(textBuffer[totalLines++], LINE_LENGTH, "%*s%c", indent * 2, "", c);
		} else if (c == ',') {
			line[linelen++] = c;
			line[linelen] = '\0';
			snprintf(textBuffer[totalLines++], LINE_LENGTH, "%*s%s", indent * 2, "", line);
			linelen = 0;
			memset(line, 0, LINE_LENGTH);
		} else if (c == ':') {
			line[linelen++] = c;
			line[linelen++] = ' ';
		} else if (c == '\n' || c == '\r') {
			// ignore original newlines
		} else {
			if (linelen < LINE_LENGTH - 1) {
				line[linelen++] = c;
			}
		}

		if (totalLines >= MAX_LINES - 1) break;
	}

	if (linelen > 0 && totalLines < MAX_LINES) {
		line[linelen] = '\0';
		snprintf(textBuffer[totalLines++], LINE_LENGTH, "%*s%s", indent * 2, "", line);
	}
}


void parseSaveFilesFromTextBuffer()
{
	saveFileCount = 0;
	int i = 0;

	while (i < totalLines && saveFileCount < MAX_SAVES) {
		if (strchr(textBuffer[i], '{')) {
			SaveFile s;
			memset(&s, 0, sizeof(SaveFile));

			while (i < totalLines && !strchr(textBuffer[i], '}')) {
				char *line = textBuffer[i];
				char key[64], value[128];

				if (sscanf(line, " \"%[^\"]\": \"%[^\"]\"", key, value) == 2) {
					if (strcmp(key, "filename") == 0) {
						snprintf(s.filename, sizeof(s.filename), "%s", value);
					} else if (strcmp(key, "lastModified") == 0) {
						snprintf(s.lastModified, sizeof(s.lastModified), "%s", value);
					}
				} else if (sscanf(line, " \"%[^\"]\": %lf", key, &s.timestamp) == 2) {
					// Parsed timestamp
				} else if (sscanf(line, " \"%[^\"]\": %d", key, &s.size) == 2) {
					// Parsed size
				}
				i++;
			}

			saveFiles[saveFileCount++] = s;
		}
		i++;
	}

	printf("\nParsed %d save files\n", saveFileCount);
}

Result http_download_json(const char *url)
{
	Result ret = 0;
	httpcContext context;
	char *newurl = NULL;
	u32 statuscode = 0, contentsize = 0, readsize = 0, size = 0;
	u8 *buf = NULL, *lastbuf = NULL;

	printf("Requesting: %s\n", url);

	// HTTP initialization
	do {
		ret = httpcOpenContext(&context, HTTPC_METHOD_GET, url, 1);
		if (ret) break;

		httpcSetSSLOpt(&context, SSLCOPT_DisableVerify);
		httpcSetKeepAlive(&context, HTTPC_KEEPALIVE_ENABLED);
		httpcAddRequestHeaderField(&context, "User-Agent", "3ds-json-client/1.0");
		httpcAddRequestHeaderField(&context, "Connection", "Keep-Alive");

		ret = httpcBeginRequest(&context);
		if (ret) break;

		ret = httpcGetResponseStatusCode(&context, &statuscode);
		if (ret) break;

		if ((statuscode >= 301 && statuscode <= 303) || (statuscode >= 307 && statuscode <= 308)) {
			if (!newurl) newurl = (char*)malloc(0x1000);
			if (!newurl) { ret = -1; break; }

			ret = httpcGetResponseHeader(&context, "Location", newurl, 0x1000);
			if (ret) break;

			url = newurl;
			httpcCloseContext(&context);
			continue;
		}
	} while ((statuscode >= 301 && statuscode <= 303) || (statuscode >= 307 && statuscode <= 308));

	if (ret || statuscode != 200) {
		printf("HTTP error: %lu\n", statuscode);
		httpcCloseContext(&context);
		if (newurl) free(newurl);
		return -1;
	}

	ret = httpcGetDownloadSizeState(&context, NULL, &contentsize);
	if (ret) {
		httpcCloseContext(&context);
		if (newurl) free(newurl);
		return ret;
	}

	buf = (u8*)malloc(0x1000);
	if (!buf) {
		httpcCloseContext(&context);
		if (newurl) free(newurl);
		return -1;
	}

	do {
		ret = httpcDownloadData(&context, buf + size, 0x1000, &readsize);
		size += readsize;

		if (ret == (s32)HTTPC_RESULTCODE_DOWNLOADPENDING) {
			lastbuf = buf;
			buf = (u8*)realloc(buf, size + 0x1000);
			if (!buf) {
				httpcCloseContext(&context);
				free(lastbuf);
				if (newurl) free(newurl);
				return -1;
			}
		}
	} while (ret == (s32)HTTPC_RESULTCODE_DOWNLOADPENDING);

	if (ret) {
		httpcCloseContext(&context);
		free(buf);
		if (newurl) free(newurl);
		return ret;
	}

	buf = (u8*)realloc(buf, size + 1);
	buf[size] = '\0'; // Null-terminate the JSON

	// Print JSON to console
	// printf("\n--- JSON RESPONSE ---\n%s\n---------------------\n", buf);
	// Parse and store each line into the text buffer
	formatAndStoreJson((char*)buf);
	parseSaveFilesFromTextBuffer();
	char *line = strtok((char*)buf, "\n");
	totalLines = 0;
	while (line && totalLines < MAX_LINES) {
		snprintf(textBuffer[totalLines], LINE_LENGTH, "%s", line);
		line = strtok(NULL, "\n");
		totalLines++;
	}

	httpcCloseContext(&context);
	free(buf);
	if (newurl) free(newurl);
	return 0;
}


// Result http_download(const char *url)
Result http_download(const char *url, const char *saveFilename)
{
	Result ret=0;
	httpcContext context;
	char *newurl=NULL;
	u8* framebuf_top;
	u32 statuscode=0;
	u32 contentsize=0, readsize=0, size=0;
	u8 *buf, *lastbuf;

	printf("Downloading %s\n",url);

	do {
		ret = httpcOpenContext(&context, HTTPC_METHOD_GET, url, 1);
		printf("return from httpcOpenContext: %" PRId32 "\n",ret);

		// This disables SSL cert verification, so https:// will be usable
		ret = httpcSetSSLOpt(&context, SSLCOPT_DisableVerify);
		printf("return from httpcSetSSLOpt: %" PRId32 "\n",ret);

		// Enable Keep-Alive connections
		ret = httpcSetKeepAlive(&context, HTTPC_KEEPALIVE_ENABLED);
		printf("return from httpcSetKeepAlive: %" PRId32 "\n",ret);

		// Set a User-Agent header so websites can identify your application
		ret = httpcAddRequestHeaderField(&context, "User-Agent", "httpc-example/1.0.0");
		printf("return from httpcAddRequestHeaderField: %" PRId32 "\n",ret);

		// Tell the server we can support Keep-Alive connections.
		// This will delay connection teardown momentarily (typically 5s)
		// in case there is another request made to the same server.
		ret = httpcAddRequestHeaderField(&context, "Connection", "Keep-Alive");
		printf("return from httpcAddRequestHeaderField: %" PRId32 "\n",ret);

		ret = httpcBeginRequest(&context);
		if(ret!=0){
			httpcCloseContext(&context);
			if(newurl!=NULL) free(newurl);
			return ret;
		}

		ret = httpcGetResponseStatusCode(&context, &statuscode);
		if(ret!=0){
			httpcCloseContext(&context);
			if(newurl!=NULL) free(newurl);
			return ret;
		}

		if ((statuscode >= 301 && statuscode <= 303) || (statuscode >= 307 && statuscode <= 308)) {
			if(newurl==NULL) newurl = (char*)malloc(0x1000); // One 4K page for new URL
			if (newurl==NULL){
				httpcCloseContext(&context);
				return -1;
			}
			ret = httpcGetResponseHeader(&context, "Location", newurl, 0x1000);
			url = newurl; // Change pointer to the url that we just learned
			printf("redirecting to url: %s\n",url);
			httpcCloseContext(&context); // Close this context before we try the next
		}
	} while ((statuscode >= 301 && statuscode <= 303) || (statuscode >= 307 && statuscode <= 308));

	if(statuscode!=200){
		printf("URL returned status: %" PRId32 "\n", statuscode);
		httpcCloseContext(&context);
		if(newurl!=NULL) free(newurl);
		return -2;
	}

	// This relies on an optional Content-Length header and may be 0
	ret=httpcGetDownloadSizeState(&context, NULL, &contentsize);
	if(ret!=0){
		httpcCloseContext(&context);
		if(newurl!=NULL) free(newurl);
		return ret;
	}

	printf("reported size: %" PRId32 "\n",contentsize);

	// Start with a single page buffer
	buf = (u8*)malloc(0x1000);
	if(buf==NULL){
		httpcCloseContext(&context);
		if(newurl!=NULL) free(newurl);
		return -1;
	}

	do {
		// This download loop resizes the buffer as data is read.
		ret = httpcDownloadData(&context, buf+size, 0x1000, &readsize);
		size += readsize; 
		if (ret == (s32)HTTPC_RESULTCODE_DOWNLOADPENDING){
				lastbuf = buf; // Save the old pointer, in case realloc() fails.
				buf = (u8*)realloc(buf, size + 0x1000);
				if(buf==NULL){ 
					httpcCloseContext(&context);
					free(lastbuf);
					if(newurl!=NULL) free(newurl);
					return -1;
				}
			}
	} while (ret == (s32)HTTPC_RESULTCODE_DOWNLOADPENDING);	

	if(ret!=0){
		httpcCloseContext(&context);
		if(newurl!=NULL) free(newurl);
		free(buf);
		return -1;
	}

	// Resize the buffer back down to our actual final size
	lastbuf = buf;
	buf = (u8*)realloc(buf, size);
	if(buf==NULL){ // realloc() failed.
		httpcCloseContext(&context);
		free(lastbuf);
		if(newurl!=NULL) free(newurl);
		return -1;
	}

	printf("downloaded size: %" PRId32 "\n",size);

	if(size>(240*400*3*2))size = 240*400*3*2;

	// framebuf_top = gfxGetFramebuffer(GFX_TOP, GFX_LEFT, NULL, NULL);
	// memcpy(framebuf_top, buf, size);
///----------------------------------------------------------------------
	char path[256];
	snprintf(path, sizeof(path), "sdmc:/SaveFiles/%s", saveFilename);

	FILE *f = fopen(path, "wb");
	if (!f) {
		printf("Failed to open %s for writing\n", path);
		free(buf);
		httpcCloseContext(&context);
		if (newurl) free(newurl);
		return -3;
	}

	fwrite(buf, 1, size, f);
	fclose(f);
	printf("Saved to %s (%lu bytes)\n", path, size);
///----------------------------------------------------------------------
	gfxFlushBuffers();
	gfxSwapBuffers();

	framebuf_top = gfxGetFramebuffer(GFX_TOP, GFX_LEFT, NULL, NULL);
	memcpy(framebuf_top, buf, size);

	gfxFlushBuffers();
	gfxSwapBuffers();
	gspWaitForVBlank();

	httpcCloseContext(&context);
	free(buf);
	if (newurl!=NULL) free(newurl);

	return 0;
}

void drawTextBuffer()
{
	consoleClear();
	int linesToShow = SCREEN_LINES;
	if (scrollOffset + linesToShow > totalLines)
		linesToShow = totalLines - scrollOffset;

	for (int i = 0; i < linesToShow; i++) {
		printf("%s\n", textBuffer[scrollOffset + i]);
	}
	printf("\n↑/↓ to scroll, START to exit");
}

void printSaveFiles()
{
	consoleClear();
	for (int i = 0; i < saveFileCount && i < SCREEN_LINES; i++) {
		SaveFile *s = &saveFiles[i];
		printf("%d. %s\n%lu bytes\n%s\n\n", i + 1, s->filename, s->size, s->lastModified);
	}
}

void url_encode(const char *src, char *dest, int destsize)
{
	const char *hex = "0123456789ABCDEF";
	int i = 0, j = 0;

	while (src[i] && j < destsize - 1) {
		char c = src[i];
		if ((c >= 'A' && c <= 'Z') ||
		    (c >= 'a' && c <= 'z') ||
		    (c >= '0' && c <= '9') ||
		    c == '-' || c == '_' || c == '.' || c == '~') {
			dest[j++] = c;
		} else {
			if (j + 3 >= destsize) break;
			dest[j++] = '%';
			dest[j++] = hex[(c >> 4) & 0xF];
			dest[j++] = hex[c & 0xF];
		}
		i++;
	}
	dest[j] = '\0';
}

void writeHelloFile()
{
    const char *folder = "sdmc:/SaveFiles";
    const char *filename = "hello.txt";
    char fullpath[256];

    snprintf(fullpath, sizeof(fullpath), "%s/%s", folder, filename);

    FILE *f = fopen(fullpath, "w");
    if (!f) {
        printf("Failed to create %s\n", fullpath);
        return;
    }

    const char *message = "HELLO FROM HTML";
    size_t written = fwrite(message, 1, strlen(message), f);
    fclose(f);

    if (written == strlen(message)) {
        printf("Successfully wrote to %s\n", fullpath);
    } else {
        printf("Partial write to %s\n", fullpath);
    }
}


int main()
{
	gfxInitDefault();
	httpcInit(0);
	consoleInit(GFX_BOTTOM, NULL);

	// Your new endpoint
	Result ret = http_download_json("http://10.0.0.106:3000/gbaSaveFiles");

	writeHelloFile();
	drawTextBuffer();
	printf("Download result: %" PRId32 "\n", ret);
	printSaveFiles();
	if (saveFileCount > 0) {
		char encodedFilename[256];
		char fullUrl[512];

		url_encode(saveFiles[0].filename, encodedFilename, sizeof(encodedFilename));
		snprintf(fullUrl, sizeof(fullUrl), "http://10.0.0.106:3000/gbaSaveFiles/%s", encodedFilename);

		printf("\nDownloading first save file:\n%s\n", saveFiles[0].filename);
		ensureSaveFolderExists();
		Result downloadResult = http_download(fullUrl, saveFiles[0].filename);
		printf("Download result: %" PRId32 "\n", downloadResult);
	}

	while (aptMainLoop()) {
	gspWaitForVBlank();
	hidScanInput();

	u32 kDown = hidKeysDown();
	u32 kHeld = hidKeysHeld();

	if (kDown & KEY_START)
		break;

	if (kDown & KEY_DOWN && scrollOffset + SCREEN_LINES < totalLines) {
		scrollOffset++;
		drawTextBuffer();
	}

	if (kDown & KEY_UP && scrollOffset > 0) {
		scrollOffset--;
		drawTextBuffer();
	}
}


	httpcExit();
	gfxExit();
	return 0;
}
// int main()
// {
// 	Result ret=0;
// 	gfxInitDefault();
// 	httpcInit(0); // Buffer size when POST/PUT.

// 	consoleInit(GFX_BOTTOM,NULL);

// 	// ret=http_download("http://devkitpro.org/misc/httpexample_rawimg.rgb");
// 	ret=http_download("http://10.0.0.106/image/ichicka.rgb");
// 	// Try the following for redirection to the above URL.
// 	// ret=http_download("http://tinyurl.com/hd8jwqx");
// 	printf("return from http_download: %" PRId32 "\n",ret);

// 	// Main loop
// 	while (aptMainLoop())
// 	{
// 		gspWaitForVBlank();
// 		hidScanInput();

// 		// Your code goes here

// 		u32 kDown = hidKeysDown();
// 		if (kDown & KEY_START)
// 			break; // break in order to return to hbmenu

// 	}

// 	// Exit services
// 	httpcExit();
// 	gfxExit();
// 	return 0;
// }

