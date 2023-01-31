#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <math.h>
#include <dirent.h>
#include <ctype.h>
#include <limits.h>
#include <stdint.h>

enum FINDMASK
{
    AT = 1 << 3,
    BYWORD = 1 << 2,
    COUNT = 1 << 1,
    ALL = 1 << 0
};

enum COMMAND
{
    CREATEFILE,
    INSERTSTR,
    CAT,
    REMOVESTR,
    COPYSTR,
    CUTSTR,
    PASTESTR,
    FIND,
    REPLACE,
    GREP,
    UNDO,
    AUTOINDENT,
    COMPARE,
    TREE,
    INVALID
};

char *COMSTR[] = {
    "createfile",
    "insertstr",
    "cat",
    "removestr",
    "copystr",
    "cutstr",
    "pastestr",
    "find",
    "replace",
    "grep",
    "undo",
    "auto-indent",
    "compare",
    "tree"};

enum COMMAND ixtocom[] = {
    CREATEFILE,
    INSERTSTR,
    CAT,
    REMOVESTR,
    COPYSTR,
    CUTSTR,
    PASTESTR,
    FIND,
    REPLACE,
    GREP,
    UNDO,
    AUTOINDENT,
    COMPARE,
    TREE,
    INVALID};

char parentDir[512];
char clipboardPath[512];
char outputPath[512];
char argsPath[512];
char clipboardPathCat[512];
FILE *CLIPBOARD;
FILE *OUTPUT;
int currargc;

const unsigned char ENDBRANCH[] = {192, ' ', '\0'};
const unsigned char MIDBRANCH[] = {195, ' ', '\0'};
const unsigned char PERBRANCH[] = {179, '\0'};

/// @brief Changes output color to purple
void __boldpurple__()
{
    printf("\033[1;35m");
}

/// @brief Resets output color
void __reset__()
{
    printf("\033[0m");
}

/// @brief Changes output color to red
void __red__()
{
    printf("\033[0;31m");
}

/// @brief Changes output color to red
void __cyan__()
{
    printf("\033[0;36m");
}

/// @brief Checks wether a string is a number
/// @param str string
/// @return 1 if is number
int _isnum(char *str)
{
    for (int i = 0; str[i] != '\0'; i++)
    {
        if (!isdigit(str[i]))
        {
            return 0;
        }
    }

    return 1;
}

/// @brief Converts a POSITIVE string to int
/// @param str string
/// @return number
int _tonum(char *str)
{
    int out = 0;
    for (int i = 0; str[i] != '\0'; i++)
    {
        out = out * 10 + (str[i] - '0');
    }
    return out;
}

/// @brief Converts int to string
/// @param a number
/// @return string
char *__itoa(int a)
{
    if (a == 0)
    {
        char *out = malloc(sizeof(char));
        strcpy(out, "0");
        return out;
    }

    int enough = (int)(ceil(log10(abs(a))) + 2);
    char *out = malloc(enough * sizeof(char));

    sprintf(out, "%d", a);

    return out;
}

/// @brief Checks if a certain directory exists
/// @param path file path
/// @return 1 if exists, 0 otherwise
int __fileExists(char *path)
{
    FILE *file = fopen(path, "r");

    if (file == NULL)
        return 0;

    fclose(file);

    return 1;
}

/// @brief Copies a file into another
/// @param source source file
/// @param dest destination file
void __copy(FILE *source, FILE *dest)
{
    char c;
    while ((c = fgetc(source)) != EOF)
        fputc(c, dest);
}

/// @brief Prints to output
/// @param txt text to be written
void _writeToOutput(const char *txt)
{
    char cwd[512];
    getcwd(cwd, 512);

    chdir(parentDir);

    OUTPUT = fopen(outputPath, "a");
    fprintf(OUTPUT, "%s", txt);

    fclose(OUTPUT);

    chdir(cwd);
}

/// @brief Clears output
void _clearOutput()
{
    char cwd[512];
    getcwd(cwd, 512);

    chdir(parentDir);

    OUTPUT = fopen(outputPath, "w");
    fclose(OUTPUT);

    chdir(cwd);
}

/// @brief Shows output in stdout
void _showOutput()
{
    char cwd[512];
    getcwd(cwd, 512);

    chdir(parentDir);

    OUTPUT = fopen(outputPath, "r");

    char c = fgetc(OUTPUT);
    while (c != EOF)
    {
        if (c == '\n' || c == '\0' || c == EOF)
        {
            __reset__();
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
            else if (c == '^')
            {
                printf("%s", ENDBRANCH);
                c = fgetc(OUTPUT);
                continue;
            }
            else if (c == '`')
            {
                printf("%s", MIDBRANCH);
                c = fgetc(OUTPUT);
                continue;
            }
            else if (c == '%')
            {
                printf("%s", PERBRANCH);
                c = fgetc(OUTPUT);
                continue;
            }
            else
            {
                printf("~");
            }
        }

        printf("%c", c);
        c = fgetc(OUTPUT);
    }

    fclose(OUTPUT);

    chdir(cwd);
}

/// @brief Copy-path of a file
/// @param filename file
/// @return copy-path
char *__copyPath(char *filename)
{
    char *out = malloc((strlen(filename) + strlen(parentDir) + 30) * sizeof(char));

    out[0] = '\0';
    strcpy(out, ".");
    strcat(out, filename);
    strcat(out, "copy");

    return out;
}

/// @brief Undo-path of a file
/// @param filename file
/// @return undo-path
char *__undoPath(char *filename)
{
    char *out = malloc((strlen(filename) + strlen(parentDir) + 30) * sizeof(char));

    out[0] = '\0';
    strcpy(out, ".");
    strcat(out, filename);
    strcat(out, "undo");

    return out;
}

/// @brief Updates undo
/// @param __file_path file
void __update(char *__file_path)
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
        return;
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
            return;
        }

        isFirst = 0;

        if (chdir(tok))
        {
            _writeToOutput("Invalid directory.");
            chdir(parentDir);
            return;
        }

        path = NULL;
    }

    if (!__fileExists(filename))
    {
        _writeToOutput("~!ERROR: File  ~?");
        _writeToOutput(__file_path);
        _writeToOutput(" ~&does not exist\n");
        free(path);
        chdir(parentDir);
        return;
    }

    FILE *fp = fopen(filename, "r");

    FILE *fcp = fopen(__undoPath(filename), "w");

    __copy(fp, fcp);

    fclose(fp);
    fclose(fcp);
    chdir(parentDir);
}

/// @brief Checks if the file is empty
/// @param file source file
/// @return 1 if empty
int __isEmpty(FILE *file)
{
    int c = fgetc(file);
    if (c == EOF)
    {
        return 1;
    }
    ungetc(c, file);

    return 0;
}

/// @brief Starts the application
void init()
{
    getcwd(parentDir, 512);

    _mkdir("root");
    chdir("root");
    _mkdir(".clipboard");
    chdir(".clipboard");

    getcwd(clipboardPath, 512);
    strcat(clipboardPath, "\\clipboard.clp");

    strcpy(clipboardPathCat, "/root/.clipboard/clipboard.clp");

    CLIPBOARD = fopen("clipboard.clp", "w");
    fclose(CLIPBOARD);

    chdir("..");
    _mkdir(".output");
    chdir(".output");
    getcwd(outputPath, 512);
    strcat(outputPath, "\\out.out");
    OUTPUT = fopen("out.out", "w");
    fclose(OUTPUT);

    chdir("..");
    _mkdir(".args");
    chdir(".args");
    getcwd(argsPath, 512);
    currargc = 0;

    chdir(parentDir);
}

/// @brief Generates path of an arg
/// @param i arg index
/// @return path
char *_argpath(int i)
{
    char *out = malloc(512 * sizeof(char));
    strcpy(out, argsPath);
    strcat(out, "\\");
    strcat(out, __itoa(i));
    strcat(out, ".arg");

    return out;
}

/// @brief makes a new arg
void _addArg(char *txt)
{
    chdir(argsPath);

    FILE *fp = fopen(_argpath(currargc++), "w+");

    fputs(txt, fp);

    fclose(fp);
    chdir(parentDir);
}

/// @brief Empties the args folder
void _deleteArgs()
{
    while (currargc--)
    {
        remove(_argpath(currargc));
    }

    currargc = 0;
}

/// @brief Checks if a path is file or directory
/// @param path path
/// @return 1 if is file
int __isFile(const char *path)
{
    struct stat ps;
    stat(path, &ps);
    return S_ISREG(ps.st_mode);
}

/// @brief helper function to clear cache
void _removeDots()
{
    struct dirent *de;
    DIR *dr = opendir(".");

    if (dr == NULL)
    {
        return;
    }

    while ((de = readdir(dr)) != NULL)
    {
        if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, ".."))
        {
            continue;
        }

        if (!__isFile(de->d_name))
        {
            chdir(de->d_name);

            _removeDots();

            chdir("..");
        }
        else
        {
            if (de->d_name[0] == '.')
                remove(de->d_name);
        }
    }
}

/// @brief Empties the cache folders
void _deleteCache()
{
    chdir(parentDir);
    chdir("root");

    rmdir(".args");
    remove(".clipboard\\clipboard.clp");
    rmdir(".clipboard");
    remove(".output\\out.out");
    rmdir(".output");

    chdir(parentDir);
    chdir("root");

    _removeDots();
}

/// @brief Compares two files
/// @param thisfile first file
/// @param thatfile second file
/// @return 1 if they're equal, 0 if not
int _filesAreEqual(FILE *thisfile, FILE *thatfile)
{
    char thisc = fgetc(thisfile);
    char thatc = fgetc(thatfile);

    while (thisc != EOF && thatc != EOF)
    {
        if (thisc != thatc)
        {
            fseek(thisfile, 0, SEEK_SET);
            fseek(thatfile, 0, SEEK_SET);

            return 0;
        }

        thisc = fgetc(thisfile);
        thatc = fgetc(thatfile);
    }

    fseek(thisfile, 0, SEEK_SET);
    fseek(thatfile, 0, SEEK_SET);

    return thisc == EOF && thatc == EOF;
}

