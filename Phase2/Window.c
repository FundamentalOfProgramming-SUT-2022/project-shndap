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
const WORD CRSR_COLOR = COLOR(8, 0);
const WORD CLPR_COLOR = COLOR(13, 15);
const WORD STATECOLOR[] = {COLOR(3, 0), COLOR(2, 0), COLOR(14, 0)};

const char UPCOM = 'k';
const char DNCOM = 'j';
const char LTCOM = 'h';
const char RTCOM = 'i';

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

struct SCREEN
{
    char *filename;
    int *linesize;
    int saved;
    int maxrow;
    int maxcol;
    int startrow;
    int endrow;
    int startcol;
    int endcol;
    int activestart;
    int activeend;
    int sidelen;
    enum STATE state;
    char *desc;
};

/// @brief initializes a struct screen
struct SCREEN *newScreen(char *_filename, int* _linesize, int _saved,
                         int _maxrow, int _maxcol,
                         int _startrow, int _endrow,
                         int _startcol, int _endcol,
                         int _activestart, int _activeend, int _sidelen,
                         enum STATE _state, char *_desc)
{
    struct SCREEN *new = (struct SCREEN *)malloc(sizeof(struct SCREEN));

    new->activeend = _activeend;
    new->activestart = _activestart;
    new->desc = _desc;
    new->endcol = _endcol;
    new->endrow = _endrow;
    new->filename = _filename;
    new->linesize = _linesize;
    new->maxcol = _maxcol;
    new->maxrow = _maxrow;
    new->saved = _saved;
    new->sidelen = _sidelen;
    new->startcol = _startcol;
    new->startrow = _startrow;
    new->state = _state;

    return new;
}

struct SCRCUR
{
    struct SCREEN *scr;
    COORD *cursor;
};

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

