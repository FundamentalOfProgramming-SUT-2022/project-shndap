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
#define SIGN(x) (x > 0 ? +1 : -1)
#define ISNUM(x) (x >= '0' && x <= '9')
#define esc_key 27

enum COLORVAL
{
    BLACK,
    BLUE,
    GREEN,
    CYAN,
    RED,
    MAGENTA,
    BROWN,
    LIGHTGRAY,
    DARKGRAY,
    LIGHTBLUE,
    LIGHTGREEN,
    LIGHTCYAN,
    LIGHTRED,
    LIGHTMAGENTA,
    YELLOW,
    WHITE
};

enum COLORINGSTATE
{
    DEFAULT,
    ONCURSOR,
    SELECTED,
    HIGHLIGHTED
};

/* Theme Colors */
const WORD TEXT_COLOR = COLOR(BLACK, WHITE);
const WORD HEAD_COLOR = COLOR(LIGHTGRAY, BLACK);
const WORD SIDE_COLOR = COLOR(BLACK, DARKGRAY);
const WORD ACTV_COLOR = COLOR(BLACK, WHITE);
const WORD DESC_COLOR = COLOR(DARKGRAY, BLACK);
const WORD CRSR_COLOR = COLOR(DARKGRAY, BLACK);
const WORD CLPR_COLOR = COLOR(LIGHTMAGENTA, WHITE);
const WORD HGLT_COLOR = COLOR(YELLOW, RED);
const WORD SLCT_COLOR = COLOR(LIGHTBLUE, WHITE);
const WORD NARO_COLOR = COLOR(BLACK, MAGENTA);
const WORD EROR_COLOR = COLOR(BLACK, RED);
const WORD DNGE_COLOR[] = {COLOR(BLACK, RED), COLOR(DARKGRAY, RED), COLOR(LIGHTBLUE, RED), COLOR(YELLOW, RED)};
const WORD CORR_COLOR[] = {COLOR(BLACK, YELLOW), COLOR(DARKGRAY, YELLOW), COLOR(LIGHTBLUE, YELLOW), COLOR(YELLOW, BROWN)};
const WORD QUOT_COLOR[] = {COLOR(BLACK, BROWN), COLOR(DARKGRAY, BROWN), COLOR(LIGHTBLUE, BROWN), COLOR(YELLOW, BROWN)};
const WORD STATECOLOR[] = {COLOR(CYAN, BLACK), COLOR(GREEN, BLACK), COLOR(YELLOW, BLACK)};

/* Commands */
const char UPCOM = 'k';
const char DNCOM = 'j';
const char LTCOM = 'h';
const char RTCOM = 'l';
const char INDCOM = '=';
const char PH1COM = ':';
const char FNDCOM = '/';
const char NXTCOM = 'n';
const char UDOCOM = 'u';
const char CPYCOM = 'y';
const char CUTCOM = 't';
const char DELCOM = 'd';
const char PSTCOM = 'p';
const char CMDCOM = '!';
const char TO_NORMAL = esc_key;
const char TO_INSERT[] = "INSERT";
const char TO_VISUAL[] = "VISUAL";
const char SAVE_COM[] = "save";
const char SVAS_COM[] = "saveas";
const char OPEN_COM[] = "open";
const char UNDO_COM[] = "undo";
const char RPLC_COM[] = "replace";

enum STATE
{
    NORMAL, /* CLI */
    INSERT, /* Typing text */
    VISUAL  /* Edit text in bulk */
};

const char *STATESTR[] =
    {
        " NORMAL ",
        " INSERT ",
        " VISUAL "};

struct SCREEN
{
    char *initfilepath;
    char *filepath;
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
struct SCREEN *newScreen(char *_initfilepath, char *_filepath, char *_filename, int *_linesize, int _saved,
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
    new->filepath = _filepath;
    new->initfilepath = _initfilepath;
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
    COORD *initcursor;
    int count;
};

/// @brief Moves cursor to x, y
/// @param X
/// @param Y
void setCursorPos(int X, int Y)
{
    COORD pos = {X, Y};
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), pos);
}

/// @brief Compare this with that
/// @param this
/// @param that
/// @return 1 if this is bigger, -1 if that is bigger, 0 otherwise
int cmpCursor(COORD *this, COORD *that)
{
    if (this->Y < that->Y)
        return -1;
    if (this->Y > that->Y)
        return 1;
    if (this->X < that->X)
        return -1;
    if (this->X > that->X)
        return 1;
    return 0;
}

/// @brief Compare cursor with XY
/// @param this cursor
/// @param X
/// @param Y
/// @return 1 if this is bigger, -1 if this is smaller, 0 otherwise
int cmpCursorXY(COORD *this, int X, int Y)
{
    if (this->Y < Y)
        return -1;
    if (this->Y > Y)
        return 1;
    if (this->X < X)
        return -1;
    if (this->X > X)
        return 1;
    return 0;
}