/// @brief Makes a copy of the file
/// @param __file_path the given file
void _makeACopy(char *__file_path)
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
        free(path);
        chdir(parentDir);
        return;
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
            chdir(parentDir);
            return;
        }

        isFirst = 0;

        if (chdir(tok))
        {
            chdir(parentDir);
            return;
        }

        path = NULL;
    }

    if (!__fileExists(filename))
    {
        free(path);
        chdir(parentDir);
        return;
    }

    FILE *fp = fopen(filename, "r");
    FILE *nfp = fopen(__copyPath(filename), "w");

    char _c = fgetc(fp);
    while (_c != EOF)
    {
        fputc(_c, nfp);
        _c = fgetc(fp);
    }

    fseek(fp, 0, SEEK_SET);
    fseek(nfp, 0, SEEK_SET);

    fclose(fp);
    fclose(nfp);
    chdir(parentDir);
}

/// @brief Pastes copied file into the given position
/// @param __file_path file to be pasted in
/// @param done should paste?
void _pasteCopyInto(char *__file_path, int done)
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
        free(path);
        chdir(parentDir);
        return;
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
            free(path);
            chdir(parentDir);
            return;
        }

        isFirst = 0;

        if (chdir(tok))
        {
            free(path);
            chdir(parentDir);
            return;
        }

        path = NULL;
    }

    if (!__fileExists(filename))
    {
        free(path);
        chdir(parentDir);
        return;
    }

    if (done)
    {
        FILE *nfp = fopen(__undoPath(filename), "w");
        FILE *cfp = fopen(__copyPath(filename), "r");

        char _c = fgetc(cfp);
        while (_c != EOF)
        {
            fputc(_c, nfp);
            _c = fgetc(cfp);
        }

        fclose(cfp);
        fclose(nfp);

        remove(__copyPath(filename));
    }

    free(path);
    chdir(parentDir);
}

/// @brief Ends the program
void finish()
{
    _deleteCache();
}

/// @brief Gets number of lines, words and characters in the file
/// @param __file_path file name
/// @param characters number of characters
/// @param lines number of lines
/// @param words number of words
/// @param maxlen maximum number of characters in a single line
void __originFileLen(char *__file_path, int *characters, int *lines, int *words, int *maxlen)
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
        _writeToOutput("~!ERROR: Invalid file name\n\n");
        free(path);
        chdir(parentDir);
        return;
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
            return;
        }

        isFirst = 0;

        if (chdir(tok))
        {
            _writeToOutput("Invalid directory.");
            chdir(parentDir);
            return;
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
        return;
    }

    char newPath[512];

    int jp;
    for (jp = 0; filename[jp] != '.'; jp++)
    {
        newPath[jp] = filename[jp];
    }

    newPath[jp] = '\0';

    strcat(newPath, ".tempofln");

    FILE *fp = fopen(filename, "r");
    FILE *nfp = fopen(newPath, "a");

    int c = fgetc(fp);
    fputc(c, nfp);
    *characters = 0;
    *lines = 0;
    *words = 0;
    *maxlen = 0;

    int curlen = 0;

    while (c != EOF)
    {
        (*characters)++;

        if (c == '\n')
        {
            *maxlen = *maxlen > curlen ? *maxlen : curlen;
            curlen = 0;
        }
        else
        {
            curlen++;
        }

        *lines += (c == '\n');
        *words += (c == '\n' || c == '\t' || c == ' ');
        c = fgetc(fp);
        fputc(c, nfp);
    }

    *maxlen = *maxlen > curlen ? *maxlen : curlen;
    curlen = 0;

    fclose(fp);
    fclose(nfp);

    fp = fopen(filename, "w");
    nfp = fopen(newPath, "r");

    __copy(nfp, fp);

    fclose(fp);
    fclose(nfp);

    remove(newPath);
}

/// @brief Gets number of lines, words and characters in the file
/// @param __file_path file name
/// @param characters number of characters
/// @param lines number of lines
/// @param words number of words
void __fileLen(char *__file_path, int *characters, int *lines, int *words)
{
    char path[512];
    strcpy(path, __file_path);

    char newPath[512];

    int jp;
    for (jp = 0; path[jp] != '.'; jp++)
    {
        newPath[jp] = path[jp];
    }

    newPath[jp] = '\0';

    strcat(newPath, ".tempfln");

    FILE *fp = fopen(path, "r");
    FILE *nfp = fopen(newPath, "a");

    int c = fgetc(fp);
    fputc(c, nfp);
    *characters = 0;
    *lines = 0;
    *words = 0;

    while (c != EOF)
    {
        (*characters)++;
        *lines += (c == '\n');
        *words += (c == '\n' || c == '\t' || c == ' ');
        c = fgetc(fp);
        fputc(c, nfp);
    }

    fclose(fp);
    fclose(nfp);

    fp = fopen(path, "w");
    nfp = fopen(newPath, "r");

    __copy(nfp, fp);

    fclose(fp);
    fclose(nfp);

    remove(newPath);
}

/// @brief Finds character index of row:col
/// @param __file_path file name
/// @param row starting line (1 indexed)
/// @param col starting column (0 indexed)
/// @return -1 if not found
int __index(char *__file_path, int row, int col)
{
    char path[512];
    strcpy(path, __file_path);

    char newPath[512];

    int jp;
    for (jp = 0; path[jp] != '.'; jp++)
    {
        newPath[jp] = path[jp];
    }

    newPath[jp] = '\0';

    strcat(newPath, ".tempinx");

    FILE *fp = fopen(path, "r");
    FILE *nfp = fopen(newPath, "a");

    int c = fgetc(fp);
    fputc(c, nfp);
    int found = 0;
    int _char = 0;
    int _c = 0;
    int _r = 1;

    while (c != EOF)
    {
        if (_r == row && _c == col)
        {
            found = 1;
        }

        _c++;
        _char += !found;
        if (c == '\n')
        {
            if (_r == row && _c == col)
            {
                found = 1;
            }

            _c = 0;
            _r++;
        }
        c = fgetc(fp);
        fputc(c, nfp);
    }

    if (_r == row && _c == col)
    {
        found = 1;
    }

    fclose(fp);
    fclose(nfp);

    fp = fopen(path, "w");
    nfp = fopen(newPath, "r");

    __copy(nfp, fp);

    fclose(fp);
    fclose(nfp);

    remove(newPath);

    return found ? _char : -1;
}

/// @brief Converts file to string
/// @param f file
/// @return String
char *__toString(FILE *f)
{
    if (f == NULL || fseek(f, 0, SEEK_END))
    {
        return NULL;
    }

    long length = ftell(f);
    rewind(f);
    // Did ftell() fail?  Is the length too long?
    if (length == -1 || (unsigned long)length >= SIZE_MAX)
    {
        return NULL;
    }

    // Convert from long to size_t
    size_t ulength = (size_t)length;
    char *buffer = malloc(ulength + 1);
    size_t g = 0;

    char c = fgetc(f);
    while (c != EOF)
    {
        buffer[g++] = c;
        c = fgetc(f);
    }

    rewind(f);

    buffer[ulength] = '\0'; // Now buffer points to a string

    return buffer;
}

/// @brief Returns arg as string
/// @param i arg index
/// @return arg
char *_getarg(int i)
{
    chdir(argsPath);
    FILE *fp = fopen(_argpath(i), "r");

    if (!fp)
        return NULL;

    char *f = malloc(32768 * sizeof(char));
    f = __toString(fp);

    fclose(fp);
    chdir(parentDir);

    return f;
}

/// @brief Matches two characters
/// @param patc pattern character
/// @param strc string character
/// @return 1 if matches, 0 if not
int __matchc(short patc, char strc)
{
    if (patc == -1)
    {
        return (strc != ' ' && strc != '\n' && strc != '\0' && strc != ' ' && strc != EOF);
    }

    return (strc == patc);
}

/// @brief Converts string to pattern (-1 means wildcard)
/// @param str input tring
/// @param retsize size of output
/// @return pattern as an array of ints
short *__toPat(char *str, int *retsize)
{
    int n = strlen(str);
    short *out = malloc((n + 1) * sizeof(short));
    int ixout = 0;

    for (int i = 0; i < n; i++)
    {
        if (str[i] == '*')
        {
            if (i == 0)
            {
                out[ixout++] = -1;
            }
            else
            {
                if (str[i - 1] == '\\')
                {
                    out[ixout - 1] = str[i];
                }
                else
                {
                    out[ixout++] = -1;
                }
            }
        }
        else
        {
            out[ixout++] = str[i];
        }
    }

    *retsize = ixout;
    out[n] = -2;

    return out;
}

/// @brief helper function for __find
/// @param str string
/// @param pat pattern
/// @param last_ix last index of match
/// @return 1 if found, 0 otherwise
int match_str(const char *str, const short *pat, int *lastix)
{
    const short *star = NULL;
    const char *ss = str;
    int last_dummy = *lastix;

    while (*str && *pat != -2)
    {
        if (*pat == -1)
        {
            if (__matchc(*pat, *str))
            {
                star = pat++;
                ss = str;
                last_dummy = *lastix;
                continue;
            }
            else
            {
                star = NULL;
                ss = str;
                last_dummy = *lastix;
                pat++;
                continue;
            }
        }
        else
        {
            if (__matchc(*pat, *str))
            {
                if (*str == ' ' || *str == '\n' || *str == '\t')
                {
                    star = NULL;
                }
                str++;
                (*lastix)++;
                pat++;
                continue;
            }
        }

        if (star)
        {
            pat = star + 1;
            if (*str == ' ' || *str == '\n' || *str == '\t')
            {
                star = NULL;
            }
            str = ++ss;
            *lastix = ++last_dummy;
            continue;
        }

        return 0;
    }

    while (*pat == -1)
    {
        pat++;
    }

    return *pat == -2;
}

/// @brief Matches two strings
/// @param str main string
/// @param pat pattern
/// @param pats size of pattern
/// @param s_i starting index
/// @return index of match, -1 if not found
int __find(char *str, short *pat, int pats, int s_i, int *last_ix)
{
    int strs = strlen(str);

    for (int i = s_i; i < strs; i++)
    {
        *last_ix = i;
        if (match_str(str + i, pat, last_ix))
        {
            while (i < strs && pat[0] == -1 && !__matchc(pat[1], str[i]))
            {
                i++;
            }

            while ((*last_ix) < strs && pat[pats - 1] == -1 && __matchc(pat[pats - 1], str[*last_ix]))
            {
                (*last_ix)++;
            }

            return i == strs ? -1 : i;
        }
    }

    return -1;
}

