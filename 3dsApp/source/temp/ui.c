
// ui.c
#include <3ds.h>
#include <stdio.h>
#include "savefile.h"
#include "config.h"

void drawTextBuffer()
{
    consoleClear();
    int linesToShow = SCREEN_LINES;
    if (scrollOffset + linesToShow > totalLines)
        linesToShow = totalLines - scrollOffset;

    for (int i = 0; i < linesToShow; i++) {
        printf("%s\n", textBuffer[scrollOffset + i]);
    }
    printf("\n\x18/\x19 to scroll, START to exit");
}

void printSaveFiles()
{
    consoleClear();
    for (int i = 0; i < saveFileCount && i < SCREEN_LINES; i++) {
        SaveFile *s = &saveFiles[i];
        printf("%d. %s\n%lu bytes\n%s\n\n", i + 1, s->filename, s->size, s->lastModified);
    }
}