// ============================
// parser.c
// ============================
#include <string.h>
#include <stdio.h>
#include "savefile.h"

void formatAndStoreJson(char *json) {
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
        } else {
            if (linelen < LINE_LENGTH - 1) line[linelen++] = c;
        }
        if (totalLines >= MAX_LINES - 1) break;
    }

    if (linelen > 0 && totalLines < MAX_LINES) {
        line[linelen] = '\0';
        snprintf(textBuffer[totalLines++], LINE_LENGTH, "%*s%s", indent * 2, "", line);
    }
}

void printSaveFileNames() {
    for (int i = 0; i < saveFileCount; i++) {
        printf("%s\n", saveFiles[i].filename);
    }
}

void parseSaveFilesFromTextBuffer() {
    saveFileCount = 0;
    int i = 0;

    while (i < totalLines && saveFileCount < MAX_SAVES) {
        if (strchr(textBuffer[i], '{')) {
            SaveFile s = {0};
            while (i < totalLines && !strchr(textBuffer[i], '}')) {
                char *line = textBuffer[i];
                char key[64], value[128];
                if (sscanf(line, " \"%[^\"]\": \"%[^\"]\"", key, value) == 2) {
                    if (strcmp(key, "filename") == 0) snprintf(s.filename, sizeof(s.filename), "%s", value);
                    else if (strcmp(key, "lastModified") == 0) snprintf(s.lastModified, sizeof(s.lastModified), "%s", value);
                } else if (sscanf(line, " \"%[^\"]\": %lf", key, &s.timestamp) == 2) {
                } else if (sscanf(line, " \"%[^\"]\": %d", key, &s.size) == 2) {
                }
                i++;
            }
            saveFiles[saveFileCount++] = s;
        }
        i++;
    }
    printf("\nParsed %d save files\n", saveFileCount);
    printSaveFileNames();
}