/// @brief Converts array of characters to array of words
/// @param str string
/// @param arrc array of characters
void __toWordArr(char *str, int *arrc)
{
    int wcnt = 1, ccnt = 0, prevs = 0;
    int _dum_ = strlen(str);
    for (int i = 0; i < _dum_; i++)
    {
        if (arrc[ccnt] == i)
        {
            arrc[ccnt++] = wcnt;
        }

        if (prevs && (str[i] == ' ' || str[i] == '\n' || str[i] == EOF || str[i] == '\0'))
        {
            wcnt++;
            prevs = 0;
            continue;
        }

        prevs++;
    }
}

/// @brief Creates a file
/// @param __file_path file name
void createFile(char *__file_path)
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
        return;
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
            _writeToOutput("~!ERROR: File should be created in root folder\n");
            chdir(parentDir);
            return;
        }

        isFirst = 0;

        _mkdir(tok);
        chdir(tok);

        path = NULL;
    }

    if (__fileExists(filename))
    {
        _writeToOutput("~!ERROR: File already exists\n");
        free(path);
        chdir(parentDir);
        return;
    }

    FILE *fp = fopen(filename, "w");

    if (fp)
    {
        _writeToOutput("File created successfully\n");
        fclose(fp);
    }
    else
    {
        _writeToOutput("~!ERROR: Could not create file\n");
    }

    chdir(parentDir);
}

/// @brief Inserts text in a particular position
/// @param __file_path file name
/// @param text text to be inserted
/// @param row starting line (1 indexed)
/// @param col starting column (0 indexed)
/// @return 1 if the file changed, 0 if not
int insertStr(char *__file_path, char *text, int row, int col)
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
            _writeToOutput("~!ERROR: File should be in root folder\n");
            chdir(parentDir);
            return 0;
        }

        isFirst = 0;

        if (chdir(tok))
        {
            _writeToOutput("~!ERROR: Invalid directory\n");
            chdir(parentDir);
            return 0;
        }

        path = NULL;
    }

    if (!__fileExists(filename))
    {
        _writeToOutput("~!ERROR: File  ~?");
        _writeToOutput(__file_path);
        _writeToOutput(" ~&does not exist\n");
        chdir(parentDir);
        return 0;
    }

    char newPath[512];

    int jp;
    for (jp = 0; filename[jp] != '.'; jp++)
    {
        newPath[jp] = filename[jp];
    }

    newPath[jp] = '\0';

    strcat(newPath, ".temp");

    FILE *fp = fopen(filename, "r");
    FILE *nfp = fopen(newPath, "a");

    int r = 1;

    char *line = NULL;
    size_t len = 0;

    int found = 0;

    if (!__isEmpty(fp))
    {
        while (getline(&line, &len, fp) != -1)
        {
            if (r == row)
            {
                int _dum_ = strlen(line);

                for (int j = 0; j < _dum_; j++)
                {
                    if (j == col)
                    {
                        found = 1;
                        fputs(text, nfp);
                    }
                    fputc(line[j], nfp);
                }

                if (_dum_ == col)
                {
                    found = 1;
                    fputs(text, nfp);
                }
            }
            else
            {
                fputs(line, nfp);
            }

            r++;
        }

        if (row == r && col == 0)
        {
            found = 1;
            fputs(text, nfp);
        }
    }
    else
    {
        if (row == 1 && col == 0)
        {
            found = 1;
            fputs(text, nfp);
        }
    }

    fclose(fp);
    fclose(nfp);

    fp = fopen(filename, "w");
    nfp = fopen(newPath, "r");

    __copy(nfp, fp);

    fclose(fp);
    fclose(nfp);

    remove(newPath);

    if (!found)
    {
        _writeToOutput("~!ERROR: Invalid position\n");
        return 0;
    }

    free(line);
    chdir(parentDir);
    return 1;
}

/// @brief Shows the file
/// @param __file_path file name
void cat(char *__file_path)
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
        return;
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
            return;
        }

        isFirst = 0;

        if (chdir(tok))
        {
            _writeToOutput("~!ERROR: Invalid directory\n");
            chdir(parentDir);
            return;
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
        return;
    }

    FILE *fp = fopen(filename, "r+");

    int r = 1;

    char *line = NULL;
    size_t len = 0;

    if (!__isEmpty(fp))
    {
        while (getline(&line, &len, fp) != -1)
        {
            _writeToOutput(line);
        }
    }

    _writeToOutput("\n");

    fclose(fp);
    free(line);
    chdir(parentDir);
}

/// @brief Removes a certain number of characters in the file
/// @param __file_path file name
/// @param row starting line (1 indexed)
/// @param col starting column (0 indexed)
/// @param count number of characters to be removed
/// @param forward if set to one, characters are removed one after the other
/// @return 1 if changed, 0 if not
int removeStr(char *__file_path, int row, int col, int count, int forward)
{
    char *path = malloc(512 * sizeof(char));
    strcpy(path, __file_path);

    int mul = forward ? 1 : -1;

    int i, foundDot = 0;
    for (i = strlen(path) - 1; path[i] != '/' && i > -1; i--)
    {
        foundDot |= (path[i] == '.');
    }

    if (i == -1 || !foundDot)
    {
        _writeToOutput("~!ERROR: Invalid file name\n");
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
            _writeToOutput("~!ERROR: File should be in root folder\n");
            chdir(parentDir);
            return 0;
        }

        isFirst = 0;

        if (chdir(tok))
        {
            _writeToOutput("~!ERROR: Invalid directory\n");
            chdir(parentDir);
            return 0;
        }

        path = NULL;
    }

    if (!__fileExists(filename))
    {
        _writeToOutput("~!ERROR: File  ~?");
        _writeToOutput(__file_path);
        _writeToOutput(" ~&does not exist\n");
        chdir(parentDir);
        return 0;
    }

    char newPath[512];

    int jp;
    for (jp = 0; filename[jp] != '.'; jp++)
    {
        newPath[jp] = filename[jp];
    }

    newPath[jp] = '\0';

    strcat(newPath, ".temp");

    FILE *fp = fopen(filename, "r");
    FILE *nfp = fopen(newPath, "a");

    int r = 1;

    char *line = NULL;
    size_t len = 0;

    int found = 0;

    /**/

    int lines, words, chars;
    __fileLen(filename, &chars, &lines, &words);
    int ix = __index(filename, row, col);
    int e_ix = ix + mul * (count);

    if (ix == -1)
    {
        _writeToOutput("~!ERROR: Invalid position\n");
        fclose(fp);
        fclose(nfp);
        free(line);
        chdir(parentDir);

        return 0;
    }

    if (e_ix < 0 || e_ix > chars)
    {
        _writeToOutput("~!ERROR: Invalid count\n");
        fclose(fp);
        fclose(nfp);
        free(line);
        chdir(parentDir);

        return 0;
    }

    int s_ix = (e_ix < ix ? e_ix : ix),
        f_ix = (e_ix > ix ? e_ix : ix);

    char c_c;

    for (int c_ix = 0; c_ix < chars; c_ix++)
    {
        c_c = fgetc(fp);
        if (c_ix < s_ix || c_ix > f_ix)
        {
            fputc(c_c, nfp);
        }
        else
        {
        }
    }

    /**/

    fclose(fp);
    fclose(nfp);

    fp = fopen(filename, "w");
    nfp = fopen(newPath, "r");

    __copy(nfp, fp);

    fclose(fp);
    fclose(nfp);

    remove(newPath);

    free(line);
    chdir(parentDir);

    return 1;
}

/// @brief Copies a certain number of characters to clipboard
/// @param __file_path file name
/// @param row starting line (1 indexed)
/// @param col starting column (0 indexed)
/// @param count number of characters to be copied
/// @param forward if set to one, characters are copied one after the other
/// @return 1 if changed, 0 if not
int copyStr(char *__file_path, int row, int col, int count, int forward)
{
    char *path = malloc(512 * sizeof(char));
    strcpy(path, __file_path);

    int mul = forward ? 1 : -1;

    int i, foundDot = 0;
    for (i = strlen(path) - 1; path[i] != '/' && i > -1; i--)
    {
        foundDot |= (path[i] == '.');
    }

    if (i == -1 || !foundDot)
    {
        _writeToOutput("~!ERROR: Invalid file name\n");
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
            _writeToOutput("~!ERROR: File should be in root folder\n");
            chdir(parentDir);
            return 0;
        }

        isFirst = 0;

        if (chdir(tok))
        {
            _writeToOutput("~!ERROR: Invalid directory\n");
            chdir(parentDir);
            return 0;
        }

        path = NULL;
    }

    if (!__fileExists(filename))
    {
        _writeToOutput("~!ERROR: File  ~?");
        _writeToOutput(__file_path);
        _writeToOutput(" ~&does not exist\n");
        chdir(parentDir);
        return 0;
    }

    char newPath[512];
    char clpPath[512];

    int jp;
    for (jp = 0; filename[jp] != '.'; jp++)
    {
        newPath[jp] = filename[jp];
        clpPath[jp] = filename[jp];
    }

    newPath[jp] = '\0';
    clpPath[jp] = '\0';

    strcat(newPath, ".temp");
    strcat(clpPath, ".clp");

    FILE *fp = fopen(filename, "r");
    FILE *nfp = fopen(newPath, "a");
    FILE *cfp = fopen(clpPath, "a");

    int r = 1;

    char *line = NULL;
    size_t len = 0;

    int found = 0;

    /**/

    int lines, words, chars;
    __fileLen(filename, &chars, &lines, &words);
    int ix = __index(filename, row, col);
    int e_ix = ix + mul * (count);

    if (ix == -1)
    {
        _writeToOutput("~!ERROR: Invalid position\n");
        fclose(fp);
        fclose(nfp);
        fclose(cfp);
        free(line);
        chdir(parentDir);

        return 0;
    }

    if (e_ix < 0 || e_ix >= chars)
    {
        _writeToOutput("~!ERROR: Invalid count\n");
        fclose(fp);
        fclose(nfp);
        fclose(cfp);
        free(line);
        chdir(parentDir);

        return 0;
    }

    int s_ix = (e_ix < ix ? e_ix : ix),
        f_ix = (e_ix > ix ? e_ix : ix) - 1;

    char c_c;

    for (int c_ix = 0; c_ix < chars; c_ix++)
    {
        c_c = fgetc(fp);
        if (!(c_ix < s_ix || c_ix > f_ix))
        {
            fputc(c_c, cfp);
        }
        fputc(c_c, nfp);
    }

    fclose(fp);
    fclose(nfp);

    fp = fopen(filename, "w");
    nfp = fopen(newPath, "r");

    __copy(nfp, fp);

    fclose(fp);
    fclose(nfp);
    fclose(cfp);

    char k[512];
    getcwd(k, 512);

    cfp = fopen(clpPath, "r");
    chdir(parentDir);
    CLIPBOARD = fopen(clipboardPath, "w");
    __copy(cfp, CLIPBOARD);
    fclose(CLIPBOARD);
    fclose(cfp);

    chdir(k);
    remove(newPath);
    remove(clpPath);

    free(line);
    chdir(parentDir);

    return 1;
}

