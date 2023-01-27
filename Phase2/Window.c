#include "../Phase1/functions.c"

#ifndef _STDIO_H
#include <stdio.h>
#endif

#ifndef _CONIO_H_
#include <conio.h>
#endif

#ifndef _WINDOWS_H
#include <windows.h>
#endif

#ifndef _MATH_H
#include <math.h>
#endif

#define COLOR(bg, fg) (bg * 16 + fg)

const WORD TEXT_COLOR = COLOR(0, 15);
const WORD HEAD_COLOR = COLOR(7, 0);
const WORD SIDE_COLOR = COLOR(8, 0);
const WORD ACTV_COLOR = COLOR(0, 8);
const WORD DESC_COLOR = COLOR(8, 0);
const WORD CLPR_COLOR = COLOR(13, 15);
const WORD STATECOLOR[] = {COLOR(3, 0), COLOR(2, 0), COLOR(14, 0)};

enum STATE
{
    NORMAL,
    INSERT,
    VISUAL
};

const char *STATESTR[] =
    {
        " NORMAL ",
        " INSERT ",
        " VISUAL "};

/// @brief Moves cursor to x, y
/// @param X
/// @param Y
void setCursorPos(int X, int Y)
{
    COORD pos = {X, Y};
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), pos);
}

/// @brief Gets size of terminal
/// @param col number of columns
/// @param row number of rows
void scrsize(int *col, int *row)
{
    CONSOLE_SCREEN_BUFFER_INFO csbi;

    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);

    *col = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    *row = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
}

/// @brief Changes colors to text mode
void totext()
{
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), TEXT_COLOR);
}

/// @brief Changes colors to header mode
void tohead()
{
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), HEAD_COLOR);
}

/// @brief Changes colors to side mode
void toside()
{
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), SIDE_COLOR);
}

/// @brief Changes colors to active mode
void toactv()
{
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), ACTV_COLOR);
}

/// @brief Changes colors to description mode
void todesc()
{
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), DESC_COLOR);
}

/// @brief Changes colors to command line prefix mode
void toclpr()
{
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), CLPR_COLOR);
}

/// @brief Changes color to state mode
/// @param state
void tostate(enum STATE state)
{
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), STATECOLOR[state]);
}

/// @brief Prints header
/// @param name name of file
/// @param saved non-zero if the file is saved
void header(const char *name, int saved)
{
    // COORD pos = {0, 0};
    // SetConsoleCursorPosition(output, pos);

    int col, row, startix, endix;
    scrsize(&col, &row);

    startix = col / 2 - strlen(name);
    endix = startix + strlen(name);

    tohead();

    for (int i = 0; i < col; i++)
    {
        if (i < startix || i > endix)
            printf(" ");
        else if (i == endix && !saved)
            printf("+");
        else
            printf("%c", name[i - startix]);
    }

    totext();
    printf("\n");
}

/// @brief Prints sidebar
/// @param maxrow number of rows in the file
/// @param startrow first row number
/// @param endrow last row number (won't be printed)
/// @param scrrow number of rows in the screen
/// @param activestart first active row index
/// @param activeend last active row index
/// @return length of sidebar
int sidebar(int maxrow, int startrow, int endrow, int scrrow, int activestart, int activeend)
{
    int maxlen = (int)ceil(log10(maxrow));

    char format[] = "%0d ";
    format[1] = (maxlen + '0');

    char formatp[maxlen + 1];
    for (int i = 0; i < maxlen + 1; i++)
    {
        if (i == maxlen / 2)
            formatp[0] = '~';
        else
            formatp[i] = ' ';
    }

    COORD pos = {0, 0};

    for (int i = startrow; i < scrrow - 2; i++)
    {
        pos.Y = i - startrow + 1;
        SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), pos);
        if (i < endrow)
        {
            if (i >= activestart && i < activeend)
                toactv();
            else
                toside();
            printf(format, i);
        }
        else
        {
            totext();
            printf(formatp);
        }
    }
    totext();

    return maxlen + 1;
}

/// @brief Prints a line
/// @param line string
/// @param lineno line number (0 based)
/// @param startcol starting index of the string
/// @param endcol last index of the string (safe if endcol > size of line)
/// @param sidelength width of sidebar
void printline(char *line, int lineno, int startcol, int endcol, int sidelength)
{
    int row, col;
    scrsize(&col, &row);

    COORD pos = {sidelength + 1, lineno + 1};
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), pos);
    totext();

    int n = strlen(line);

    for (int i = startcol; i < endcol && i < n && i < col - sidelength - 1; i++)
    {
        printf("%c", line[i]);
    }
}