/// @brief Swaps two cursors
/// @param this
/// @param that
void swap(COORD *this, COORD *that)
{
    COORD temp = {.X = this->X, .Y = this->Y};
    this->X = that->X;
    this->Y = that->Y;
    that->X = temp.X;
    that->Y = temp.Y;
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

/// @brief background color
/// @param col
int bgcol(WORD col)
{
    return col / 16;
}

/// @brief foreground color
/// @param col
int fgcol(WORD col)
{
    return col % 16;
}

/// @brief current color of output
/// @return current color of output
WORD curcol()
{
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    return csbi.wAttributes;
}

/// @brief Get color for output
/// @param openparn
/// @param openbrkt
/// @param opencurl
/// @param quote
/// @param c current character
/// @return color
WORD getcol(int openparn, int openbrkt, int opencurl, int quote, char c)
{
    enum COLORINGSTATE colstate;
    int current = curcol();
    int curbg = bgcol(current);

    if (curbg == bgcol(HGLT_COLOR))
        colstate = HIGHLIGHTED;
    else if (curbg == bgcol(SLCT_COLOR))
        colstate = SELECTED;
    else if (curbg == bgcol(CRSR_COLOR))
        colstate = ONCURSOR;
    else
        colstate = DEFAULT;

    if (quote || c == '"')
    {
        return QUOT_COLOR[colstate];
    }

    if (c == '{' || c == '(' || c == '[')
    {
        return CORR_COLOR[colstate];
    }

    if (c == ')')
    {
        if (openparn < 0)
            return DNGE_COLOR[colstate];
        else
            return CORR_COLOR[colstate];
    }

    if (c == '}')
    {
        if (opencurl < 0)
            return DNGE_COLOR[colstate];
        else
            return CORR_COLOR[colstate];
    }

    if (c == ']')
    {
        if (openbrkt < 0)
            return DNGE_COLOR[colstate];
        else
            return CORR_COLOR[colstate];
    }

    return current;
}

/// @brief Changes color to col
/// @param col
void tocol(WORD col)
{
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), col);
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

/// @brief Changes colors to select mode
void toslct()
{
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), SLCT_COLOR);
}

/// @brief Changes colors to highlight mode
void tohglt()
{
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), HGLT_COLOR);
}

/// @brief Changes colors to not-a-row mode
void tonaro()
{
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), NARO_COLOR);
}

/// @brief Changes colors to error mode
void toeror()
{
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), EROR_COLOR);
}

/// @brief Changes colors to correct parantheses mode
void tocorr(enum COLORINGSTATE colstate)
{
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), CORR_COLOR[colstate]);
}

/// @brief Changes colors to dangeling parantheses mode
void todnge(enum COLORINGSTATE colstate)
{
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), DNGE_COLOR[colstate]);
}