/// @brief Cuts a certain number of characters to clipboard
/// @param __file_path file name
/// @param row starting line (1 indexed)
/// @param col starting column (0 indexed)
/// @param count number of characters to be cut
/// @param forward if set to one, characters are cut one after the other
/// @return 1 if changed, 0 if not
int cutStr(char *__file_path, int row, int col, int count, int forward)
{
    char *path = malloc(512 * sizeof(char));
    strcpy(path, __file_path);

    int mul = forward ? 1 : -1;

    int i, foundDot = 0;
    for (i = strlen(path) - 1; path[i] != '/' && i > -1; i--)
    {
        foundDot |= (path[i] == '.');
    }

    if (i == -1 || !foundDot)
    {
        _writeToOutput("~!ERROR: Invalid file name\n");
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
            _writeToOutput("~!ERROR: File should be in root folder\n");
            chdir(parentDir);
            return 0;
        }

        isFirst = 0;

        if (chdir(tok))
        {
            _writeToOutput("~!ERROR: Invalid directory\n");
            chdir(parentDir);
            return 0;
        }

        path = NULL;
    }

    if (!__fileExists(filename))
    {
        _writeToOutput("~!ERROR: File  ~?");
        _writeToOutput(__file_path);
        _writeToOutput(" ~&does not exist\n");
        chdir(parentDir);
        return 0;
    }

    char newPath[512];
    char clpPath[512];

    int jp;
    for (jp = 0; filename[jp] != '.'; jp++)
    {
        newPath[jp] = filename[jp];
        clpPath[jp] = filename[jp];
    }

    newPath[jp] = '\0';
    clpPath[jp] = '\0';

    strcat(newPath, ".temp");
    strcat(clpPath, ".clp");

    FILE *fp = fopen(filename, "r");
    FILE *nfp = fopen(newPath, "a");
    FILE *cfp = fopen(clpPath, "a");

    int r = 1;

    char *line = NULL;
    size_t len = 0;

    int found = 0;

    /**/

    int lines, words, chars;
    __fileLen(filename, &chars, &lines, &words);
    int ix = __index(filename, row, col);
    int e_ix = ix + mul * (count);

    if (ix == -1)
    {
        _writeToOutput("~!ERROR: Invalid position\n");
        fclose(fp);
        fclose(nfp);
        fclose(cfp);
        free(line);
        chdir(parentDir);

        return 0;
    }

    if (e_ix < 0 || e_ix >= chars)
    {
        _writeToOutput("~!ERROR: Invalid count\n");
        fclose(fp);
        fclose(nfp);
        fclose(cfp);
        free(line);
        chdir(parentDir);

        return 0;
    }

    int s_ix = (e_ix < ix ? e_ix : ix),
        f_ix = (e_ix > ix ? e_ix : ix) - 1;

    char c_c;

    for (int c_ix = 0; c_ix < chars; c_ix++)
    {
        c_c = fgetc(fp);
        if (!(c_ix < s_ix || c_ix > f_ix))
        {
            fputc(c_c, cfp);
        }
        else
        {
            fputc(c_c, nfp);
        }
    }

    fclose(fp);
    fclose(nfp);

    fp = fopen(filename, "w");
    nfp = fopen(newPath, "r");

    __copy(nfp, fp);

    fclose(fp);
    fclose(nfp);
    fclose(cfp);

    char k[512];
    getcwd(k, 512);

    cfp = fopen(clpPath, "r");
    chdir(parentDir);
    CLIPBOARD = fopen(clipboardPath, "w");
    __copy(cfp, CLIPBOARD);
    fclose(CLIPBOARD);
    fclose(cfp);

    chdir(k);
    remove(newPath);
    remove(clpPath);

    free(line);
    chdir(parentDir);

    return 1;
}

/// @brief Pastes clipboard into the given position
/// @param __file_path file name
/// @param row starting line (1 indexed)
/// @param col starting column (0 indexed)
/// @return 1 if changed, 0 if not
int pasteStr(char *__file_path, int row, int col)
{
    char *path = malloc(512 * sizeof(char));
    strcpy(path, __file_path);

    CLIPBOARD = fopen(clipboardPath, "rb");
    char *str = __toString(CLIPBOARD);

    int out = insertStr(path, str, row, col);
    fclose(CLIPBOARD);
    free(str);

    return out;
}

/// @brief Finds a pattern in the file (helper for grep)
/// @param __file_path file name
/// @param pat_ pattern to be matched
/// @return 1 if successful, 0 if not
int crazy_find(char *__file_path, char *pat_)
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
        _writeToOutput("~!ERROR: Invalid file name\n");
        chdir(parentDir);
        return 0;
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
            _writeToOutput("~!ERROR: File should be in root folder\n");
            chdir(parentDir);
            return 0;
        }

        isFirst = 0;

        if (chdir(tok))
        {
            _writeToOutput("~!ERROR: Invalid directory\n");
            chdir(parentDir);
            return 0;
        }

        path = NULL;
    }

    if (!__fileExists(filename))
    {
        _writeToOutput("~!ERROR: File  ~?");
        _writeToOutput(__file_path);
        _writeToOutput(" ~&does not exist\n");
        chdir(parentDir);
        return 0;
    }

    FILE *fp = fopen(filename, "r+");

    char *str = __toString(fp);

    int pats;
    short *pat = __toPat(pat_, &pats);
    if ((pats == 1 && pat[0] == -1) || (pats == 2 && pat[0] == -1 && pat[1] == -1))
    {
        _writeToOutput("~!ERROR: Invalid regex\n");
        return 0;
    }

    int cmatch[512];
    int matchno = 0;

    int _dum_ = (int)strlen(str);
    for (int ii = 0; ii < _dum_; ii++)
    {
        int last_ix = ii;
        int c_index = __find(str, pat, pats, ii, &last_ix);
        if (c_index != -1)
        {
            if (pat[0] != -1 && c_index > 0 && !(str[c_index - 1] == ' ' || str[c_index - 1] == '\n' || str[c_index - 1] == '\t'))
            {
                continue;
            }
            else if (pat[pats - 1] != -1 && last_ix < _dum_ && !(str[last_ix] == ' ' || str[last_ix] == '\n' || str[last_ix] == '\t'))
            {
                continue;
            }
            else
            {
                cmatch[matchno++] = c_index;
                ii = c_index;
            }
        }
    }

    fclose(fp);
    chdir(parentDir);
    return matchno ? 1 : 0;
}

/// @brief Finds a pattern in the file
/// @param __file_path file name
/// @param pat_ pattern to be matched
/// @param f_type type of find (at|byword|count|all) e.g. 0110 means count byword
/// @param at if f_type is 1XXX this parameter is at's argument
/// @return 1 if successful, 0 if not
int find(char *__file_path, char *pat_, int f_type, int at)
{
    char *path = malloc(512 * sizeof(char));
    strcpy(path, __file_path);

    // Errors
    if ((f_type & (ALL & AT)) || (f_type & (ALL & COUNT)) || (f_type & (AT & COUNT)) || ((f_type & AT) && at < 0))
    {
        _writeToOutput("~!ERROR: wrong input\n");
        return 0;
    }

    int i, foundDot = 0;
    for (i = (int)strlen(path) - 1; path[i] != '/' && i > -1; i--)
    {
        foundDot |= (path[i] == '.');
    }

    if (i == -1 || !foundDot)
    {
        _writeToOutput("~!ERROR: Invalid file name\n");
        chdir(parentDir);
        return 0;
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
            _writeToOutput("~!ERROR: File should be in root folder\n");
            chdir(parentDir);
            return 0;
        }

        isFirst = 0;

        if (chdir(tok))
        {
            _writeToOutput("~!ERROR: Invalid directory\n");
            chdir(parentDir);
            return 0;
        }

        path = NULL;
    }

    if (!__fileExists(filename))
    {
        _writeToOutput("~!ERROR: File  ~?");
        _writeToOutput(__file_path);
        _writeToOutput(" ~&does not exist\n");
        chdir(parentDir);
        return 0;
    }

    FILE *fp = fopen(filename, "r+");

    char *str = __toString(fp);

    int pats;
    short *pat = __toPat(pat_, &pats);
    if ((pats == 1 && pat[0] == -1) || (pats == 2 && pat[0] == -1 && pat[1] == -1))
    {
        _writeToOutput("~!ERROR: Invalid regex\n");
        return 0;
    }

    int cmatch[512];
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
                cmatch[matchno++] = c_index;
                ii = c_index;
            }
        }
    }

    if (!matchno)
    {
        _writeToOutput("No matches\n");
        fclose(fp);
        chdir(parentDir);
        return 0;
    }

    if (f_type & AT)
    {
        if (at >= matchno)
        {
            _writeToOutput("Not enough matches\n");
            fclose(fp);
            chdir(parentDir);
            return 0;
        }
    }

    if (f_type & COUNT)
    {
        _writeToOutput(__itoa(matchno));
        _writeToOutput("\n");
        fclose(fp);
        chdir(parentDir);
        return 1;
    }

    if (f_type & BYWORD)
    {
        __toWordArr(str, cmatch);
    }

    if (f_type & ALL)
    {
        for (int ii = 0; ii < matchno; ii++)
        {
            _writeToOutput(__itoa(cmatch[ii]));
            if (ii != matchno - 1)
            {
                _writeToOutput(", ");
            }
        }

        _writeToOutput("\n");
        fclose(fp);
        chdir(parentDir);
        return 1;
    }

    if (f_type & AT)
    {
        _writeToOutput(__itoa(cmatch[at]));
        _writeToOutput("\n");
        fclose(fp);
        chdir(parentDir);
        return 1;
    }

    _writeToOutput(__itoa(cmatch[0]));
    _writeToOutput("\n");

    fclose(fp);
    chdir(parentDir);
    return 1;
}