/// @brief Prints output to screen
/// @param sidelen length of sidebar
/// @param start starting row
/// @param end last row
void printOutputToScr(int sidelen, int start, int end, int startcol, int endcol)
{
    char cwd[512];
    getcwd(cwd, 512);

    chdir(parentDir);

    OUTPUT = fopen(outputPath, "r");

    char c = fgetc(OUTPUT);

    int X = sidelen + 1, Y = 1;

    setCursorPos(X, Y);
    X = 0;

    while (c != EOF)
    {
        if (c == '\n' || c == '\0' || c == EOF)
        {
            Y++;
            X = sidelen + 1;
            setCursorPos(X, Y);
            X = 0;
            c = fgetc(OUTPUT);
            continue;
        }

        if (c == '~')
        {
            c = fgetc(OUTPUT);
            if (c == '!')
            {
                __red__();
                c = fgetc(OUTPUT);
                continue;
            }
            else if (c == '?')
            {
                __cyan__();
                c = fgetc(OUTPUT);
                continue;
            }
            else if (c == '&')
            {
                __red__();
                c = fgetc(OUTPUT);
                continue;
            }
            else
            {
                if (Y >= start && Y < end && X >= startcol && X < endcol)
                    printf("~");
                X++;
            }
        }

        if (Y >= start && Y < end && X >= startcol && X < endcol){
            printf("%c", c);
        }

        X++;
        c = fgetc(OUTPUT);
    }

    fclose(OUTPUT);

    chdir(cwd);
}

/// @brief Prints state
/// @param state
/// @param desc description
/// @param scrlen screen width
void printState(enum STATE state, const char *desc, int scrlen)
{
    tostate(state);
    printf("%s", STATESTR[state]);
    todesc();
    printf(" ");

    int n = strlen(desc);
    for (int i = 0; i < scrlen - (strlen(STATESTR[state]) + 2); i++)
    {
        if (i < n)
            printf("%c", desc[i]);
        else
            printf(" ");
    }

    totext();
}

/// @brief Prints command line
void printCL()
{
    toclpr();
    printf(" Command Line: ");
    totext();
}

/// @brief Prints footer
/// @param state
/// @param desc description in front of footer
/// @param row screen size in rows
/// @param col screen size in columns
void footer(enum STATE state, const char *desc, int row, int col)
{
    COORD pos = {0, row - 2};
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), pos);
    printState(state, desc, col);

    pos.Y++;
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), pos);
    printCL();
}

/// @brief Initializes a screen
/// @param fp file
/// @param filename name of file
/// @param saved non-zero if the file is saved
/// @param maxrow maximum number of rows in the file
/// @param startrow starting row
/// @param endrow last row
/// @param startcol starting column of output
/// @param endcol last column of output
/// @param activestart first active row index
/// @param activeend last active row index
/// @param state
/// @param desc description in front of state
void initscr(
    char *path, const char *filename, int saved,
    int maxrow, int startrow, int endrow, int startcol, int endcol, int activestart, int activeend,
    enum STATE state, const char *desc)
{
    system("cls");
    int rows, cols;
    scrsize(&cols, &rows);

    header(filename, saved);
    int sl = sidebar(maxrow, startrow, endrow, rows, activestart, activeend);

    int safeendcol = (endcol - startcol > cols - sl - 2 ? cols - sl - 2 : endcol);

    cat(path);
    printOutputToScr(sl, startrow, endrow, startcol, safeendcol);
    _clearOutput();

    footer(state, desc, rows, cols);
}

void showfile(char *path, enum STATE state)
{
    char filename[512];
    int n = strlen(path);
    int ii;
    for (ii = n - 1; ii > -1; ii--)
    {
        if (path[ii] == '/')
            break;
    }

    for (int i = ii + 1; i < n; i++)
    {
        filename[i - ii - 1] = path[i];
    }

    filename[n - ii - 1] = '\0';

    int chars, words, lines;
    __originFileLen(path, &chars, &lines, &words);
    chdir(parentDir);

    char desc[512];
    strcpy(desc, __itoa(chars));
    strcat(desc, " characters, ");
    strcat(desc, __itoa(words));
    strcat(desc, " words ");
    strcat(desc, __itoa(lines));
    strcat(desc, " lines");

    initscr(path, filename, 1, lines + 1, 1, lines + 2, 0, 1 << 12, -1, -1, state, desc);
}

int main()
{
    init();
    showfile("/root/bruh/wtf/this/is/a/test/myfile1.txt", NORMAL);

    getch();
    system("cls");
    finish();
}
