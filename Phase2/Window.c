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
#define TONUM(x) (x - '0')
#define esc_key 27
#define DEL_KEY 127
#define BACKSPACE 8
#define NEWLINE '\n'
#define CARRIAGERETURN '\r'
#define STX 2  // start of text (ctrl + b)
#define ETX 3  // end of text (ctrl + c)
#define CAN 24 // cancel (ctrl + x)

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
const char EXITCOM = CAN;
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

struct FINDDATATYPE
{
    int *first_ix;
    int *last_ix;
    int cnt;
};

/// @brief creates a new finddatatype
/// @param _fix first_ix
/// @param _six last_ix
/// @param _cnt cnt
/// @return data type
struct FINDDATATYPE *newfinddatatype(int *_fix, int *_six, int _cnt)
{
    struct FINDDATATYPE *new = (struct FINDDATATYPE *)malloc(sizeof(struct FINDDATATYPE));

    new->first_ix = _fix;
    new->last_ix = _six;
    new->cnt = _cnt;

    return new;
}

/// @brief Get finddata[at]
/// @param fdt
/// @param at
/// @param _fix pass by pointer
/// @param _six pass by pointer
void getfinddata(struct FINDDATATYPE *fdt, int at, int *_fix, int *_six)
{
    *_fix = fdt->first_ix[at % fdt->cnt];
    *_six = fdt->last_ix[at % fdt->cnt];
}

struct SCRCUR
{
    struct SCREEN *scr;
    struct FINDDATATYPE *fdt;
    COORD *cursor;
    COORD *initcursor;
    int count;
    int hghlt;
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
        if (c == '\n')
        {
            linesize[curline++] = curlen;
            curlen = 0;
        }
        else
        {
            curlen++;
        }

        c = fgetc(OUTPUT);
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
/// @param highlight if 0, won't highlight
/// @param fdt finddatatype
void printOutputToScr(
    enum STATE state, int sidelen, int start, int end, int startcol, int endcol,
    int scrrow, COORD *cursor, COORD *initcursor, int highlight, struct FINDDATATYPE *fdt)
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

    int firstisinit = 1;
    if (cmpCursor(&firstcrsr, &secondcrsr) != -1)
    {
        firstisinit = 0;
        swap(&firstcrsr, &secondcrsr);
    }

    int slct = 0;
    int quote = 0;
    int openparn = 0;
    int openbrkt = 0;
    int opencurl = 0;
    int fdtix = 0;
    int fdtsix = 0;
    int fdtlix = 0;
    int highlighting = 0;
    int curix = 0;

    if (highlight)
    {
        if (fdt->cnt && fdt)
        {
            getfinddata(fdt, fdtix++, &fdtsix, &fdtlix);
        }
    }

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
            int beforefirst = (cmpCursorXY(&firstcrsr, X + sidelen + 1, Y) == 1);
            int aftersecond = (cmpCursorXY(&secondcrsr, X + sidelen + 1, Y) != 1);

            if (!firstisinit)
            {
                beforefirst |= !cmpCursorXY(&firstcrsr, X + sidelen + 1, Y);
            }
            // else
            // {
            //     beforefirst |= cmpCursorXY(&secondcrsr, X + sidelen + 1, Y);
            // }