/// @brief Replaces string str with fill
/// @param __file_path file name
/// @param str pattern to be matched
/// @param fill string to be filled
/// @param at if set to n, replaces nth occurence, if set -1 replaces all
/// @return 1 if changed, 0 if not
int replace(char *__file_path, char *str, char *fill, int at)
{
    char *path = malloc(512 * sizeof(char));
    strcpy(path, __file_path);

    // Errors
    if (at < -1)
    {
        _writeToOutput("~!ERROR: wrong input\n");
        return 0;
    }

    int i, foundDot = 0;
    for (i = (int)strlen(path) - 1; path[i] != '/' && i > -1; i--)
    {
        foundDot |= (path[i] == '.');
    }

    if (i == -1 || !foundDot)
    {
        _writeToOutput("~!ERROR: Invalid file name\n");
        chdir(parentDir);
        return 0;
    }

    char *filename = malloc((unsigned int)((int)strlen(path) - 1 - i) * sizeof(char));

    int __bull__ = strlen(path);
    for (int j = i + 1; j < __bull__; j++)
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
            _writeToOutput("~!ERROR: File should be in root folder\n");
            chdir(parentDir);
            return 0;
        }

        isFirst = 0;

        if (chdir(tok))
        {
            _writeToOutput("~!ERROR: Invalid directory\n");
            chdir(parentDir);
            return 0;
        }

        path = NULL;
    }

    if (!__fileExists(filename))
    {
        _writeToOutput("~!ERROR: File  ~?");
        _writeToOutput(__file_path);
        _writeToOutput(" ~&does not exist\n");
        chdir(parentDir);
        return 0;
    }

    FILE *fp = fopen(filename, "r+");

    char *fstr = __toString(fp);

    int pats;
    short *pat = __toPat(str, &pats);
    if ((pats == 1 && pat[0] == -1) || (pats == 2 && pat[0] == -1 && pat[1] == -1))
    {
        _writeToOutput("~!ERROR: Invalid regex\n");
        fclose(fp);
        free(fp);
        chdir(parentDir);
        return 0;
    }

    int cmatch[512];
    int matchno = 0;

    int _dum_ = strlen(fstr);

    for (int ii = 0; ii < _dum_; ii++)
    {
        int last_ix = ii;
        int c_index = __find(fstr, pat, pats, ii, &last_ix);
        if (c_index != -1)
        {
            if (pat[0] != -1 && c_index > 0 && !(fstr[c_index - 1] == ' ' || fstr[c_index - 1] == '\n' || fstr[c_index - 1] == '\t'))
            {
                continue;
            }
            else if (pat[pats - 1] != -1 && last_ix < _dum_ && !(fstr[last_ix] == ' ' || fstr[last_ix] == '\n' || fstr[last_ix] == '\t'))
            {
                continue;
            }
            else
            {
                cmatch[matchno++] = c_index;
                ii = c_index;
            }
        }
    }

    fclose(fp);

    int purelen;
    short *ppat = __toPat(str, &purelen);

    if ((purelen == 1 && ppat[0] == -1) || (purelen == 2 && ppat[0] == -1 && ppat[1] == -1))
    {
        _writeToOutput("~!ERROR: Invalid regex\n");
        free(fp);
        chdir(parentDir);
        return 0;
    }

    purelen -= (ppat[0] == -1) + (ppat[purelen - 1] == -1);

    int strs = strlen(fstr);

    fp = fopen(filename, "w");

    if (at == -1)
    {
        int oci = 0;
        for (int o = 0; o < strs; o++)
        {
            if (oci != matchno && o == cmatch[oci])
            {
                fputs(fill, fp);
                oci++;
                o += purelen - 1;
            }
            else
            {
                fputc(fstr[o], fp);
            }
        }
    }
    else
    {
        for (int o = 0; o < strs; o++)
        {
            if (at < matchno && o == cmatch[at])
            {
                fputs(fill, fp);
                o += purelen - 1;
            }
            else
            {
                fputc(fstr[o], fp);
            }
        }

        if (at >= matchno)
        {
            _writeToOutput("Not enough matches\n");
            fclose(fp);
            free(fp);
            chdir(parentDir);
            return 0;
        }
    }

    fclose(fp);
    free(fp);
    chdir(parentDir);

    return 1;
}

/// @brief Find helper for grep
/// @param __file_path file
/// @param pat_ pattern
/// @return matches
char *__specialFind(char *__file_path, char *pat_)
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
        _writeToOutput("~!ERROR: Invalid file name\n");
        chdir(parentDir);
        return "-1";
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
            _writeToOutput("~!ERROR: File should be in root folder\n");
            chdir(parentDir);
            return "-1";
        }

        isFirst = 0;

        if (chdir(tok))
        {
            _writeToOutput("~!ERROR: Invalid directory\n");
            chdir(parentDir);
            return "-1";
        }

        path = NULL;
    }

    if (!__fileExists(filename))
    {
        _writeToOutput("~!ERROR: File  ~?");
        _writeToOutput(__file_path);
        _writeToOutput(" ~&does not exist\n");
        chdir(parentDir);
        return "-1";
    }

    FILE *fp = fopen(filename, "r+");

    char *out = calloc(1000, sizeof(out));

    char *str = __toString(fp);

    int pats;
    short *pat = __toPat(pat_, &pats);
    if ((pats == 1 && pat[0] == -1) || (pats == 2 && pat[0] == -1 && pat[1] == -1))
    {
        _writeToOutput("~!ERROR: Invalid regex\n");
        chdir(parentDir);
        return "-1";
    }

    int cmatch[512];
    int matchno = 0;

    int _dum_ = (int)strlen(str);
    for (int ii = 0; ii < _dum_; ii++)
    {
        int last_ix = ii;
        int c_index = __find(str, pat, pats, ii, &last_ix);
        if (c_index != -1)
        {
            if (pat[0] != -1 && c_index > 0 && !(str[c_index - 1] == ' ' || str[c_index - 1] == '\n' || str[c_index - 1] == '\t'))
            {
                continue;
            }
            else if (pat[pats - 1] != -1 && last_ix < _dum_ && !(str[last_ix] == ' ' || str[last_ix] == '\n' || str[last_ix] == '\t'))
            {
                continue;
            }
            else
            {
                cmatch[matchno++] = c_index;
                ii = c_index;
            }
        }
    }

    if (!matchno)
    {
        chdir(parentDir);
        return "-1";
    }

    int clinestart = 0;
    int printed = 0;
    int cint = 0;
    for (int ii = 0; ii < _dum_; ii++)
    {
        if (cmatch[cint] == ii && !printed)
        {
            int jj = clinestart;

            strcat(out, __file_path);
            strcat(out, ": ");

            while (str[jj] != '\n' && str[jj] != '\0')
            {
                strncat(out, &str[jj], 1);
                jj++;
            }

            strcat(out, "\n");

            cint++;
            printed = 1;
        }
        else if (cmatch[cint] == ii && printed)
        {
            cint++;
        }

        if (str[ii] == '\n')
        {
            clinestart = ii + 1;
            printed = 0;
        }
    }

    fclose(fp);
    chdir(parentDir);
    return out;
}

/// @brief Finds in given files
/// @param files file paths
/// @param s_files number of files
/// @param str pattern to be matched
/// @param option either c(only number of matches), l(only file names) or none
void grep(char **files, int s_files, char *str, char option)
{
    if (option == 'l')
    {
        for (int i = 0; i < s_files; i++)
        {
            int res = crazy_find(files[i], str);
            if (res)
            {
                _writeToOutput(files[i]);
                _writeToOutput("\n");
            }
        }
    }
    else if (option == 'c')
    {
        int o = 0;
        for (int i = 0; i < s_files; i++)
        {
            int res = crazy_find(files[i], str);
            if (res)
            {
                o++;
            }
        }
        _writeToOutput(__itoa(o));
        _writeToOutput("\n");
    }
    else
    {
        for (int i = 0; i < s_files; i++)
        {
            char *res = __specialFind(files[i], str);
            if (strcmp(res, "-1"))
            {
                _writeToOutput(res);
            }
        }
    }
}

/// @brief Undoes recent action (only once)
/// @param __file_path file name
void undo(char *__file_path)
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
        return;
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
            return;
        }

        isFirst = 0;

        if (chdir(tok))
        {
            _writeToOutput("~!ERROR: Invalid directory\n");
            chdir(parentDir);
            return;
        }

        path = NULL;
    }

    if (!__fileExists(filename))
    {
        _writeToOutput("~!ERROR: File  ~?");
        _writeToOutput(__file_path);
        _writeToOutput(" ~&does not exist\n");
        free(path);
        chdir(parentDir);
        return;
    }

    if (!__fileExists(__undoPath(filename)))
    {
        _writeToOutput("~!ERROR: No undo available\n");
        free(path);
        chdir(parentDir);
        return;
    }

    FILE *fp = fopen(filename, "r+");
    FILE *df = fopen("dummy.dum", "w+");
    FILE *fu = fopen(__undoPath(filename), "r+");

    __copy(fp, df);

    fseek(fp, 0, SEEK_SET);
    fseek(df, 0, SEEK_SET);

    fclose(fp);
    fp = fopen(filename, "w");

    __copy(fu, fp);

    fseek(fu, 0, SEEK_SET);
    fseek(fp, 0, SEEK_SET);

    fclose(fu);
    fu = fopen(__undoPath(filename), "w");

    __copy(df, fu);

    fseek(fu, 0, SEEK_SET);
    fseek(df, 0, SEEK_SET);

    fclose(df);
    fclose(fu);
    fclose(fp);

    remove("dummy.dum");
    chdir(parentDir);
}

