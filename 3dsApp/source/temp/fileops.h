// ============================
// fileops.h
// ============================
#ifndef FILEOPS_H
#define FILEOPS_H

void ensureSaveFolderExists();
void writeHelloFile();

#endif

// ============================// ============================
// config.h
// ============================
#ifndef CONFIG_H
#define CONFIG_H

#define SAVE_FOLDER "sdmc:/SaveFiles"
#define SERVER_BASE_URL "http://10.0.0.106:3000"
#define SAVEFILES_ENDPOINT "/gbaSaveFiles"
#define MAX_LINES 512
#define SCREEN_LINES 25
#define LINE_LENGTH 128
#define MAX_SAVES 128

#endif