/// @brief Changes colors to quote mode
void toquot(enum COLORINGSTATE colstate)
{
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), QUOT_COLOR[colstate]);
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
            tonaro();
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
void getlinesize(int *linesize)
{
    char cwd[512];
    getcwd(cwd, 512);

    chdir(parentDir);

    OUTPUT = fopen(outputPath, "r");

    char c = fgetc(OUTPUT);

    int curlen = 1;
    int curline = 1;

    while (c != EOF)
    {
        if (c != '\n')
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
/// @param state
/// @param sidelen length of sidebar
/// @param start starting row
/// @param end last row
/// @param startcol starting column
/// @param endcol last column
/// @param scrrow size of screen
/// @param cursor
void printOutputToScr(enum STATE state, int sidelen, int start, int end, int startcol, int endcol, int scrrow, COORD *cursor, COORD *initcursor)
{
    char cwd[512];
    getcwd(cwd, 512);

    chdir(parentDir);

    OUTPUT = fopen(outputPath, "r");

    char c = fgetc(OUTPUT);

    int X = sidelen + 1, Y = 1;

    setCursorPos(X, 1);
    X = 0;

    COORD firstcrsr = {.X = initcursor->X, .Y = initcursor->Y};
    COORD secondcrsr = {.X = cursor->X, .Y = cursor->Y};

    if (cmpCursor(&firstcrsr, &secondcrsr) == 1)
    {
        swap(&firstcrsr, &secondcrsr);
    }

    int slct = 0;
    int quote = 0;
    int openparn = 0;
    int openbrkt = 0;
    int opencurl = 0;

    while (c != EOF)
    {
        if (c == '[' && !quote)
            openbrkt = openbrkt < 0 ? openbrkt = 1 : openbrkt + 1;
        if (c == ']' && !quote)
            openbrkt--;
        if (c == '{' && !quote)
            opencurl = opencurl < 0 ? opencurl = 1 : opencurl + 1;
        if (c == '}' && !quote)
            opencurl--;
        if (c == '(' && !quote)
            openparn = openparn < 0 ? openparn = 1 : openparn + 1;
        if (c == ')' && !quote)
            openparn--;
        if (c == '"')
            quote = !quote;

        if (state == VISUAL)
        {
            // printf("%d %d | %d %d\n", firstcrsr.X, firstcrsr.Y, X + sidelen + 1, Y);
            // getch();
            if (cmpCursorXY(&firstcrsr, X + sidelen + 1, Y) == 1)
            {
                totext();
                slct = 0;
            }
            if (cmpCursorXY(&secondcrsr, X + sidelen + 1, Y) != -1)
            {
                toslct();
                slct = 1;
            }
            if (cmpCursorXY(&secondcrsr, X + sidelen + 1, Y) == -1)
            {
                totext();
                slct = 0;
            }
        }

        if (Y >= end)
            break;

        if (X == cursor->X - sidelen - 1 && Y == cursor->Y)
        {
            if (!slct)
                tocrsr();
            else
                totext();
        }

        if (c == '\n' || c == '\0' || c == EOF)
        {
            if (Y == cursor->Y && cursor->X - sidelen - 1 - startcol >= X)
            {
                if (!slct)
                    tocrsr();
                else
                    totext();

                cursor->X = sidelen + 1 + X + startcol;
                printf(" ");
            }

            totext();
            Y++;
            X = sidelen + 1;
            if (Y - start + 1)
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
                {
                    WORD prevcol = curcol();
                    tocol(getcol(openparn, openbrkt, opencurl, quote, c));
                    printf("~");
                    tocol(prevcol);
                }
                X++;
            }
        }

        if (Y >= start && Y < end && X >= startcol && X < endcol)
        {
            WORD prevcol = curcol();
            tocol(getcol(openparn, openbrkt, opencurl, quote, c));
            printf("%c", c);
            tocol(prevcol);
        }

        X++;
        c = fgetc(OUTPUT);
        totext();
    }

    if (Y == cursor->Y && cursor->X - sidelen - 1 - startcol >= X)
    {
        if (!slct)
            tocrsr();
        else
            totext();
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
/// @param scrcur current screen and cursor
/// @param cursor
void initscr(struct SCRCUR *scrcur)
{
    system("cls");
    int rows, cols;
    scrsize(&cols, &rows);

    header(scrcur->scr->filename, scrcur->scr->saved);
    int safeendrow = (scrcur->scr->endrow - scrcur->scr->startrow > rows - 2 ? rows - 2 : scrcur->scr->endrow);
    scrcur->scr->endrow = safeendrow;

    int sl = sidebar(scrcur->scr->maxrow, scrcur->scr->startrow, scrcur->scr->endrow, rows, scrcur->scr->activestart, scrcur->scr->activeend);

    int safeendcol = (scrcur->scr->endcol - scrcur->scr->startcol > cols - sl - 2 ? cols - sl - 2 : scrcur->scr->endcol);
    scrcur->scr->endcol = safeendcol;

    cat(scrcur->scr->filepath);
    printOutputToScr(scrcur->scr->state, sl, scrcur->scr->startrow, safeendrow, scrcur->scr->startcol, safeendcol, rows, scrcur->cursor, scrcur->initcursor);
    _clearOutput();

    footer(scrcur->scr->state, scrcur->scr->desc, rows, cols);
}

/// @brief Empties a file
/// @param path
void emptyFile(char *path)
{
    int chars, lines, words, maxlen;
    __originFileLen(path, &chars, &lines, &words, &maxlen);
    chdir(parentDir);
    removeStr(path, 1, 0, chars, 1);
}

/// @brief Open a file
/// @param __file_path
/// @return file
FILE *openPath(char *__file_path)
{
    char *path = malloc(512 * sizeof(char));
    strcpy(path, __file_path);

    int i, foundDot = 0;
    for (i = strlen(path) - 1; path[i] != '/' && i > -1; i--)
    {
        foundDot |= (path[i] == '.');
    }

    if (i == -1 || !foundDot)
    {
        _writeToOutput("~!ERROR: Invalid file name\n");
        free(path);
        chdir(parentDir);
        return NULL;
    }

    char *filename = malloc((strlen(path) - 1 - i) * sizeof(char));

    int _dum_ = strlen(path);
    for (int j = i + 1; j < _dum_; j++)
    {
        filename[j - i - 1] = path[j];
    }

    filename[strlen(path) - 1 - i] = '\0';
    path[i] = '\0';

    char *tok;
    int isFirst = 1;

    while ((tok = strtok(path, "/")) != NULL)
    {
        if (isFirst && strcmp(tok, "root"))
        {
            _writeToOutput("~!ERROR: File should be in root folder\n");
            chdir(parentDir);
            return NULL;
        }

        isFirst = 0;

        if (chdir(tok))
        {
            _writeToOutput("~!ERROR: Invalid directory\n");
            chdir(parentDir);
            return NULL;
        }

        path = NULL;
    }

    if (!__fileExists(filename))
    {
        _writeToOutput("~!ERROR: File ~?");
        _writeToOutput(__file_path);
        _writeToOutput(" ~&does not exist\n");
        free(path);
        chdir(parentDir);
        return NULL;
    }

    FILE *fp = fopen(filename, "r+");

    free(path);
    chdir(parentDir);
    return fp;
}

/// @brief Pastes a file into another using path
/// @param thispath
/// @param thatpath
void pasteFileInto(char *thispath, char *thatpath)
{
    FILE *this = openPath(thispath);
    FILE *that = openPath(thatpath);

    __copy(this, that);

    fclose(this);
    fclose(that);
}

/// @brief Shows a file
/// @param path file path
/// @param state
/// @return current screen pointer and cursor
struct SCRCUR *showfile(char *path, enum STATE state)
{
    char *filename = calloc(512, sizeof(char));
    char *untcopy = calloc(512, sizeof(char));
    strcpy(untcopy, untitledPathCat);

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
    scrcur->initcursor = (COORD *)malloc(sizeof(COORD));

    emptyFile(untcopy);
    pasteFileInto(path, untcopy);

    scrcur->scr = newScreen(path, untcopy, filename, linesize, 0, lines + 1, maxcol, 1, lines + 2, 0, maxcol, 1, 2, maxlen + 1, state, desc);
    scrcur->cursor->X = scrcur->scr->sidelen + 1;
    scrcur->cursor->Y = 1;
    scrcur->initcursor->X = scrcur->cursor->X;
    scrcur->initcursor->Y = scrcur->cursor->Y;

    scrcur->count = 0;

    initscr(scrcur);

    return scrcur;
}

/// @brief Handles Changing mode commands in CLI
/// @param str
/// @param row screen size in rows
void handleChmodCommands(struct SCRCUR *scrcur, char *str, int row)
{
    if (!strcmp(str, TO_VISUAL))
    {
        scrcur->scr->state = VISUAL;
    }
    else if (!strcmp(str, TO_INSERT))
    {
        scrcur->scr->state = INSERT;
    }
    else
    {
        initscr(scrcur);
        _writeToOutput("~!ERROR: Invalid Mode");
        setCursorPos(strlen(" Command Line: "), row);
        _showOutput();

        sleep(1);

        _clearOutput();
        return;
    }
}

/// @brief Extracts args from inp
/// @param inp
/// @return Number of arguments
int buildArgs(char *inp)
{
    int argc = 0;
    int lenc = strlen(inp);
    char currarg[32768];
    currarg[0] = '\0';
    int inQuote = 0;
    int currarglen = 0;

    for (int i = 0; i < lenc; i++)
    {
        if (inp[i] == ' ')
        {
            if (inQuote)
            {
                strncat(currarg, inp + i, 1);
                currarglen++;
            }
            else
            {
                currarg[currarglen] = '\0';
                _addArg(currarg);
                currarglen = 0;
                currarg[0] = '\0';
                argc++;
            }
        }
        else if (inp[i] == '"')
        {
            int isrealquote = (i != 0 && inp[i - 1] != '\\');

            if (isrealquote)
            {
                inQuote = !inQuote;
            }
            else
            {
                strncat(currarg, inp + i, 1);
                currarglen++;
            }
        }
        else if (inp[i] == '\\')
        {
            if (i != lenc - 1)
            {
                if (inp[i + 1] == '\\')
                {
                    strncat(currarg, inp + i, 1);
                    currarglen++;
                    i++;
                    continue;
                }
                else
                {
                    if (inp[i + 1] == 'n')
                    {
                        strcat(currarg, "\n");
                        i++;
                        currarglen++;
                        continue;
                    }
                    else if (inp[i + 1] == 't')
                    {
                        strcat(currarg, "\t");
                        i++;
                        currarglen++;
                        continue;
                    }
                    else if (inp[i + 1] == 'v')
                    {
                        strcat(currarg, "\v");
                        i++;
                        currarglen++;
                        continue;
                    }
                    else if (inp[i + 1] == '"')
                    {
                        strcat(currarg, "\"");
                        i++;
                        currarglen++;
                        continue;
                    }
                    else
                    {
                        strncat(currarg, inp + i, 1);
                        currarglen++;
                        continue;
                    }
                }
            }
            else
            {
                strncat(currarg, inp + i, 1);
                currarglen++;
            }
        }
        else
        {
            strncat(currarg, inp + i, 1);
            currarglen++;
        }

        if (currarglen >= 32768)
        {
            _writeToOutput("~!ERROR: Argument size exceeded (32768 characters)\n");
            return 0;
        }
    }

    if (currarglen)
    {
        currarg[currarglen] = '\0';
        _addArg(currarg);
        argc++;
        currarg[0] = '\0';
        currarglen = 0;
    }

    return argc;
}

/// @brief Prints Error
/// @param scrcur
/// @param text
void printErrorInCLI(struct SCRCUR *scrcur, char *text)
{
    int col, row;
    scrsize(&col, &row);

    initscr(scrcur);

    setCursorPos(strlen(" Command Line: "), row);

    toeror();
    printf("ERROR: %s", text);
    totext();
    sleep(1);
}

/// @brief Prints Message with 1 second waiting
/// @param scrcur
/// @param text
void printMessageInCLI(struct SCRCUR *scrcur, char *text)
{
    int col, row;
    scrsize(&col, &row);

    initscr(scrcur);

    setCursorPos(strlen(" Command Line: "), row);

    totext();
    printf("%s", text);
    sleep(1);
}

/// @brief Prints message with no wait
/// @param scrcur
/// @param text
void printMessageInCLIWOwaiting(struct SCRCUR *scrcur, char *text)
{
    int col, row;
    scrsize(&col, &row);

    initscr(scrcur);

    setCursorPos(strlen(" Command Line: "), row);

    totext();
    printf("%s", text);
}

/// @brief if a file is ready to be pasted into
/// @param __file_path file name
/// @param scrcur
/// @return 0: error, 1: doesn't exist, 2: exists
int readytoCreateFile(struct SCRCUR *scrcur, char *__file_path)
{
    char *path = malloc(512 * sizeof(char));
    strcpy(path, __file_path);

    int i, foundDot = 0;
    for (i = strlen(path) - 1; path[i] != '/' && i > -1; i--)
    {
        foundDot |= (path[i] == '.');
    }

    if (i == -1 || !foundDot)
    {
        printErrorInCLI(scrcur, "Invalid file name");
        free(path);
        chdir(parentDir);
        return 0;
    }

    char *filename = malloc((strlen(path) - 1 - i) * sizeof(char));

    int _dum_ = strlen(path);
    for (int j = i + 1; j < _dum_; j++)
    {
        filename[j - i - 1] = path[j];
    }

    filename[strlen(path) - 1 - i] = '\0';
    path[i] = '\0';

    char *tok;
    int isFirst = 1;

    while ((tok = strtok(path, "/")) != NULL)
    {
        if (isFirst && strcmp(tok, "root"))
        {
            printErrorInCLI(scrcur, "File should be created in root folder");
            chdir(parentDir);
            return 0;
        }

        isFirst = 0;

        _mkdir(tok);
        chdir(tok);

        path = NULL;
    }

    if (__fileExists(filename))
    {
        free(path);
        chdir(parentDir);
        return 2;
    }
    else
    {
        free(path);
        chdir(parentDir);
        return 1;
    }
}

/// @brief Creates a file
/// @param __file_path file name
/// @param scrcur
/// @return 1 if there waas no problem
int newCreateFile(struct SCRCUR *scrcur, char *__file_path)
{
    char *path = malloc(512 * sizeof(char));
    strcpy(path, __file_path);

    int i, foundDot = 0;
    for (i = strlen(path) - 1; path[i] != '/' && i > -1; i--)
    {
        foundDot |= (path[i] == '.');
    }

    if (i == -1 || !foundDot)
    {
        printErrorInCLI(scrcur, "Invalid file name");
        free(path);
        chdir(parentDir);
        return 0;
    }

    char *filename = malloc((strlen(path) - 1 - i) * sizeof(char));

    int _dum_ = strlen(path);
    for (int j = i + 1; j < _dum_; j++)
    {
        filename[j - i - 1] = path[j];
    }

    filename[strlen(path) - 1 - i] = '\0';
    path[i] = '\0';

    char *tok;
    int isFirst = 1;

    while ((tok = strtok(path, "/")) != NULL)
    {
        if (isFirst && strcmp(tok, "root"))
        {
            printErrorInCLI(scrcur, "File should be created in root folder");
            chdir(parentDir);
            return 0;
        }

        isFirst = 0;

        _mkdir(tok);
        chdir(tok);

        path = NULL;
    }

    if (__fileExists(filename))
    {
        printErrorInCLI(scrcur, "File already exists");
        free(path);
        chdir(parentDir);
        return 0;
    }

    FILE *fp = fopen(filename, "w");

    if (fp)
    {
        fclose(fp);
        chdir(parentDir);
        return 1;
    }
    else
    {
        printErrorInCLI(scrcur, "Could not create file\n");
        chdir(parentDir);
        return 0;
    }
}

/// @brief Saves a file
/// @param scrcur
void savefile(struct SCRCUR *scrcur)
{
    if (!strcmp(scrcur->scr->initfilepath, untitledPathCat))
    {
        printMessageInCLIWOwaiting(scrcur, "Provide a name/path for your file: ");
        char *str = calloc(1024, sizeof(char));
        gets(str);

        int argc = buildArgs(str);

        char *arg[argc];
        for (int i = 0; i < argc; i++)
        {
            arg[i] = _getarg(i);
        }

        if (argc != 1)
        {
            printErrorInCLI(scrcur, "Invalid input");
            return;
        }

        int rtcf = readytoCreateFile(scrcur, arg[0]);

        if (rtcf == 2)
        {
            printMessageInCLIWOwaiting(scrcur, "File already exists, do you wish to replace it? (y/n) ");
            char c;
            scanf("%c", &c);
            if (c == 'y')
            {
                emptyFile(arg[0]);
                pasteFileInto(scrcur->scr->initfilepath, arg[0]);
                *scrcur = *showfile(arg[0], NORMAL);
                scrcur->scr->saved = 1;
                printMessageInCLI(scrcur, "File saved succesfully");
            }
            else
            {
                return;
            }
        }
        else if (rtcf == 1)
        {
            int done = newCreateFile(scrcur, arg[0]);
            _clearOutput();

            if (done)
            {
                pasteFileInto(scrcur->scr->initfilepath, arg[0]);
                *scrcur = *showfile(arg[0], NORMAL);
                scrcur->scr->saved = 1;
                printMessageInCLI(scrcur, "File saved succesfully");
                return;
            }
            else
            {
                return;
            }
        }
        else
        {
        }

        return;
    }
    else
    {
        if (!scrcur->scr->saved)
        {
            pasteFileInto(untitledPath, scrcur->scr->initfilepath);
            *scrcur = *showfile(scrcur->scr->initfilepath, NORMAL);
            scrcur->scr->saved = 1;
            printMessageInCLI(scrcur, "File saved successfully");
            return;
        }
    }
}

/// @brief Saves a file in path
/// @param scrcur
/// @param path
void saveasfile(struct SCRCUR *scrcur, char *path)
{
    int rtcf = readytoCreateFile(scrcur, path);

    if (rtcf == 2)
    {
        printMessageInCLIWOwaiting(scrcur, "File already exists, do you wish to replace it? (y/n) ");
        char c;
        scanf("%c", &c);
        if (c == 'y')
        {
            emptyFile(path);
            pasteFileInto(scrcur->scr->filepath, path);
            *scrcur = *showfile(path, NORMAL);
            scrcur->scr->saved = 1;
            printMessageInCLI(scrcur, "File saved succesfully");
        }
        else
        {
            return;
        }
    }
    else if (rtcf == 1)
    {
        int done = newCreateFile(scrcur, path);
        _clearOutput();

        if (done)
        {
            pasteFileInto(scrcur->scr->filepath, path);
            *scrcur = *showfile(path, NORMAL);
            scrcur->scr->saved = 1;
            printMessageInCLI(scrcur, "File saved succesfully");
            return;
        }
        else
        {
            return;
        }
    }
}

/// @brief Moves cursor to a certain pos
/// @param scrcur
/// @param torow row of file
/// @param tocol col of file
void moveCursorTo(struct SCRCUR *scrcur, int torow, int tocol)
{
    int scrrow, scrcol, avlcol, avlrow;
    scrsize(&scrcol, &scrrow);

    avlrow = scrrow - 3;
    avlcol = scrcol - scrcur->scr->sidelen - 1;

    int col_left = tocol - tocol % avlcol;
    int col_right = col_left + avlcol - 1;

    if (col_right >= scrcur->scr->linesize[torow])
    {
        col_left = max(0, scrcur->scr->linesize[torow] - avlcol + 1);
        col_right = col_left + avlcol - 1;
    }

    int row_up = torow - torow % avlrow;
    int row_down = row_up + avlrow + 1;

    if (row_down > scrcur->scr->maxrow + 1)
    {
        row_down = scrcur->scr->maxrow + 1;
        row_up = max(0, row_down - avlrow - 1);
    }

    scrcur->cursor->X = tocol + scrcur->scr->sidelen + 1;
    scrcur->cursor->Y = torow;
    scrcur->initcursor->X = scrcur->cursor->X;
    scrcur->initcursor->Y = scrcur->cursor->Y;
    scrcur->count = 0;

    scrcur->scr->activestart = scrcur->cursor->Y;
    scrcur->scr->activeend = scrcur->cursor->Y + 1;

    scrcur->scr->startcol = col_left;
    scrcur->scr->endcol = col_right;
    scrcur->scr->startrow = row_up + 1;
    scrcur->scr->endrow = row_down;

    initscr(scrcur);
}

/// @brief Reads first number of output
/// @return number
int getfirstoutputnum()
{
    OUTPUT = fopen(outputPath, "r");
    int out = 0;

    char c = fgetc(OUTPUT);

    for (int i = 0; ISNUM(c); i++)
    {
        out = out * 10 + (c - '0');
        c = fgetc(OUTPUT);
    }

    fclose(OUTPUT);
    return out;
}

/// @brief replace
/// @param scrcur
/// @param argc
/// @return index of first occurence (-1 if error occurred)
int replaceFile(struct SCRCUR *scrcur, int argc)
{
    char *arg[argc];
    for (int i = 0; i < argc; i++)
    {
        arg[i] = _getarg(i);
    }

    if (argc >= 5) /*normal*/
    {
        if (strcmp(arg[1], "--str1") || strcmp(arg[3], "--str2"))
        {
            printErrorInCLI(scrcur, "Invalid command");
            return -1;
        }

        if (argc == 5)
        {
            int findres = find(scrcur->scr->filepath, arg[2], 0, 0);

            if (!findres)
            {
                _clearOutput();
                return -1;
            }

            int ix = getfirstoutputnum();
            _clearOutput();

            _makeACopy(scrcur->scr->filepath);
            _pasteCopyInto(scrcur->scr->filepath, replace(scrcur->scr->filepath, arg[2], arg[4], 0));

            return ix;
        }
        else if (argc == 6)
        {
            if (!strcmp(arg[5], "-all"))
            {
                int findres = find(scrcur->scr->filepath, arg[2], 0, 0);
                if (!findres)
                {
                    _clearOutput();
                    return -1;
                }

                int ix = getfirstoutputnum();
                _clearOutput();

                _makeACopy(scrcur->scr->filepath);
                _pasteCopyInto(scrcur->scr->filepath, replace(scrcur->scr->filepath, arg[2], arg[4], -1));

                return ix;
            }
            else
            {
                printErrorInCLI(scrcur, "Invalid command");
                return -1;
            }
        }
        else if (argc == 7)
        {
            if (!strcmp(arg[5], "-at") && _isnum(arg[6]))
            {
                int findres = find(scrcur->scr->filepath, arg[2], AT, _tonum(arg[6]));
                if (!findres)
                {
                    _clearOutput();
                    return -1;
                }

                int ix = getfirstoutputnum();
                _clearOutput();

                _makeACopy(scrcur->scr->filepath);
                _pasteCopyInto(scrcur->scr->filepath, replace(scrcur->scr->filepath, arg[2], arg[4], _tonum(arg[6])));

                return ix;
            }
            else
            {
                printErrorInCLI(scrcur, "Invalid command");
                return -1;
            }
        }
        else
        {
            printErrorInCLI(scrcur, "Invalid command");
            return -1;
        }
    }
    else
    {
        printErrorInCLI(scrcur, "Invalid command");
        return -1;
    }
}

/// @brief Converts ix to row and col
/// @param path
/// @param ix
/// @param row (pass by pointer)
/// @param col (pass by pointer)
void getRowColbyindex(char *path, int ix, int *row, int *col)
{
    FILE *fp = openPath(path);

    if (!fp)
        return;

    char c = fgetc(fp);
    int cnt = 0;
    *row = 1;
    *col = 0;
    int newline = (c == '\n');

    while (c != EOF)
    {
        if (newline == 1)
        {
            (*row)++;
            *col = 0;
            newline = 0;
        }

        if (c == '\n')
        {
            newline = 1;
        }

        if (cnt == ix)
        {
            fclose(fp);
            return;
        }

        c = fgetc(fp);
        (*col)++;
        cnt++;
    }

    if (cnt == ix)
    {
        return;
    }

    *col = -1;
    *row = -1;

    fclose(fp);
}

/// @brief Handles ':' commands in CLI
/// @param str
/// @param row screen size in rows
void handleColonCommands(struct SCRCUR *scrcur, char *str, int row)
{
    int argc = buildArgs(str);

    char *arg[argc];
    for (int i = 0; i < argc; i++)
    {
        arg[i] = _getarg(i);
    }

    if (!strcmp(arg[0], SAVE_COM))
    {
        if (argc != 1)
        {
            printErrorInCLI(scrcur, "Invalid command");
            return;
        }

        savefile(scrcur);
    }
    else if (!strcmp(arg[0], SVAS_COM))
    {
        if (argc != 2)
        {
            printErrorInCLI(scrcur, "Invalid command");
            return;
        }

        saveasfile(scrcur, arg[1]);
    }
    else if (!strcmp(arg[0], OPEN_COM))
    {
        if (argc != 2)
        {
            printErrorInCLI(scrcur, "Invalid command");
            return;
        }

        int rtcf = readytoCreateFile(scrcur, arg[1]);

        if (rtcf == 1)
        {
            savefile(scrcur);
            createFile(arg[1]);
            *scrcur = *showfile(arg[1], NORMAL);
        }
        else if (rtcf == 2)
        {
            savefile(scrcur);
            *scrcur = *showfile(arg[1], NORMAL);
        }

        return;
    }
    else if (!strcmp(arg[0], UNDO_COM))
    {
        if (argc != 1)
        {
            printErrorInCLI(scrcur, "Invalid command");
            return;
        }

        undo(scrcur->scr->filepath);
        scrcur->scr->saved = 0;
    }
    else if (!strcmp(arg[0], RPLC_COM))
    {
        int ix = replaceFile(scrcur, argc);
        if (ix != -1)
        {
            int crow, ccol;
            getRowColbyindex(scrcur->scr->filepath, ix, &crow, &ccol);
            if (!(crow == -1 || ccol == -1))
            {
                moveCursorTo(scrcur, crow, ccol);
            }
        }
    }
    else
    {
        /* PHASE 1 */
    }
}

/// @brief Handles scr according to com
/// @param scrcur current screen and cursor
void navigateScr(struct SCRCUR *scrcur, char com)
{
    int row, col;
    scrsize(&col, &row);

    if (com == TO_NORMAL)
    {
        scrcur->scr->state = NORMAL;
        initscr(scrcur);
        setCursorPos(strlen(" Command Line: "), row);
        return;
    }

    if (scrcur->scr->state == NORMAL)
    {
        if (com == UDOCOM)
        {
            undo(scrcur->scr->filepath);
            scrcur->scr->saved = 0;
        }
        else if (com == PH1COM)
        {
            char *str = calloc(32768, sizeof(char));
            gets(str);
            handleColonCommands(scrcur, str, row);
            free(str);
        }
        else if (com == CMDCOM)
        {
            char *str = calloc(32768, sizeof(char));
            gets(str);
            handleChmodCommands(scrcur, str, row);
            free(str);
        }
        else if (com == FNDCOM)
        {
            char *str = calloc(32768, sizeof(char));
            gets(str);
            /* find */
            /* prolly handle nxtcom in here */
            free(str);
        }
        else if (com == NXTCOM)
        {
            /* next occurence */
        }
        else
        {
            /* inavlid */
        }

        return;
    }

    if (scrcur->scr->state == VISUAL)
    {
        COORD *firstcursor = malloc(sizeof(COORD));
        firstcursor->X = scrcur->cursor->X;
        firstcursor->Y = scrcur->cursor->Y;

        if (cmpCursor(scrcur->cursor, scrcur->initcursor) == 1)
        {
            firstcursor->X = scrcur->initcursor->X;
            firstcursor->Y = scrcur->initcursor->Y;
        }

        if (com == CPYCOM)
        {
            int changed = copyStr(scrcur->scr->filepath, firstcursor->Y, firstcursor->X - scrcur->scr->sidelen - 1, scrcur->count,
                                  SIGN(scrcur->count));
            scrcur->scr->saved = !changed;
            scrcur->scr->state = INSERT;
            return;
        }
        else if (com == CUTCOM)
        {
            int changed = cutStr(scrcur->scr->filepath, firstcursor->Y, firstcursor->X - scrcur->scr->sidelen - 1, scrcur->count,
                                 SIGN(scrcur->count));

            if (cmpCursor(scrcur->cursor, scrcur->initcursor) == 1)
            {
                scrcur->cursor->X = scrcur->initcursor->X;
                scrcur->cursor->Y = scrcur->initcursor->Y;
            }
            scrcur->count = 0;

            scrcur->scr->activestart = scrcur->cursor->Y;
            scrcur->scr->activeend = scrcur->cursor->Y + 1;

            scrcur->scr->state = INSERT;

            scrcur->scr->saved = !changed;

            return;
        }
        else if (com == DELCOM)
        {
            int changed = removeStr(scrcur->scr->filepath, firstcursor->Y, firstcursor->X - scrcur->scr->sidelen - 1, scrcur->count,
                                    SIGN(scrcur->count));

            if (cmpCursor(scrcur->cursor, scrcur->initcursor) == 1)
            {
                scrcur->cursor->X = scrcur->initcursor->X;
                scrcur->cursor->Y = scrcur->initcursor->Y;
            }
            scrcur->count = 0;

            scrcur->scr->activestart = scrcur->cursor->Y;
            scrcur->scr->activeend = scrcur->cursor->Y + 1;

            scrcur->scr->state = INSERT;

            scrcur->scr->saved = !changed;

            return;
        }
    }

    if (scrcur->scr->state == INSERT)
    {
        if (com == PSTCOM)
        {
            int changed = pasteStr(scrcur->scr->filepath, scrcur->cursor->Y, scrcur->cursor->X - scrcur->scr->sidelen - 1);
            scrcur->scr->saved = 0;
            scrcur->scr->saved = !changed;

            return;
        }
    }

    if (com == UPCOM)
    {
        if (scrcur->cursor->Y != 1)
            scrcur->count -= scrcur->scr->linesize[scrcur->cursor->Y - 1];

        if (scrcur->cursor->Y == 1)
        {
            return;
        }
        else if (scrcur->cursor->Y - scrcur->scr->startrow <= 4)
        {
            if (scrcur->scr->startrow > 1)
            {
                scrcur->scr->startrow--;
                scrcur->scr->endrow--;
                scrcur->cursor->Y--;
            }
            else
            {
                scrcur->cursor->Y--;
            }
        }
        else
        {
            scrcur->cursor->Y--;
        }
    }
    else if (com == DNCOM)
    {
        if (scrcur->cursor->Y + 1 != scrcur->scr->maxrow + 1)
            scrcur->count += scrcur->scr->linesize[scrcur->cursor->Y];

        if (scrcur->cursor->Y + 1 == scrcur->scr->maxrow + 1)
        {
            return;
        }
        else if (scrcur->cursor->Y - scrcur->scr->startrow >= row - 6)
        {
            if (scrcur->scr->endrow <= scrcur->scr->maxrow)
            {
                scrcur->scr->startrow++;
                scrcur->scr->endrow++;
                scrcur->cursor->Y++;
            }
            else
            {
                scrcur->cursor->Y++;
            }
        }
        else
        {
            scrcur->cursor->Y++;
        }
    }
    else if (com == LTCOM)
    {
        if (scrcur->cursor->X > scrcur->scr->sidelen + 1)
            scrcur->count--;

        if (scrcur->cursor->X == scrcur->scr->startcol + scrcur->scr->sidelen + 1)
        {
            return;
        }
        else if (scrcur->cursor->X <= scrcur->scr->sidelen + scrcur->scr->startcol + 5)
        {
            if (scrcur->scr->startcol > 0)
            {
                scrcur->scr->startcol--;
                scrcur->scr->endcol--;
                scrcur->cursor->X--;
            }
            else
            {
                scrcur->cursor->X--;
            }
        }
        else
        {
            scrcur->cursor->X--;
        }
    }
    else if (com == RTCOM)
    {
        if (scrcur->cursor->X < scrcur->scr->linesize[scrcur->cursor->Y] + scrcur->scr->sidelen)
            scrcur->count++;

        if (scrcur->cursor->X == scrcur->scr->maxcol + scrcur->scr->sidelen + 1)
        {
            /*may be incorrect*/
            return;
        }
        else if (scrcur->cursor->X - scrcur->scr->startcol - 1 >= col - 4 - scrcur->scr->sidelen)
        {
            if (scrcur->scr->endcol < scrcur->scr->maxcol)
            {
                scrcur->scr->startcol++;
                scrcur->scr->endcol++;
                scrcur->cursor->X++;
            }
            else
            {
                scrcur->cursor->X++;
            }
        }
        else
        {
            scrcur->cursor->X++;
        }
    }

    int lnsz = scrcur->scr->linesize[scrcur->cursor->Y];
    int diff = scrcur->cursor->X - lnsz - scrcur->scr->sidelen - 1;
    if (diff > 0)
    {
        scrcur->cursor->X -= diff;
        scrcur->count -= diff;

        if (scrcur->scr->startcol - diff < 0)
        {
            scrcur->scr->startcol = 0;
            scrcur->scr->endcol = scrcur->scr->maxcol;
        }
        else
        {
            scrcur->scr->startcol -= diff;
            scrcur->scr->startcol -= diff;
        }
    }

    if (scrcur->scr->state == VISUAL)
    {
        if (cmpCursor(scrcur->cursor, scrcur->initcursor) == -1)
        {
            scrcur->scr->activestart = scrcur->cursor->Y;
            scrcur->scr->activeend = scrcur->initcursor->Y + 1;
        }
        else
        {
            scrcur->scr->activestart = scrcur->initcursor->Y;
            scrcur->scr->activeend = scrcur->cursor->Y + 1;
        }
    }
    else if (scrcur->scr->state == INSERT)
    {
        scrcur->scr->activestart = scrcur->cursor->Y;
        scrcur->scr->activeend = scrcur->cursor->Y + 1;
    }
}

void runFile(char *path, enum STATE state)
{
    struct SCRCUR *scrcur = showfile(path, state);

    char c;

    if (scrcur->scr->state == NORMAL)
        c = getche();
    else
        c = getch();

    while (c != 'x')
    {
        navigateScr(scrcur, c);
        initscr(scrcur);
        if (scrcur->scr->state == NORMAL)
            c = getche();
        else
            c = getch();
    }
}

int main()
{
    /* VERYYYYYYY IMPORTANNNNNNNNNNTTTTTTTTTTTTTTTTTTTT */
    init();
    runFile("/root/bruh/wtf/this/is/a/test/myfile1.txt", NORMAL);

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
    // finish();
}