/// @brief Indents the given file
/// @param __file_path file name
/// @return 1 if chanegd, 0 if not
int autoIndent(char *__file_path)
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
            _writeToOutput("~!ERROR: File should be in root folder\n");
            chdir(parentDir);
            return 0;
        }

        isFirst = 0;

        if (chdir(tok))
        {
            _writeToOutput("~!ERROR: Invalid directory\n");
            chdir(parentDir);
            return 0;
        }

        path = NULL;
    }

    if (!__fileExists(filename))
    {
        _writeToOutput("~!ERROR: File  ~?");
        _writeToOutput(__file_path);
        _writeToOutput(" ~&does not exist\n");
        chdir(parentDir);
        return 0;
    }

    char newPath[512];

    int jp;
    for (jp = 0; filename[jp] != '.'; jp++)
    {
        newPath[jp] = filename[jp];
    }

    newPath[jp] = '\0';

    strcat(newPath, ".temp");

    FILE *fp = fopen(filename, "r");
    FILE *nfp = fopen(newPath, "a");

    char *line = NULL;
    size_t len = 0;

    int wscnt = 0;

    if (!__isEmpty(fp))
    {
        while (getline(&line, &len, fp) != -1)
        {
            char *ins = malloc(1024 * sizeof(char));
            int ins_s = 0;
            int ignorespace = 0;

            int glen = strlen(line);

            for (int ii = 0; ii < glen; ii++)
            {
                if ((line[ii] == '\t' || line[ii] == ' ' || line[ii] == '\n') && ignorespace)
                {
                    continue;
                }

                ignorespace = 0;

                if (line[ii] == '{')
                {
                    while (ins_s != 0 && ins[ins_s - 1] == ' ' || ins[ins_s - 1] == '\t')
                    {
                        ins[--ins_s] = '\0';
                    }

                    for (int jj = 0; jj < wscnt; jj++)
                    {
                        fputc(' ', nfp);
                    }

                    if (ins_s && ins[ins_s - 1] != ' ')
                        ins[ins_s++] = ' ';
                    ins[ins_s++] = '{';
                    if (ins[ins_s - 1] != '\n')
                        ins[ins_s++] = '\n';
                    ins[ins_s] = '\0';

                    fputs(ins, nfp);
                    wscnt += 4;
                    ins_s = 0;
                    ignorespace = 1;
                    ins[0] = '\0';
                }
                else if (line[ii] == '}')
                {
                    if (ins_s)
                    {
                        for (int jj = 0; jj < wscnt; jj++)
                        {
                            fputc(' ', nfp);
                        }

                        if (ins[ins_s - 1] != '\n')
                            ins[ins_s++] = '\n';
                        ins[ins_s] = '\0';

                        fputs(ins, nfp);

                        ins_s = 0;
                        ins[0] = '\0';
                    }

                    wscnt = ((wscnt - 4) > 0 ? (wscnt - 4) : 0);

                    for (int jj = 0; jj < wscnt; jj++)
                    {
                        fputc(' ', nfp);
                    }

                    fputs("}\n", nfp);

                    ins_s = 0;
                    ignorespace = 1;
                    ins[0] = '\0';
                }
                else if (line[ii] == '\n' || line[ii] == '\0')
                {
                    for (int jj = 0; jj < wscnt; jj++)
                    {
                        if (ins_s)
                            fputc(' ', nfp);
                    }

                    if (ins[ins_s - 1] != '\n' && line[ii] != '\0')
                        ins[ins_s++] = '\n';

                    ins[ins_s] = '\0';

                    fputs(ins, nfp);

                    ins_s = 0;
                    ignorespace = 1;
                    ins[0] = '\0';
                }
                else
                {
                    ins[ins_s++] = line[ii];
                    ins[ins_s] = '\0';
                }
            }
        }
    }

    fclose(fp);
    fclose(nfp);

    fp = fopen(filename, "w");
    nfp = fopen(newPath, "r");

    __copy(nfp, fp);

    fclose(fp);
    fclose(nfp);

    remove(newPath);

    free(line);
    chdir(parentDir);

    return 1;
}

/// @brief Compares two strings
/// @param this first string
/// @param that second string
/// @return result
char *__cmpLine(char *this, char *that)
{
    int maxlenthis = 0, maxlenthat = 0, wordthis = 1, wordthat = 1, thiss = strlen(this), thats = strlen(that);

    for (int i = 0, last = 0; i < thiss; i++)
    {
        if (this[i] == ' ' || this[i] == '\t')
        {
            if (maxlenthis < i - last + 2)
            {
                maxlenthis = i - last + 2;
            }

            last = i;
            wordthis++;
        }
    }

    for (int i = 0, last = 0; i < thats; i++)
    {
        if (that[i] == ' ' || that[i] == '\t')
        {
            if (maxlenthat < i - last + 2)
            {
                maxlenthat = i - last + 2;
            }

            last = i;
            wordthat++;
        }
    }

    if (wordthat != wordthis || maxlenthis != maxlenthat)
    {
        char *out = calloc((strlen(this) + strlen(that) + 15), sizeof(char));

        strcat(out, this);
        if (out[strlen(out) - 1] != '\n')
        {
            strcat(out, "\n");
        }
        strcat(out, that);

        if (out[strlen(out) - 1] == '\n')
        {
            out[strlen(out) - 1] = '\0';
        }

        return out;
    }

    char wthis[wordthis][maxlenthis];
    char wthat[wordthat][maxlenthat];

    char *thiscpy = calloc(strlen(this), sizeof(char));
    strcpy(thiscpy, this);

    char *thatcpy = calloc(strlen(that), sizeof(char));
    strcpy(thatcpy, that);

    int cntthis = 0;
    char *thistok = strtok(thiscpy, " ");
    while (thistok != NULL)
    {
        strcpy(wthis[cntthis++], thistok);
        thistok = strtok(NULL, " ");
    }

    int cntthat = 0;
    char *thattok = strtok(thatcpy, " ");
    while (thattok != NULL)
    {
        strcpy(wthat[cntthat++], thattok);
        thattok = strtok(NULL, " ");
    }

    int diff = 0;
    char *outthis = calloc((strlen(this) + 10), sizeof(char));
    char *outthat = calloc((strlen(that) + 10), sizeof(char));
    char *outthisone = calloc((strlen(this) + 10), sizeof(char));
    char *outthatone = calloc((strlen(that) + 10), sizeof(char));

    int cnt = 0;
    while (cnt != wordthat && cnt != wordthis)
    {
        if (strcmp(wthis[cnt], wthat[cnt]))
        {
            if (diff == 0)
            {
                strcat(outthisone, ">>");
                strcat(outthisone, wthis[cnt]);
                strcat(outthisone, "<< ");

                strcat(outthatone, ">>");
                strcat(outthatone, wthat[cnt]);
                strcat(outthatone, "<< ");
            }
            else
            {
                strcat(outthisone, wthis[cnt]);
                strcat(outthisone, " ");

                strcat(outthatone, wthat[cnt]);
                strcat(outthatone, " ");
            }

            strcat(outthis, wthis[cnt]);
            strcat(outthis, " ");

            strcat(outthat, wthat[cnt]);
            strcat(outthat, " ");

            diff++;
        }
        else
        {
            strcat(outthis, wthis[cnt]);
            strcat(outthis, " ");

            strcat(outthat, wthat[cnt]);
            strcat(outthat, " ");

            strcat(outthisone, wthis[cnt]);
            strcat(outthisone, " ");

            strcat(outthatone, wthat[cnt]);
            strcat(outthatone, " ");
        }

        cnt++;
    }

    if (diff)
    {
        char *out = calloc((strlen(this) + strlen(that) + 15), sizeof(char));

        if (diff == 1)
        {
            strcat(out, outthisone);
            if (out[strlen(out) - 1] != '\n')
            {
                strcat(out, "\n");
            }
            strcat(out, outthatone);
        }
        else
        {
            strcat(out, outthis);
            if (out[strlen(out) - 1] != '\n')
            {
                strcat(out, "\n");
            }
            strcat(out, outthat);
        }

        if (out[strlen(out) - 1] == '\n')
        {
            out[strlen(out) - 1] = '\0';
        }

        return out;
    }
    else
    {
        return "-1";
    }
}

/// @brief Compares two files
/// @param this first file
/// @param that second file
/// @param this_path first path
/// @param that_path second path
void __cmpFile(FILE *this, FILE *that, char *this_path, char *that_path)
{
    char *out = calloc(100000, sizeof(char));

    int rthis = 1, rthat = 1;

    int _lthis = 0, _wthis, _cthis,
        _lthat = 0, _wthat, _cthat;

    // test this
    int dummy;
    chdir(parentDir);
    __originFileLen(this_path, &_cthis, &_lthis, &_wthis, &dummy);
    chdir(parentDir);
    __originFileLen(that_path, &_cthat, &_lthat, &_wthat, &dummy);

    _lthis++;
    _lthat++;

    char *linethis = NULL;
    char *linethat = NULL;
    size_t lenthis = 0, lenthat = 0;

    if (!__isEmpty(this) && !__isEmpty(that))
    {
        while ((getline(&linethat, &lenthat, that) != -1) && (getline(&linethis, &lenthis, this) != -1))
        {
            char *res = __cmpLine(linethis, linethat);
            if (strcmp(res, "-1"))
            {
                _writeToOutput("============ #");
                _writeToOutput(__itoa(rthis));
                _writeToOutput(" ============\n");
                _writeToOutput(res);
                _writeToOutput("\n");
            }

            rthis++;
            rthat++;
        }

        _lthis++;
        _lthat++;

        if (rthis == _lthis && rthat != _lthat)
        {
            _writeToOutput(">>>>>>>>>>>> #");
            _writeToOutput(__itoa(rthat));
            _writeToOutput(" - #");
            _writeToOutput(__itoa(_lthat - 1));
            _writeToOutput(" <<<<<<<<<<<<\n");

            _writeToOutput(linethat);

            while (getline(&linethat, &lenthat, that) != -1)
            {
                _writeToOutput(linethat);
            }
        }

        if (rthat == _lthat && rthis != _lthis)
        {
            _writeToOutput(">>>>>>>>>>>> #");
            _writeToOutput(__itoa(rthis));
            _writeToOutput(" - #");
            _writeToOutput(__itoa(_lthis - 1));
            _writeToOutput(" <<<<<<<<<<<<\n");

            _writeToOutput(linethis);
            while (getline(&linethis, &lenthis, this) != -1)
            {
                _writeToOutput(linethis);
            }
        }
    }
}