/// @brief Changes colors to cursor mode
void tocrsr()
{
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), CRSR_COLOR);
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

    for (int ii = 1; ii < scrrow - 2; ii++)
    {
        int i = startrow + ii - 1;
        pos.Y = ii;
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

/// @brief Get size of lines
/// @param linesize array to be filled (1-based)
void getlinesize(int* linesize)
{
    char cwd[512];
    getcwd(cwd, 512);

    chdir(parentDir);

    OUTPUT = fopen(outputPath, "r");

    char c = fgetc(OUTPUT);

    int curlen = 1;
    int curline = 1;

    while(c != EOF)
    {
        if(c != '\n')
            goto current;

        linesize[curline++] = curlen; /*problematic*/
        curlen = 0;
        
        current:
            c = fgetc(OUTPUT);
            curlen++;
    }

    linesize[curline++] = curlen;

    fclose(OUTPUT);
    chdir(cwd);
}

/// @brief Prints output to screen
/// @param sidelen length of sidebar
/// @param start starting row
/// @param end last row
/// @param startcol starting column
/// @param endcol last column
/// @param scrrow size of screen
/// @param cursor
void printOutputToScr(int sidelen, int start, int end, int startcol, int endcol, int scrrow, COORD *cursor)
{
    char cwd[512];
    getcwd(cwd, 512);

    chdir(parentDir);

    OUTPUT = fopen(outputPath, "r");

    char c = fgetc(OUTPUT);

    int X = sidelen + 1, Y = 1;

    setCursorPos(X, 1);
    X = 0;

    // printf("%d, %d, %d\n", start, end, cursor->Y);
    // getch();

    while (c != EOF)
    {
        if (Y >= end)
            break;

            
        if (X == cursor->X - sidelen - 1 && Y == cursor->Y)
            tocrsr();

        if (c == '\n' || c == '\0' || c == EOF)
        {
            if (Y == cursor->Y && cursor->X - sidelen - 1 - startcol >= X){
                tocrsr();
                cursor->X = sidelen + 1 + X + startcol;
                printf(" ");
            }

            totext();
            Y++;
            X = sidelen + 1;
            if(Y - start + 1)
                setCursorPos(X, Y - start + 1);
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

        if (Y >= start && Y < end && X >= startcol && X < endcol)
        {
            printf("%c", c);
        }

        X++;
        c = fgetc(OUTPUT);
        totext();
    }

    if (Y == cursor->Y && cursor->X - sidelen - 1 - startcol >= X){
        tocrsr();
        cursor->X = sidelen + 1 + X;
    }
    printf(" ");
    totext();

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
/// @param scr current screen pointer
/// @param cursor
void initscr(char *path, struct SCREEN *scr, COORD *cursor)
{
    system("cls");
    int rows, cols;
    scrsize(&cols, &rows);

    header(scr->filename, scr->saved);
    int safeendrow = (scr->endrow - scr->startrow > rows - 2 ? rows - 2 : scr->endrow);
    scr->endrow = safeendrow;
    
    int sl = sidebar(scr->maxrow, scr->startrow, scr->endrow, rows, scr->activestart, scr->activeend);

    int safeendcol = (scr->endcol - scr->startcol > cols - sl - 2 ? cols - sl - 2 : scr->endcol);
    scr->endcol = safeendcol;

    cat(path);
    printOutputToScr(sl, scr->startrow, safeendrow, scr->startcol, safeendcol, rows, cursor);
    _clearOutput();

    footer(scr->state, scr->desc, rows, cols);
}

/// @brief Shows a file
/// @param path file path
/// @param state
/// @return current screen pointer and cursor
struct SCRCUR *showfile(char *path, enum STATE state)
{
    char *filename = calloc(512, sizeof(char));
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

    int chars, words, lines, maxcol;
    __originFileLen(path, &chars, &lines, &words, &maxcol);
    chdir(parentDir);

    char *desc = calloc(512, sizeof(char));
    strcpy(desc, __itoa(chars));
    strcat(desc, " characters, ");
    strcat(desc, __itoa(words));
    strcat(desc, " words ");
    strcat(desc, __itoa(lines));
    strcat(desc, " lines");

    int *linesize = calloc(lines + 2, sizeof(int));
    cat(path);
    getlinesize(linesize);
    _clearOutput();

    int maxlen = (int)ceil(log10(lines + 1));

    struct SCRCUR *scrcur = (struct SCRCUR *)malloc(sizeof(struct SCRCUR));
    scrcur->cursor = (COORD *)malloc(sizeof(COORD));
    scrcur->scr = (struct SCREEN *)malloc(sizeof(struct SCREEN));

    scrcur->scr = newScreen(filename, linesize, 1, lines + 1, maxcol, 1, lines + 1, 0, maxcol, 0, 0, maxlen + 1, state, desc);
    scrcur->cursor->X = scrcur->scr->sidelen + 1;
    scrcur->cursor->Y = 1;

    initscr(path, scrcur->scr, scrcur->cursor);

    return scrcur;
}

/// @brief Handles scr according to com
/// @param scr current screen
/// @param cursor
/// @param com command
void navigateScr(struct SCREEN *scr, COORD *cursor, char com)
{
    int row, col;
    scrsize(&col, &row);

    if (com == UPCOM)
    {
        if (cursor->Y == 1)
        {
            return;
        }
        else if (cursor->Y - scr->startrow <= 4)
        {
            if (scr->startrow > 1)
            {
                scr->startrow--;
                scr->endrow--;
                cursor->Y--;
            }
            else
            {
                cursor->Y--;
            }
        }
        else
        {
            cursor->Y--;
        }
    }
    else if (com == DNCOM)
    {
        if (cursor->Y + 1 == scr->maxrow)
        {
            return;
        }
        else if (cursor->Y - scr->startrow >= row - 6)
        {
            if (scr->endrow < scr->maxrow)
            {
                scr->startrow++;
                scr->endrow++;
                cursor->Y++;
            }
            else
            {
                cursor->Y++;
            }
        }
        else
        {
            cursor->Y++;
        }
    }
    else if (com == LTCOM)
    {
        if (cursor->X == scr->startcol + scr->sidelen + 1)
        {
            return;
        }
        else if (cursor->X <= scr->sidelen + scr->startcol + 5)
        {
            if (scr->startcol > 0)
            {
                scr->startcol--;
                scr->endcol--;
                cursor->X--;
            }
            else
            {
                cursor->X--;
            }
        }
        else
        {
            cursor->X--;
        }
    }
    else if (com == RTCOM)
    {
        if (cursor->X == scr->maxcol + scr->sidelen)
        {
            return;
        }
        else if (cursor->X - scr->startcol - 1 >= col - 4 - scr->sidelen)
        {
            if (scr->endcol < scr->maxcol)
            {
                scr->startcol++;
                scr->endcol++;
                cursor->X++;
            }
            else
            {
                cursor->X++;
            }
        }
        else
        {
            cursor->X++;
        }
    }

    int lnsz = scr->linesize[cursor->Y];
    int diff = cursor->X - lnsz - scr->sidelen - 1;
    if(diff > 0)
    {
        cursor->X -= diff;
        if(scr->startcol - diff < 0)
        {
            scr->startcol = 0;
            scr->endcol = scr->maxcol;
        }else
        {
            scr->startcol -= diff;
            scr->startcol -= diff;
        }
    }
}

void runFile(char* path, enum STATE state)
{
    struct SCRCUR *scrcur = showfile(path, state);

    char c = getch();
    while(c != 'x')
    {
        navigateScr(scrcur->scr, scrcur->cursor, c);
        initscr(path, scrcur->scr, scrcur->cursor);
        c = getch();
    }
}

int main()
{
    init();

    runFile("/root/bruh/wtf/this/is/a/test/myfile1.txt", VISUAL);

    // Handler("find --str \"4* 1\" --file /root/bruh/wtf/this/is/a/test/myfile1.txt -all -byword");
    // struct SCRCUR *scrcur = showfile("/root/bruh/wtf/this/is/a/test/myfile1.txt", VISUAL);
    // navigateScr(scrcur->scr, scrcur->cursor, RTCOM);
    // system("cls");
    // navigateScr(scrcur->scr, scrcur->cursor, LTCOM);
    // system("cls");
    // printf("%d", scrcur->cursor->X);
    // initscr("/root/bruh/wtf/this/is/a/test/myfile1.txt", scrcur->scr, scrcur->cursor);
    // getch();

    // system("cls");
    finish();
}
