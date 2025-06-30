// fileops.c
// ============================
#include <sys/stat.h>
#include <string.h>
#include <stdio.h>
#include "config.h"

void ensureSaveFolderExists() {
    struct stat st = {0};
    if (stat(SAVE_FOLDER, &st) == -1) {
        int res = mkdir(SAVE_FOLDER, 0777);
        if (res == 0) printf("Created folder: %s\n", SAVE_FOLDER);
        else printf("Failed to create folder: %s\n", SAVE_FOLDER);
    } else {
        printf("Folder already exists: %s\n", SAVE_FOLDER);
    }
}

void writeHelloFile() {
    char path[256];
    snprintf(path, sizeof(path), "%s/hello.txt", SAVE_FOLDER);

    FILE *f = fopen(path, "w");
    if (!f) {
        printf("Failed to create %s\n", path);
        return;
    }

    const char *msg = "HELLO FROM HTML";
    size_t written = fwrite(msg, 1, strlen(msg), f);
    fclose(f);

    if (written == strlen(msg)) printf("Successfully wrote to %s\n", path);
    else printf("Partial write to %s\n", path);
}