/// @brief Compares the files
/// @param this_path first file name
/// @param that_path second file name
void compareFiles(char *this_path, char *that_path)
{
    char *path = malloc(512 * sizeof(char));
    strcpy(path, this_path);

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
        return;
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
            return;
        }

        isFirst = 0;

        if (chdir(tok))
        {
            _writeToOutput("~!ERROR: Invalid directory\n");
            chdir(parentDir);
            return;
        }

        path = NULL;
    }

    if (!__fileExists(filename))
    {
        _writeToOutput("~!ERROR: File  ~?");
        _writeToOutput(this_path);
        _writeToOutput(" ~&does not exist\n");
        free(path);
        chdir(parentDir);
        return;
    }

    FILE *thisfile = fopen(filename, "r+");
    chdir(parentDir);

    char *ppath = malloc(512 * sizeof(char));
    strcpy(ppath, that_path);

    int pi, pfoundDot = 0;
    for (pi = strlen(ppath) - 1; ppath[pi] != '/' && pi > -1; pi--)
    {
        pfoundDot |= (ppath[pi] == '.');
    }

    if (pi == -1 || !pfoundDot)
    {
        _writeToOutput("~!ERROR: Invalid file name\n");
        free(path);
        chdir(parentDir);
        return;
    }

    char *pfilename = malloc((strlen(ppath) - 1 - pi) * sizeof(char));

    int _pdum_ = strlen(ppath);
    for (int pj = pi + 1; pj < _pdum_; pj++)
    {
        pfilename[pj - pi - 1] = ppath[pj];
    }

    pfilename[strlen(ppath) - 1 - pi] = '\0';
    ppath[pi] = '\0';

    char *ptok;
    int pisFirst = 1;

    while ((ptok = strtok(ppath, "/")) != NULL)
    {
        if (pisFirst && strcmp(ptok, "root"))
        {
            _writeToOutput("~!ERROR: File should be in root folder\n");
            chdir(parentDir);
            return;
        }

        pisFirst = 0;

        if (chdir(ptok))
        {
            _writeToOutput("~!ERROR: Invalid directory\n");
            chdir(parentDir);
            return;
        }

        ppath = NULL;
    }

    if (!__fileExists(pfilename))
    {
        _writeToOutput("~!ERROR: File  ~?");
        _writeToOutput(that_path);
        _writeToOutput(" ~&does not exist\n");
        free(path);
        chdir(parentDir);
        return;
    }

    FILE *thatfile = fopen(pfilename, "r+");

    __cmpFile(thisfile, thatfile, this_path, that_path);

    fclose(thatfile);
    fclose(thisfile);

    chdir(parentDir);
}

/// @brief Number of subdirectories in cwd
/// @return
int __cntsub()
{
    int out = 0;

    struct dirent *de;
    DIR *dr = opendir(".");

    if (dr == NULL)
    {
        return 0;
    }

    while ((de = readdir(dr)) != NULL)
    {

        if ((de->d_name)[0] == '.')
            continue;

        out++;
    }

    return out;
}

/// @brief Helper function for tree
/// @param depth current depth of tree
/// @param precspace preceding space
void __tree(int depth, int precspace, int *printPer)
{
    if (depth == 0)
        return;

    struct dirent *de;
    DIR *dr = opendir(".");

    if (dr == NULL)
    {
        return;
    }

    int subcnt = __cntsub();
    int copysub = subcnt;

    while ((de = readdir(dr)) != NULL)
    {

        if ((de->d_name)[0] == '.')
            continue;

        for (int k = 0; k < precspace; k++)
        {
            _writeToOutput(printPer[k] ? "~%" : " ");
        }

        _writeToOutput(subcnt-- == 1 ? "~^" : "~`");
        _writeToOutput(de->d_name);
        _writeToOutput("\n");

        if (!__isFile(de->d_name))
        {
            chdir(de->d_name);

            printPer[precspace] = subcnt;
            __tree(depth - 1, precspace + 2, printPer);

            chdir("..");
        }
    }
}

/// @brief Shows directory tree
/// @param depth depth of tree, -1 means show all
void tree(int depth)
{
    chdir("root");

    _writeToOutput("root\n");

    int *printPer = calloc(1024, sizeof(int));
    printPer[0] = __cntsub() != 1;

    __tree(depth, 0, printPer);

    chdir(parentDir);
}

/// @brief =D
/// @param inp input command
/// @param str input string
void arman(char *inp, char *str)
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
            return;
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

    enum COMMAND com = INVALID;

    char *arg[argc];
    for (int i = 0; i < argc; i++)
    {
        arg[i] = _getarg(i);
    }

    for (int i = 0; i < 14; i++)
    {
        if (!strcmp(arg[0], COMSTR[i]))
        {
            com = ixtocom[i];
            break;
        }
    }

    int lineno, startpos, forward, cnt, byword, at, count, all, atnum, type;

    switch (com)
    {
    case INSERTSTR:

        if (argc != 5 || strcmp(arg[1], "--file") || strcmp(arg[3], "--pos"))
        {
            _writeToOutput("~!ERROR: Invalid command\n");
            return;
        }

        if (sscanf(arg[4], "%d:%d", &lineno, &startpos) != 2)
        {
            _writeToOutput("~!ERROR: Invalid command\n");
            return;
        }

        _makeACopy(arg[2]);
        _pasteCopyInto(arg[2], insertStr(arg[2], str, lineno, startpos));

        break;

    case FIND:

        if (argc >= 3) /*normal*/
        {
            if (strcmp(arg[1], "--file"))
            {
                _writeToOutput("~!ERROR: Invalid command\n");
                return;
            }

            byword = 0, at = 0, count = 0, all = 0, atnum = -1;

            for (int i = 3; i < argc; i++)
            {
                if (!strcmp(arg[i], "-byword"))
                {
                    byword++;
                }
                else if (!strcmp(arg[i], "-count"))
                {
                    count++;
                }
                else if (!strcmp(arg[i], "-all"))
                {
                    all++;
                }
                else if (!strcmp(arg[i], "-at"))
                {
                    at++;
                }
                else if (_isnum(arg[i]) && atnum != -1)
                {
                    atnum = _tonum(arg[i]);
                }
                else
                {
                    _writeToOutput("~!ERROR: Invalid command\n");
                    return;
                }
            }

            if (byword > 1 || at > 1 || count > 1 || all > 1 || (at == 1 && atnum == -1))
            {
                _writeToOutput("~!ERROR: Invalid command\n");
                return;
            }

            type = (byword ? BYWORD : 0) +
                   (count ? COUNT : 0) +
                   (all ? ALL : 0) +
                   (at ? AT : 0);

            _makeACopy(arg[2]);
            _pasteCopyInto(arg[2], find(arg[2], str, type, atnum));
        }
        else
        {
            _writeToOutput("~!ERROR: Invalid command\n");
            return;
        }

        break;

    case REPLACE:

        if (argc >= 5) /*normal*/
        {
            if ((strcmp(arg[1], "--str1") && strcmp(arg[1], "--str2")) || strcmp(arg[3], "--file"))
            {
                _writeToOutput("~!ERROR: Invalid command\n");
                return;
            }

            if (argc == 5)
            {
                _makeACopy(arg[4]);
                if (arg[1][5] == '1')
                {
                    _pasteCopyInto(arg[4], replace(arg[4], arg[2], str, -2));
                }
                else
                {
                    _pasteCopyInto(arg[4], replace(arg[4], str, arg[2], -2));
                }
            }
            else if (argc == 6)
            {
                if (!strcmp(arg[5], "-all"))
                {
                    if (arg[1][5] == '1')
                    {
                        _pasteCopyInto(arg[4], replace(arg[4], arg[2], str, -2));
                    }
                    else
                    {
                        _pasteCopyInto(arg[4], replace(arg[4], str, arg[2], -2));
                    }
                }
                else
                {
                    _writeToOutput("~!ERROR: Invalid command\n");
                    return;
                }
            }
            else if (argc == 7)
            {
                if (!strcmp(arg[5], "-at") && _isnum(arg[6]))
                {
                    if (arg[1][5] == '1')
                    {
                        _pasteCopyInto(arg[4], replace(arg[4], arg[2], str, -2));
                    }
                    else
                    {
                        _pasteCopyInto(arg[4], replace(arg[4], str, arg[2], -2));
                    }
                }
                else
                {
                    _writeToOutput("~!ERROR: Invalid command\n");
                    return;
                }
            }
            else
            {
                _writeToOutput("~!ERROR: Invalid command\n");
                return;
            }
        }
        else
        {
            _writeToOutput("~!ERROR: Invalid command\n");
            return;
        }

        break;

    case GREP:

        if (argc >= 2)
        {
            if (argc >= 3 && (!strcmp(arg[1], "-c") || !strcmp(arg[1], "-l")))
            {
                if (strcmp(arg[2], "--files"))
                {
                    _writeToOutput("~!ERROR: Invalid command\n");
                    return;
                }
                else
                {
                    char **strings = malloc((argc - 3) * sizeof(char *));
                    for (int i = 3; i < argc; i++)
                    {
                        strings[i - 3] = arg[i];
                    }

                    grep(strings, argc - 3, str, arg[1][1]);
                }
            }
            else
            {
                if (strcmp(arg[1], "--files"))
                {
                    _writeToOutput("~!ERROR: Invalid command\n");
                    return;
                }
                else
                {
                    char **strings = malloc((argc - 2) * sizeof(char *));
                    for (int i = 2; i < argc; i++)
                    {
                        strings[i - 2] = arg[i];
                    }

                    grep(strings, argc - 2, str, 'n');
                }
            }
        }
        else
        {
            _writeToOutput("~!ERROR: Invalid command\n");
            return;
        }

        break;

    case INVALID:

        _writeToOutput("~!ERROR: Invalid command\n");
        return;

        break;

    default:
        _writeToOutput("~!ERROR: Invalid command\n");
        return;

        break;
    }
}

