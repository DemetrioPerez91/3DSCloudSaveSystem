// savefile.c

#include "savefile.h"

int totalLines = 0;
int saveFileCount = 0;
char textBuffer[MAX_LINES][LINE_LENGTH];
SaveFile saveFiles[MAX_SAVES];