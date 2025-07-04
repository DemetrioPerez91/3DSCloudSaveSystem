#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <inttypes.h>
#include "includes/config.h"
#include "includes/savefile.h"
#include "includes/fileops.h"
#include "includes/upload.h"
#include "includes/encoding.h"
#include "includes/download.h"
#include <3ds.h>



// MARK: DOWNLOAD SECTION
void downloadAvailableFilesList() 
{
	// Initialize HTTP
    httpcInit(0);

	// Make json download endpoint
	char saveFIlesEndPoint[256];
	snprintf(saveFIlesEndPoint, sizeof(saveFIlesEndPoint), "%s%s", SERVER_PROD_URL, SAVEFILES_ENDPOINT);

	Result ret = http_download_json(saveFIlesEndPoint);
	if (ret != 0 ){
		printf("Download JSON failed\n");
	} else {
		printf("Download JSON Complete\n");
	}
	
	httpcExit();
}


void downloadFileByName(const char *saveFilename) 
{
	// Initialize HTTP
    httpcInit(0);

	// Make file download endpoint
	char encoded[256];
	urlEncode(saveFilename, encoded, sizeof(encoded));

	char saveFilesEndPoint[512];
	snprintf(saveFilesEndPoint, sizeof(saveFilesEndPoint), "%s%s/%s", SERVER_PROD_URL, SAVEFILES_ENDPOINT, encoded);

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

void downloadFilesSequence() {
	ensureSaveFolderExists();
	// downloadAvailableFilesList();
	downloadFileByName("Advance Wars 2 - Black Hole Rising (USA, Australia).sav");
	downloadFiles();
}


// MARK: UPLAOD FILE SECTION
void uploadFileByName(const char *saveFilename) 
{
	// Initialize HTTP
    httpcInit(4 * 1024 * 1024);

	char filePath[512];
	snprintf(filePath, sizeof(filePath), "%s/%s", SAVE_FOLDER, saveFilename);

	char saveFilesEndPoint[512];
	snprintf(saveFilesEndPoint, sizeof(saveFilesEndPoint), "%s%s", SERVER_PROD_URL, SAVEFILES_ENDPOINT);


	printf("FILE LOCATION: %s\n", filePath);
	Result ret = httpUploadFile(saveFilesEndPoint, filePath);
	if (ret != 0 ){
		printf("File upload failed\n");
	} else {
		printf("File upload Complete\n");
	}
	
	httpcExit();
}


// int main()
// {
// 	gfxInitDefault();

// 	consoleInit(GFX_BOTTOM,NULL);

	
	
	

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


// 	gfxExit();
// 	return 0;
// }

int main() {
    gfxInitDefault();
    consoleInit(GFX_BOTTOM, NULL);

    printf("Press UP to upload files\n");
    printf("Press DOWN to download files\n");
    printf("Press START to exit\n");

    while (aptMainLoop())
    {
        gspWaitForVBlank();
        hidScanInput();

        u32 kDown = hidKeysDown();

        if (kDown & KEY_START)
            break;

        if (kDown & KEY_DUP) {
            printf("Uploading files...\n");
            uploadFileByName("Advance Wars 2 - Black Hole Rising (USA, Australia).sav");
            printf("Upload complete.\n");
			printf("ALL DONE HERE, puch startt to exit");
        }

        if (kDown & KEY_DDOWN) {
            printf("Downloading files...\n");
            downloadFilesSequence();
            printf("Download complete.\n");

        }
    }

    gfxExit();
    return 0;
}