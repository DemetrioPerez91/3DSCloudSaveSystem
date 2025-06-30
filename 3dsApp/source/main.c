#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <inttypes.h>
#include "includes/config.h"
#include "includes/fileops.h"
#include "includes/network.h"
#include <3ds.h>


void downloadAvailableFilesList() 
{
	// Initialize HTTP
    httpcInit(0);

	// Make json download endpoint
	char saveFIlesEndPoint[256];
	snprintf(saveFIlesEndPoint, sizeof(saveFIlesEndPoint), "%s%s", SERVER_BASE_URL, SAVEFILES_ENDPOINT);

	Result ret = http_download_json(saveFIlesEndPoint);
	if (ret != 0 ){
		printf("Download JSON failed\n");
	} else {
		printf("Download JSON Complete\n");
	}
	
	httpcExit();
}

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


void downloadFileByName(const char *saveFilename) 
{
	// Initialize HTTP
    httpcInit(0);

	// Make file download endpoint
	char encoded[256];
	urlEncode(saveFilename, encoded, sizeof(encoded));

	char saveFilesEndPoint[512];
	snprintf(saveFilesEndPoint, sizeof(saveFilesEndPoint), "%s%s/%s", SERVER_BASE_URL, SAVEFILES_ENDPOINT, encoded);

	printf("FILE URL: %s\n", saveFilesEndPoint);
	Result ret = http_download(saveFilesEndPoint, saveFilename);
	if (ret != 0 ){
		printf("Download File failed\n");
	} else {
		printf("Download File Complete\n");
	}
	
	httpcExit();
}


void downloadFiles() {
	for (int i = 0; i < saveFileCount; i++) {
        printf(" Downloading: \n\%s\n", saveFiles[i].filename);
        downloadFileByName(saveFiles[i].filename);
    }
}


void uploadFileByName(const char *saveFilename) 
{
	// Initialize HTTP
    httpcInit(4 * 1024 * 1024);

	char filePath[512];
	snprintf(filePath, sizeof(filePath), "%s/%s", SAVE_FOLDER, saveFilename);

	char saveFilesEndPoint[512];
	snprintf(saveFilesEndPoint, sizeof(saveFilesEndPoint), "%s%s", SERVER_BASE_URL, SAVEFILES_ENDPOINT);


	printf("FILE LOCATION: %s\n", filePath);
	Result ret = http_upload_file(saveFilesEndPoint, filePath);
	if (ret != 0 ){
		printf("File upload failed\n");
	} else {
		printf("File upload Complete\n");
	}
	
	httpcExit();
}

int main()
{
	gfxInitDefault();

	consoleInit(GFX_BOTTOM,NULL);

	// ensureSaveFolderExists();
	// writeHelloFile();

	// downloadAvailableFilesList();

	// downloadFiles();

	uploadFileByName("test.txt");

	// Main loop
	while (aptMainLoop())
	{
		gspWaitForVBlank();
		hidScanInput();
		// Your code goes here

		u32 kDown = hidKeysDown();
		if (kDown & KEY_START)
			break; // break in order to return to hbmenu

	}

	gfxExit();
	return 0;
}