            if (beforefirst || aftersecond)
            {
                totext();
                slct = 0;
            }
            else
            {
                toslct();
                slct = 1;
            }
        }

        if (highlight)
        {
            if (fdt->cnt && fdt)
            {
                if (curix >= fdtsix && curix < fdtlix)
                {
                    tohglt();
                }
                else if (curix >= fdtlix)
                {
                    totext();
                    getfinddata(fdt, fdtix++, &fdtsix, &fdtlix);
                    if (curix >= fdtsix && curix < fdtlix)
                    {
                        tohglt();
                    }
                }
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
            curix++;
            continue;
        }

        if (c == '~')
        {
            c = fgetc(OUTPUT);
            curix++;
            if (c == '!')
            {
                __red__();
                c = fgetc(OUTPUT);
                curix++;
                continue;
            }
            else if (c == '?')
            {
                __cyan__();
                c = fgetc(OUTPUT);
                curix++;
                continue;
            }
            else if (c == '&')
            {
                __red__();
                c = fgetc(OUTPUT);
                curix++;
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
        curix++;
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
    printOutputToScr(scrcur->scr->state, sl, scrcur->scr->startrow, safeendrow, scrcur->scr->startcol,
                     safeendcol, rows, scrcur->cursor, scrcur->initcursor, scrcur->hghlt, scrcur->fdt);
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

/// @brief Handles ~
/// @param thispath
/// @param thatpath
void pasteAndHandleTilde(char *thispath, char *thatpath)
{
    FILE *this = openPath(thispath);
    FILE *that = openPath(thatpath);

    char c = fgetc(this);
    while (c != EOF)
    {
        if (c == '~')
        {
            c = fgetc(this);
            if (c == '!')
            {
                c = fgetc(this);
                continue;
            }
            else if (c == '?')
            {
                c = fgetc(this);
                continue;
            }
            else if (c == '&')
            {
                c = fgetc(this);
                continue;
            }
            else if (c == '^')
            {
                fputs(ENDBRANCH, that);
                c = fgetc(OUTPUT);
                continue;
            }
            else if (c == '`')
            {
                fputs(MIDBRANCH, that);
                c = fgetc(OUTPUT);
                continue;
            }
            else if (c == '%')
            {
                fputs(PERBRANCH, that);
                c = fgetc(OUTPUT);
                continue;
            }
            else
            {
                fputc('~', that);
            }
        }

        fputc(c, that);
        c = fgetc(OUTPUT);
    }

    fclose(this);
    fclose(that);
}

/// @brief Generates description
/// @param chars
/// @param words
/// @param lines
/// @param atrow cursor row
/// @param atcol cursor col
/// @param count number of selected chars
/// @param state
/// @return desc string
char *getdesc(int chars, int words, int lines, int atrow, int atcol, int count, enum STATE state)
{
    char *out = calloc(512, sizeof(char));
    strcpy(out, __itoa(chars));
    strcat(out, chars == 1 ? " character, " : " characters, ");
    strcat(out, __itoa(words));
    strcat(out, words == 1 ? " word, " : " words, ");
    strcat(out, __itoa(lines));
    strcat(out, lines == 1 ? " line" : " lines");

    if (state == VISUAL)
    {
        strcat(out, " | ");
        strcat(out, __itoa(count));
        strcat(out, count == 1 ? " selected character" : " selected characters");
    }

    if (state == INSERT)
    {
        strcat(out, " | Ln ");
        strcat(out, __itoa(atrow));
        strcat(out, ", Col ");
        strcat(out, __itoa(atcol));
    }

    return out;
}

/// @brief Generate find description
/// @param all
/// @param at
/// @return desc string
char *getFindDesc(int all, int at)
{
    if (all == 0)
    {
        return "No matches";
    }

    char *out = calloc(512, sizeof(char));
    strcpy(out, __itoa((at + all) % all + 1));
    strcat(out, " of ");
    strcat(out, __itoa(all));
    strcat(out, all == 1 ? " match" : " matches");
    return out;
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

    char *desc = getdesc(chars, words, lines, 1, 0, 0, state);

    int *linesize = calloc(lines + 4, sizeof(int));

    _clearOutput();
    cat(path);
    getlinesize(linesize);

    _clearOutput();

    int maxlen = (int)ceil(log10(lines + 1));

    struct SCRCUR *scrcur = (struct SCRCUR *)malloc(sizeof(struct SCRCUR));
    scrcur->cursor = (COORD *)malloc(sizeof(COORD));
    scrcur->scr = (struct SCREEN *)malloc(sizeof(struct SCREEN));
    scrcur->initcursor = (COORD *)malloc(sizeof(COORD));
    scrcur->fdt = (struct FINDDATATYPE *)malloc(sizeof(struct FINDDATATYPE));
    scrcur->hghlt = 0;

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
        *scrcur->initcursor = *scrcur->cursor;
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

        Sleep(1000);

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
    Sleep(1000);
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
    Sleep(1000);
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
/// @return 1 if saved successfully
int savefile(struct SCRCUR *scrcur)
{
    if (!strcmp(scrcur->scr->initfilepath, untitledPathCat))
    {
        printMessageInCLIWOwaiting(scrcur, "Provide a name/path for your file: ");
        char *str = calloc(1024, sizeof(char));
        gets(str);

        _deleteArgs();
        int argc = buildArgs(str);

        char *arg[argc];
        for (int i = 0; i < argc; i++)
        {
            arg[i] = _getarg(i);
        }

        if (argc != 1)
        {
            printErrorInCLI(scrcur, "Invalid input");
            return 0;
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
                return 1;
            }
            else
            {
                return 0;
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
                return 1;
            }
            else
            {
                return 0;
            }
        }
        else
        {
            return 0;
        }

        return 0;
    }
    else
    {
        if (!scrcur->scr->saved)
        {
            pasteFileInto(untitledPathCat, scrcur->scr->initfilepath);
            *scrcur = *showfile(scrcur->scr->initfilepath, NORMAL);
            scrcur->scr->saved = 1;
            printMessageInCLI(scrcur, "File saved successfully");
            return 1;
        }
        else
        {
            return 1;
        }
    }

    return 0;
}

/// @brief Saves a file by force
/// @param scrcur
void forcesave(struct SCRCUR *scrcur)
{
    while (!savefile(scrcur))
        ;
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

    int col_left = 0;
    int col_right = min(avlcol, scrcur->scr->linesize[torow]);

    if (tocol + 4 > avlcol)
    {
        col_right = min(scrcur->scr->linesize[torow], torow + 4);
        col_left = col_right - avlcol;
    }

    if (col_left < 0)
    {
        col_left = max(0, scrcur->scr->linesize[torow] - avlcol + 1);
        col_right = col_left + avlcol - 1;
    }

    int row_up = max(1, torow - 4);
    int row_down = row_up + avlrow;

    if (row_down > scrcur->scr->maxrow + 1)
    {
        row_down = scrcur->scr->maxrow + 1;
        row_up = max(0, row_down - avlrow);
    }

    int mxrow = scrcur->scr->maxrow;

    if (torow > 4 && torow <= mxrow - 4 && torow - row_up < 4)
    {
        int d = abs(row_up - (torow - 4));
        if (row_up - d > 0)
        {
            row_down -= d;
            row_up -= d;
        }
    }

    if (torow > 4 && torow <= mxrow - 4 && row_down - torow < 4)
    {
        int d = abs(row_down - (torow + 4));
        if (row_down + d < mxrow)
        {
            row_down += d;
            row_up += d;
        }
    }

    int mxcol = scrcur->scr->linesize[torow];

    if (tocol > 4 && tocol <= mxcol - 4 && tocol - col_left < 4)
    {
        int d = abs(col_left - (tocol - 4));
        if (col_left - d >= 0)
        {
            col_right -= d;
            col_left -= d;
        }
    }

    if (tocol > 4 && tocol <= mxcol - 4 && col_right - tocol < 4)
    {
        int d = abs(col_right - (tocol + 4));
        if (col_right + d < mxcol)
        {
            col_right += d;
            col_left += d;
        }
    }

    scrcur->cursor->X = tocol + scrcur->scr->sidelen + 1;
    scrcur->cursor->Y = torow;
    scrcur->initcursor->X = scrcur->cursor->X;
    scrcur->initcursor->Y = scrcur->cursor->Y;
    scrcur->count = 0;

    scrcur->scr->activestart = scrcur->cursor->Y;
    scrcur->scr->activeend = scrcur->cursor->Y + 1;

    scrcur->scr->startcol = col_left;
    scrcur->scr->endcol = col_left + avlcol;
    scrcur->scr->startrow = row_up;
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

/// @brief Get outputs
/// @param cnt number of numbers in the output
/// @return an array of numbers
int *getoutputnums(int cnt)
{
    int *out = calloc(cnt, sizeof(int));

    OUTPUT = fopen(outputPath, "r");
    char c = fgetc(OUTPUT);
    int ix = 0;
    int num = 0;

    while (c != EOF)
    {
        if (c == ',')
        {
            out[ix++] = num;
            num = 0;
        }
        else if (ISNUM(c))
        {
            num = num * 10 + TONUM(c);
        }

        c = fgetc(OUTPUT);
    }

    out[ix] = num;

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

/// @brief Moves cursor to a certain index
/// @param scrcur
/// @param ix index
void moveCursorToIndex(struct SCRCUR *scrcur, int ix)
{
    int row, col;
    getRowColbyindex(scrcur->scr->filepath, ix, &row, &col);
    moveCursorTo(scrcur, row, col);
}

/// @brief Same as function.c::Handler but doesn't show output
/// @param inp
void NoOutputHandler(char *inp)
{
    chdir(parentDir);
    int n = strlen(inp);

    FILE *firstf = fopen(".first.first", "w");
    FILE *secondf = fopen(".second.second", "w");

    int inquote = 0;
    int isarman = 0;

    for (int i = 0; i < n - 4; i++)
    {
        if (inp[i] == '"')
        {
            int isrealquote = (i != 0 && inp[i - 1] != '\\');

            if (isrealquote)
            {
                inquote = !inquote;
            }
        }

        if (!inquote && inp[i] == ' ' && inp[i + 1] == '=' && inp[i + 2] == 'D' && inp[i + 3] == ' ')
        {
            for (int j = 0; j < i; j++)
            {
                fputc(inp[j], firstf);
            }
            fputc(EOF, firstf);

            for (int j = i + 4; j < n; j++)
            {
                fputc(inp[j], secondf);
            }
            fputc(EOF, secondf);

            isarman = 1;
            break;
        }
    }

    if (isarman)
    {
        fclose(firstf);
        fclose(secondf);
        firstf = fopen(".first.first", "r");
        secondf = fopen(".second.second", "r");

        handler(__toString(firstf));
        FILE *fp = fopen(outputPath, "r");
        char *str = __toString(fp);
        fclose(fp);

        _clearOutput();
        _deleteArgs();

        arman(__toString(secondf), str);
        fclose(fp);
        fclose(firstf);
        fclose(secondf);
    }
    else
    {
        handler(inp);
        fclose(firstf);
        fclose(secondf);
    }

    remove(".first.first");
    remove(".second.second");

    _deleteArgs();
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
            _deleteArgs();
            return;
        }

        savefile(scrcur);
    }
    else if (!strcmp(arg[0], SVAS_COM))
    {
        if (argc != 2)
        {
            printErrorInCLI(scrcur, "Invalid command");
            _deleteArgs();
            return;
        }

        saveasfile(scrcur, arg[1]);
    }
    else if (!strcmp(arg[0], OPEN_COM))
    {
        if (argc != 2)
        {
            printErrorInCLI(scrcur, "Invalid command");
            _deleteArgs();
            return;
        }

        int rtcf = readytoCreateFile(scrcur, arg[1]);

        if (rtcf == 1)
        {
            forcesave(scrcur);
            createFile(arg[1]);
            *scrcur = *showfile(arg[1], NORMAL);
        }
        else if (rtcf == 2)
        {
            forcesave(scrcur);
            *scrcur = *showfile(arg[1], NORMAL);
        }

        _deleteArgs();
        return;
    }
    else if (!strcmp(arg[0], UNDO_COM))
    {
        if (argc != 1)
        {
            printErrorInCLI(scrcur, "Invalid command");
            _deleteArgs();
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
        if (lineisempty(str))
        {
            _deleteArgs();
            return;
        }

        NoOutputHandler(str);

        _makeACopy("/root/.output/out.out");
        _clearOutput();

        forcesave(scrcur);
        emptyFile(untitledPathCat);
        *scrcur = *showfile(untitledPathCat, NORMAL);

        fclose(fopen("root\\.output\\.lastout.out", "w"));

        pasteAndHandleTilde("/root/.output/.out.outcopy", "/root/.output/.lastout.out");

        *scrcur = *showfile("/root/.output/.lastout.out", NORMAL);
        scrcur->scr->initfilepath = untitledPathCat;

        remove("/root/.output/.out.outcopy");
        remove("/root/.output/.lastout.out");
    }

    _deleteArgs();
}

/// @brief Finds a pattern in the file
/// @param __file_path file name
/// @param pat_ pattern to be matched
/// @return cnt = -3: Invalid Regex, otherwise a finddatatype
struct FINDDATATYPE *findPh2(char *__file_path, char *pat_)
{
    char *path = malloc(512 * sizeof(char));
    strcpy(path, __file_path);

    int i, foundDot = 0;
    for (i = (int)strlen(path) - 1; path[i] != '/' && i > -1; i--)
    {
        foundDot |= (path[i] == '.');
    }

    if (i == -1 || !foundDot)
    {
        chdir(parentDir);
        return NULL;
    }

    char *filename = malloc((unsigned int)((int)strlen(path) - 1 - i) * sizeof(char));

    int _blahblah_ = (int)strlen(path);
    for (int j = i + 1; j < _blahblah_; j++)
    {
        filename[j - i - 1] = path[j];
    }

    filename[(int)strlen(path) - 1 - i] = '\0';
    path[i] = '\0';

    char *tok;
    int isFirst = 1;

    while ((tok = strtok(path, "/")) != NULL)
    {
        if (isFirst && strcmp(tok, "root"))
        {
            chdir(parentDir);
            return NULL;
        }

        isFirst = 0;

        if (chdir(tok))
        {
            chdir(parentDir);
            return NULL;
        }

        path = NULL;
    }

    if (!__fileExists(filename))
    {
        chdir(parentDir);
        return NULL;
    }

    FILE *fp = fopen(filename, "r+");

    char *str = __toString(fp);

    int pats;
    short *pat = __toPat(pat_, &pats);
    if ((pats == 1 && pat[0] == -1) || (pats == 2 && pat[0] == -1 && pat[1] == -1))
    {
        // _writeToOutput("~!ERROR: Invalid regex\n");
        return newfinddatatype(NULL, NULL, -3);
    }

    int cmatch[512];
    int lmatch[512];
    int matchno = 0;

    int _dum_ = (int)strlen(str);
    for (int ii = 0; ii < _dum_; ii++)
    {
        int last_ix = ii;
        int c_index = __find(str, pat, pats, ii, &last_ix);
        if (c_index != -1)
        {
            if (
                pat[0] == -1 &&
                (!c_index ||
                 (c_index && (str[c_index - 1] == ' ' || str[c_index - 1] == '\n' || str[c_index - 1] == '\t'))))
            {
                continue;
            }
            else if (
                pat[pats - 1] == -1 &&
                (last_ix - 1 == _dum_ ||
                 ((last_ix - 1 != _dum_) && (str[last_ix - 1] == ' ' || str[last_ix - 1] == '\n' || str[last_ix - 1] == '\t'))))
            {
                continue;
            }
            else
            {
                cmatch[matchno] = c_index;
                lmatch[matchno] = last_ix;
                matchno++;
                ii = c_index;
            }
        }
    }

    int *_fix = calloc(matchno, sizeof(int));
    int *_six = calloc(matchno, sizeof(int));

    for (int ii = 0; ii < matchno; ii++)
    {
        _fix[ii] = cmatch[ii];
        _six[ii] = lmatch[ii];
    }

    struct FINDDATATYPE *fdt = newfinddatatype(_fix, _six, matchno);

    fclose(fp);
    chdir(parentDir);
    return fdt;
}

/// @brief Updates screen
/// @param scrcur
/// @param atrow cursor
/// @param atcol cursor
void updateScrcur(struct SCRCUR *scrcur, int atrow, int atcol)
{
    int chars, words, lines, maxcol;
    __originFileLen(scrcur->scr->filepath, &chars, &lines, &words, &maxcol);
    chdir(parentDir);
    int *linesize = calloc(lines + 10, sizeof(int));

    char newpath[] = ".\\root\\.thisissthyoudontfuckwth.dontfuck";
    char newpathcat[] = "/root/.thisissthyoudontfuckwth.dontfuck";

    _clearOutput();
    fclose(fopen(newpath, "w"));

    pasteFileInto(untitledPathCat, newpathcat);

    cat(newpathcat);
    getlinesize(linesize);
    _clearOutput();
    remove(newpath);

    int maxlen = (int)ceil(log10(lines + 1));

    scrcur->scr->sidelen = maxlen + 1;
    scrcur->scr->linesize = linesize;
    scrcur->scr->maxcol = maxcol;
    scrcur->scr->maxrow = lines + 1;

    int X1, X2, Y1, Y2;
    if (cmpCursor(scrcur->cursor, scrcur->initcursor) == 1)
    {
        X1 = scrcur->initcursor->X - scrcur->scr->sidelen - 1;
        X2 = scrcur->cursor->X - scrcur->scr->sidelen - 1;
        Y1 = scrcur->initcursor->Y;
        Y2 = scrcur->cursor->Y;
    }
    else
    {
        X2 = scrcur->initcursor->X - scrcur->scr->sidelen - 1;
        X1 = scrcur->cursor->X - scrcur->scr->sidelen - 1;
        Y2 = scrcur->initcursor->Y;
        Y1 = scrcur->cursor->Y;
    }

    int cnt = 0;
    if (Y1 == Y2)
    {
        cnt = X2 - X1;
    }
    else
    {
        cnt += linesize[Y1] - X1;
        for (int i = Y1 + 1; i < Y2 - 1; i++)
            cnt += linesize[i];
        cnt += X2;
        cnt += Y2 - Y1;
    }

    scrcur->count = cnt;

    scrcur->scr->desc = getdesc(chars, words, lines, atrow, atcol, scrcur->count, scrcur->scr->state);
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
            updateScrcur(scrcur, scrcur->cursor->Y, scrcur->cursor->X - scrcur->scr->sidelen - 1);
        }
        else if (com == PH1COM)
        {
            char *str = calloc(32768, sizeof(char));
            gets(str);
            handleColonCommands(scrcur, str, row);
            updateScrcur(scrcur, scrcur->cursor->Y, scrcur->cursor->X - scrcur->scr->sidelen - 1);
            free(str);
        }
        else if (com == CMDCOM)
        {
            char *str = calloc(32768, sizeof(char));
            gets(str);
            handleChmodCommands(scrcur, str, row);
            updateScrcur(scrcur, scrcur->cursor->Y, scrcur->cursor->X - scrcur->scr->sidelen - 1);
            free(str);
        }
        else if (com == FNDCOM)
        {
            char *str = calloc(32768, sizeof(char));
            gets(str);

            _deleteArgs();
            int argc = buildArgs(str);

            if (argc != 1)
            {
                _deleteArgs();
                free(str);
                return;
            }

            char *patstr = _getarg(0);

            struct FINDDATATYPE *fdt = findPh2(scrcur->scr->filepath, patstr);

            if (!fdt)
            {
                _deleteArgs();
                free(str);
                return;
            }

            if (fdt->cnt == -3)
            {
                printErrorInCLI(scrcur, "Invalid regex");
                _deleteArgs();
                free(str);
                return;
            }

            if (!fdt->cnt)
            {
                printMessageInCLI(scrcur, "No match");
                _deleteArgs();
                free(str);
                return;
            }

            int ix = 0, this_fix, this_lix;

            getfinddata(fdt, ix++, &this_fix, &this_lix);
            moveCursorToIndex(scrcur, this_fix);
            scrcur->scr->desc = getFindDesc(fdt->cnt, ix - 1);
            scrcur->hghlt = 1;
            *(scrcur->fdt) = *fdt;
            initscr(scrcur);

            while (getch() == NXTCOM)
            {
                getfinddata(fdt, ix++, &this_fix, &this_lix);
                moveCursorToIndex(scrcur, this_fix);
                scrcur->scr->desc = getFindDesc(fdt->cnt, ix - 1);
                initscr(scrcur);
            }

            scrcur->hghlt = 0;
            updateScrcur(scrcur, scrcur->cursor->Y, scrcur->cursor->X - scrcur->scr->sidelen - 1);
            _deleteArgs();
            free(str);
        }
        else if (com == INDCOM)
        {
            _makeACopy(scrcur->scr->filepath);
            _pasteCopyInto(scrcur->scr->filepath, autoIndent(scrcur->scr->filepath));
            updateScrcur(scrcur, scrcur->cursor->Y, scrcur->cursor->X - scrcur->scr->sidelen - 1);
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
            copyStr(scrcur->scr->filepath, firstcursor->Y, firstcursor->X - scrcur->scr->sidelen - 1,
                    scrcur->count, SIGN(scrcur->count));
            scrcur->scr->state = INSERT;
            return;
        }
        else if (com == CUTCOM)
        {
            _makeACopy(scrcur->scr->filepath);
            int changed = cutStr(scrcur->scr->filepath, firstcursor->Y, firstcursor->X - scrcur->scr->sidelen - 1, scrcur->count,
                                 SIGN(scrcur->count));

            _pasteCopyInto(scrcur->scr->filepath, changed);

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

            updateScrcur(scrcur, scrcur->cursor->Y, scrcur->cursor->X - scrcur->scr->sidelen - 1);

            return;
        }
        else if (com == DELCOM)
        {

            _makeACopy(scrcur->scr->filepath);
            int changed = removeStr(scrcur->scr->filepath, firstcursor->Y, firstcursor->X - scrcur->scr->sidelen - 1, scrcur->count,
                                    SIGN(scrcur->count));
            _pasteCopyInto(scrcur->scr->filepath, changed);

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

            updateScrcur(scrcur, scrcur->cursor->Y, scrcur->cursor->X - scrcur->scr->sidelen - 1);

            return;
        }
    }

    if (scrcur->scr->state == INSERT)
    {
        if (com == STX)
        {
            strcat(scrcur->scr->desc, " (Inserting text)");
            initscr(scrcur);
            char *dummytxt = calloc(1, sizeof(char));
            dummytxt[0] = getch();

            while (dummytxt[0] != ETX)
            {
                int currow = scrcur->cursor->Y;
                int curcol = scrcur->cursor->X - scrcur->scr->sidelen - 1;
                int torow, tocol;
                dummytxt[1] = '\0';

                if (dummytxt[0] != BACKSPACE && dummytxt[0] != DEL_KEY)
                {
                    /* insert c into the file */
                    if (dummytxt[0] == CARRIAGERETURN)
                    {
                        dummytxt[0] = NEWLINE;
                    }

                    _makeACopy(scrcur->scr->filepath);
                    _pasteCopyInto(scrcur->scr->filepath, insertStr(scrcur->scr->filepath, dummytxt, currow, curcol));

                    scrcur->scr->saved = 0;
                    if (dummytxt[0] == NEWLINE)
                    {
                        torow = currow + 1;
                        tocol = 0;
                    }
                    else
                    {
                        torow = currow;
                        tocol = curcol + 1;
                    }

                    updateScrcur(scrcur, torow, tocol);
                    moveCursorTo(scrcur, torow, tocol);
                }
                else if (dummytxt[0] == BACKSPACE)
                {
                    if (currow != 1 || curcol != 0)
                    {
                        int prevlinesize = scrcur->scr->linesize[currow - 1];
                        _makeACopy(scrcur->scr->filepath);
                        _pasteCopyInto(scrcur->scr->filepath, removeStr(scrcur->scr->filepath, currow, curcol, 1, 0));

                        scrcur->scr->saved = 0;
                        if (curcol == 0)
                        {
                            tocol = prevlinesize;
                            torow = currow - 1;
                        }
                        else
                        {
                            torow = currow;
                            tocol = curcol - 1;
                        }

                        updateScrcur(scrcur, torow, tocol);
                        moveCursorTo(scrcur, torow, tocol);
                    }
                }
                else if (dummytxt[0] == DEL_KEY)
                {
                    if (currow != scrcur->scr->maxrow || curcol != scrcur->scr->linesize[currow])
                    {
                        _makeACopy(scrcur->scr->filepath);
                        _pasteCopyInto(scrcur->scr->filepath, removeStr(scrcur->scr->filepath, currow, curcol, 1, 1));

                        scrcur->scr->saved = 0;

                        updateScrcur(scrcur, torow, tocol);
                        moveCursorTo(scrcur, torow, tocol);
                    }
                }

                strcat(scrcur->scr->desc, " (Inserting text)");
                initscr(scrcur);
                dummytxt[0] = getch();
            }

            updateScrcur(scrcur, scrcur->cursor->Y, scrcur->cursor->X - scrcur->scr->sidelen - 1);

            return;
        }
        if (com == PSTCOM)
        {
            _makeACopy(scrcur->scr->filepath);
            int changed = pasteStr(scrcur->scr->filepath, scrcur->cursor->Y, scrcur->cursor->X - scrcur->scr->sidelen - 1);
            _pasteCopyInto(scrcur->scr->filepath, changed);
            scrcur->scr->saved = !changed;

            updateScrcur(scrcur, scrcur->cursor->Y, scrcur->cursor->X - scrcur->scr->sidelen - 1);

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

        if (scrcur->cursor->X == scrcur->scr->linesize[scrcur->cursor->Y] + scrcur->scr->sidelen + 1)
        {
            return;
        }
        else if (scrcur->cursor->X - scrcur->scr->startcol - 1 >= col - 4 - scrcur->scr->sidelen)
        {
            if (scrcur->scr->endcol <= scrcur->scr->linesize[scrcur->cursor->Y])
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

    updateScrcur(scrcur, scrcur->cursor->Y, scrcur->cursor->X - scrcur->scr->sidelen - 1);
}

void runFile(char *path, enum STATE state)
{
    struct SCRCUR *scrcur = showfile(path, state);

    char c;

    if (scrcur->scr->state == NORMAL)
        c = getche();
    else
        c = getch();

    while (c != EXITCOM)
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
    init();
    runFile("/root/bruh/wtf/this/is/a/test/myfile1.txt", NORMAL);
    finish();
}