/// @brief Handles input from user (doesn't handle arman)
/// @param inp input command
void handler(char *inp)
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
            return;
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

    enum COMMAND com = INVALID;

    char *arg[argc];
    for (int i = 0; i < argc; i++)
    {
        arg[i] = _getarg(i);
    }

    for (int i = 0; i < 14; i++)
    {
        if (!strcmp(arg[0], COMSTR[i]))
        {
            com = ixtocom[i];
            break;
        }
    }

    int lineno, startpos, forward, cnt, byword, at, count, all, atnum, type;

    switch (com)
    {
    case CREATEFILE:

        if (argc != 3 || strcmp(arg[1], "--file"))
        {
            _writeToOutput("~!ERROR: Invalid command\n");
            return;
        }

        createFile(arg[2]);

        break;

    case INSERTSTR:

        if (argc != 7 || strcmp(arg[1], "--file") || strcmp(arg[3], "--str") || strcmp(arg[5], "--pos"))
        {
            _writeToOutput("~!ERROR: Invalid command\n");
            return;
        }

        if (sscanf(arg[6], "%d:%d", &lineno, &startpos) != 2)
        {
            _writeToOutput("~!ERROR: Invalid command\n");
            return;
        }

        _makeACopy(arg[2]);
        _pasteCopyInto(arg[2], insertStr(arg[2], arg[4], lineno, startpos));

        break;

    case CAT:

        if (argc != 3 || strcmp(arg[1], "--file"))
        {
            _writeToOutput("~!ERROR: Invalid command\n");
            return;
        }

        cat(arg[2]);

        break;

    case REMOVESTR:

        if (argc != 8 || strcmp(arg[1], "--file") || strcmp(arg[3], "--pos") || strcmp(arg[5], "-size"))
        {
            _writeToOutput("~!ERROR: Invalid command\n");
            return;
        }

        if (sscanf(arg[4], "%d:%d", &lineno, &startpos) != 2)
        {
            _writeToOutput("~!ERROR: Invalid command\n");
            return;
        }

        if (sscanf(arg[6], "%d", &cnt) != 1)
        {
            _writeToOutput("~!ERROR: Invalid command\n");
            return;
        }

        if (!strcmp(arg[7], "-b"))
        {
            forward = 0;
        }
        else if (!strcmp(arg[7], "-f"))
        {
            forward = 1;
        }
        else
        {
            _writeToOutput("~!ERROR: Invalid command\n");
            return;
        }

        _makeACopy(arg[2]);
        _pasteCopyInto(arg[2], removeStr(arg[2], lineno, startpos, cnt, forward));

        break;

    case COPYSTR:

        if (argc != 8 || strcmp(arg[1], "--file") || strcmp(arg[3], "--pos") || strcmp(arg[5], "-size"))
        {
            _writeToOutput("~!ERROR: Invalid command\n");
            return;
        }

        if (sscanf(arg[4], "%d:%d", &lineno, &startpos) != 2)
        {
            _writeToOutput("~!ERROR: Invalid command\n");
            return;
        }

        if (sscanf(arg[6], "%d", &cnt) != 1)
        {
            _writeToOutput("~!ERROR: Invalid command\n");
            return;
        }

        if (!strcmp(arg[7], "-b"))
        {
            forward = 0;
        }
        else if (!strcmp(arg[7], "-f"))
        {
            forward = 1;
        }
        else
        {
            _writeToOutput("~!ERROR: Invalid command\n");
            return;
        }

        copyStr(arg[2], lineno, startpos, cnt, forward);

        break;

    case CUTSTR:

        if (argc != 8 || strcmp(arg[1], "--file") || strcmp(arg[3], "--pos") || strcmp(arg[5], "-size"))
        {
            _writeToOutput("~!ERROR: Invalid command\n");
            return;
        }

        if (sscanf(arg[4], "%d:%d", &lineno, &startpos) != 2)
        {
            _writeToOutput("~!ERROR: Invalid command\n");
            return;
        }

        if (sscanf(arg[6], "%d", &cnt) != 1)
        {
            _writeToOutput("~!ERROR: Invalid command\n");
            return;
        }

        if (!strcmp(arg[7], "-b"))
        {
            forward = 0;
        }
        else if (!strcmp(arg[7], "-f"))
        {
            forward = 1;
        }
        else
        {
            _writeToOutput("~!ERROR: Invalid command\n");
            return;
        }

        _makeACopy(arg[2]);
        _pasteCopyInto(arg[2], cutStr(arg[2], lineno, startpos, cnt, forward));

        break;

    case PASTESTR:

        if (argc != 5 || strcmp(arg[1], "--file") || strcmp(arg[3], "--pos"))
        {
            _writeToOutput("~!ERROR: Invalid command\n");
            return;
        }

        if (sscanf(arg[4], "%d:%d", &lineno, &startpos) != 2)
        {
            _writeToOutput("~!ERROR: Invalid command\n");
            return;
        }

        _makeACopy(arg[2]);
        _pasteCopyInto(arg[2], pasteStr(arg[2], lineno, startpos));

        break;

    case FIND:

        if (argc >= 5) /*normal*/
        {
            if (strcmp(arg[1], "--str") || strcmp(arg[3], "--file"))
            {
                _writeToOutput("~!ERROR: Invalid command\n");
                return;
            }

            byword = 0, at = 0, count = 0, all = 0, atnum = -1;

            for (int i = 5; i < argc; i++)
            {
                if (!strcmp(arg[i], "-byword"))
                {
                    byword++;
                }
                else if (!strcmp(arg[i], "-count"))
                {
                    count++;
                }
                else if (!strcmp(arg[i], "-all"))
                {
                    all++;
                }
                else if (!strcmp(arg[i], "-at"))
                {
                    at++;
                }
                else if (_isnum(arg[i]) && atnum != -1)
                {
                    atnum = _tonum(arg[i]);
                }
                else
                {
                    _writeToOutput("~!ERROR: Invalid command\n");
                    return;
                }
            }

            if (byword > 1 || at > 1 || count > 1 || all > 1 || (at == 1 && atnum == -1))
            {
                _writeToOutput("~!ERROR: Invalid command\n");
                return;
            }

            type = (byword ? BYWORD : 0) +
                   (count ? COUNT : 0) +
                   (all ? ALL : 0) +
                   (at ? AT : 0);

            _makeACopy(arg[4]);
            _pasteCopyInto(arg[4], find(arg[4], arg[2], type, atnum));
        }
        else
        {
            _writeToOutput("~!ERROR: Invalid command\n");
            return;
        }

        break;

    case REPLACE: /*check this again*/

        if (argc >= 7) /*normal*/
        {
            if (strcmp(arg[1], "--str1") || strcmp(arg[3], "--str2") || strcmp(arg[5], "--file"))
            {
                _writeToOutput("~!ERROR: Invalid command\n");
                return;
            }

            if (argc == 7)
            {
                _makeACopy(arg[6]);
                _pasteCopyInto(arg[6], replace(arg[6], arg[2], arg[4], -2));
            }
            else if (argc == 8)
            {
                if (!strcmp(arg[7], "-all"))
                {
                    _makeACopy(arg[6]);
                    _pasteCopyInto(arg[6], replace(arg[6], arg[2], arg[4], -1));
                }
                else
                {
                    _writeToOutput("~!ERROR: Invalid command\n");
                    return;
                }
            }
            else if (argc == 9)
            {
                if (!strcmp(arg[7], "-at") && _isnum(arg[8]))
                {
                    _makeACopy(arg[6]);
                    _pasteCopyInto(arg[6], replace(arg[6], arg[2], arg[4], _tonum(arg[8])));
                }
                else
                {
                    _writeToOutput("~!ERROR: Invalid command\n");
                    return;
                }
            }
            else
            {
                _writeToOutput("~!ERROR: Invalid command\n");
                return;
            }
        }
        else
        {
            _writeToOutput("~!ERROR: Invalid command\n");
            return;
        }

        break;

    case GREP:

        if (argc >= 4)
        {
            if (!strcmp(arg[1], "--str"))
            {
                if (strcmp(arg[3], "--files"))
                {
                    _writeToOutput("~!ERROR: Invalid command\n");
                    return;
                }
                else
                {
                    char **strings = malloc((argc - 4) * sizeof(char *));
                    for (int i = 4; i < argc; i++)
                    {
                        strings[i - 4] = arg[i];
                    }

                    grep(strings, argc - 4, arg[2], 'n');
                }
            }
            else if (argc >= 5 && (!strcmp(arg[1], "-c") || !strcmp(arg[1], "-l")))
            {
                if (strcmp(arg[4], "--files"))
                {
                    _writeToOutput("~!ERROR: Invalid command\n");
                    return;
                }
                else
                {
                    char **strings = malloc((argc - 5) * sizeof(char *));
                    for (int i = 5; i < argc; i++)
                    {
                        strings[i - 5] = arg[i];
                    }

                    grep(strings, argc - 5, arg[3], arg[1][1]);
                }
            }
            else
            {
                _writeToOutput("~!ERROR: Invalid command\n");
                return;
            }
        }
        else
        {
            _writeToOutput("~!ERROR: Invalid command\n");
            return;
        }

        break;

    case UNDO:

        if (argc != 3 || strcmp(arg[1], "--file"))
        {
            _writeToOutput("~!ERROR: Invalid command\n");
            return;
        }

        undo(arg[2]);

        break;

    case AUTOINDENT:

        if (argc != 2)
        {
            _writeToOutput("~!ERROR: Invalid command\n");
            return;
        }

        _makeACopy(arg[1]);
        _pasteCopyInto(arg[1], autoIndent(arg[1]));

        break;

    case COMPARE:

        if (argc != 3)
        {
            _writeToOutput("~!ERROR: Invalid command\n");
            return;
        }

        compareFiles(arg[1], arg[2]);

        break;

    case TREE:

        if (argc != 2)
        {
            _writeToOutput("~!ERROR: Invalid command\n");
            return;
        }

        tree(strcmp(arg[1], "-1") ? _tonum(arg[1]) : -1);

        break;

    case INVALID:

        _writeToOutput("~!ERROR: Invalid command\n");
        return;

        break;

    default:
        break;
    }
}

void Handler(char *inp)
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
    _showOutput();
    _clearOutput();
}

int main()
{
    init();

    char s[32768] = "find --str \"*a quote\" --file \"/root/this is a test filder/hi bruh.txt\"";

    __boldpurple__();
    printf(">>> ");
    __reset__();

    gets(s);

    while (strcmp(s, "exit"))
    {
        Handler(s);

        __boldpurple__();
        printf(">>> ");
        __reset__();

        gets(s);
    }

    finish();

    return 0;
}