#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <math.h>
#include <dirent.h>
#include <ctype.h>

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
char clipboardPathCat[512];
FILE *CLIPBOARD;

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

/// @brief Undo-path of a file
/// @param filename file
/// @return undo-path
char *__undoPath(char *filename)
{
    char *out = malloc((strlen(filename) + 5) * sizeof(char));
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
        printf("ERROR: Invalid file name.");
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
            printf("ERROR: File should be in root folder.\n");
            chdir(parentDir);
            return;
        }

        isFirst = 0;

        if (chdir(tok))
        {
            printf("Invalid directory.");
            chdir(parentDir);
            return;
        }

        path = NULL;
    }

    if (!__fileExists(filename))
    {
        printf("ERROR: File %s does not exist.", path);
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

    chdir(parentDir);
}

/// @brief Ends the program
void finish()
{
    // remove(clipboardPath);
}

/// @brief Gets number of lines, words and characters in the file
/// @param __file_path file name
/// @param characters number of characters
/// @param lines number of lines
/// @param words number of words
void __originFileLen(char *__file_path, int *characters, int *lines, int *words)
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
        printf("ERROR: Invalid file name.");
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
            printf("ERROR: File should be in root folder.\n");
            chdir(parentDir);
            return;
        }

        isFirst = 0;

        if (chdir(tok))
        {
            printf("Invalid directory.");
            chdir(parentDir);
            return;
        }

        path = NULL;
    }

    if (!__fileExists(filename))
    {
        printf("ERROR: File %s does not exist.", path);
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
            _c = 0;
            _r++;
        }
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

    return found ? _char : -1;
}

/// @brief Converts file to string
/// @param f file
/// @return String
char *__toString(FILE *f)
{
    char *buffer = NULL;
    long length;

    if (f)
    {
        fseek(f, 0, SEEK_END);
        length = ftell(f);
        fseek(f, 0, SEEK_SET);
        buffer = malloc(length);
        if (buffer)
        {
            fread(buffer, 1, length, f);
        }
    }

    return buffer;
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
    short *out = malloc(n * sizeof(short));
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

    return out;
}

/// @brief Matches two strings
/// @param str main string
/// @param pat pattern
/// @param pats size of pattern
/// @param s_i starting index
/// @return index of match, -1 if not found
int __find(char *str, short *pat, int pats, int s_i)
{
    int strs = strlen(str);

    for (int i = s_i; i < strs; i++)
    {
        int j = (pat[0] == -1);
        int ip = i;

        while (j < (pats - (pat[pats - 1] == -1)) && ip < strs)
        {
            if (__matchc(pat[j], str[ip]))
            {
                j++;
                ip++;
            }
            else
            {
                break;
            }
        }

        if (j == (pats - (pat[pats - 1] == -1)))
        {
            if (pat[0] == -1)
            {
                if (i == 0 || !(__matchc(pat[0], str[i - 1])))
                {
                    continue;
                }
            }

            if (pat[strs - 1] == -1)
            {
                if (!__matchc(pat[j], str[ip]))
                {
                    continue;
                }
            }

            return i;
        }
    }

    return -1;
}

/// @brief Converts array of characters to array of words
/// @param str string
/// @param arrc array of characters
void __toWordArr(char *str, int *arrc)
{
    int wcnt = 0, ccnt = 0, prevs = -1;
    int _dum_ = strlen(str);
    for (int i = 0; i < _dum_; i++)
    {
        if (arrc[ccnt] == i)
        {
            arrc[ccnt++] = wcnt;
        }

        if (str[i] == ' ' || str[i] == '\n' || str[i] == EOF || str[i] == '\0')
        {
            wcnt++;
        }
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
        printf("ERROR: Invalid file name.");
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
            printf("ERROR: File should be created in root folder.\n");
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
        printf("ERROR: File already exists.");
        chdir(parentDir);
        return;
    }

    FILE *fp = fopen(filename, "w");

    if (fp)
    {
        printf("File created successfully.");
        fclose(fp);
    }
    else
    {
        printf("ERROR: Could not create file.");
    }

    chdir(parentDir);
}

/// @brief Inserts text in a particular position
/// @param __file_path file name
/// @param text text to be inserted
/// @param row starting line (1 indexed)
/// @param col starting column (0 indexed)
void insertStr(char *__file_path, char *text, int row, int col)
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
        printf("ERROR: Invalid file name.");
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
            printf("ERROR: File should be in root folder.\n");
            chdir(parentDir);
            return;
        }

        isFirst = 0;

        if (chdir(tok))
        {
            printf("ERROR: Invalid directory.");
            chdir(parentDir);
            return;
        }

        path = NULL;
    }

    if (!__fileExists(filename))
    {
        printf("ERROR: File %s does not exist.", path);
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
            }
            else
            {
                fputs(line, nfp);
            }

            r++;
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
        printf("ERROR: Invalid position.\n");
    }

    free(line);
    chdir(parentDir);
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
        printf("ERROR: Invalid file name.");
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
            printf("ERROR: File should be in root folder.\n");
            chdir(parentDir);
            return;
        }

        isFirst = 0;

        if (chdir(tok))
        {
            printf("Invalid directory.");
            chdir(parentDir);
            return;
        }

        path = NULL;
    }

    if (!__fileExists(filename))
    {
        printf("ERROR: File %s does not exist.", path);
        chdir(parentDir);
        return;
    }

    FILE *fp = fopen(filename, "r+");

    int r = 1;

    char *line = NULL;
    size_t len = 0;

    printf("     ============|%s|============\n\n", filename);

    if (!__isEmpty(fp))
    {
        while (getline(&line, &len, fp) != -1)
        {
            printf("%4d| %s", r++, line);
        }
    }
    printf("\n     --------------------------------------\n");

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
void removeStr(char *__file_path, int row, int col, int count, int forward)
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
        printf("ERROR: Invalid file name.");
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
            printf("ERROR: File should be in root folder.\n");
            chdir(parentDir);
            return;
        }

        isFirst = 0;

        if (chdir(tok))
        {
            printf("ERROR: Invalid directory.");
            chdir(parentDir);
            return;
        }

        path = NULL;
    }

    if (!__fileExists(filename))
    {
        printf("ERROR: File %s does not exist.", path);
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
    int e_ix = ix + mul * (count - 1);

    if (ix == -1)
    {
        printf("ERROR: Invalid position.");
        fclose(fp);
        fclose(nfp);
        free(line);
        chdir(parentDir);

        return;
    }

    if (e_ix < 0 || e_ix >= chars)
    {
        printf("ERROR: Invalid count.");
        fclose(fp);
        fclose(nfp);
        free(line);
        chdir(parentDir);

        return;
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
}

/// @brief Copies a certain number of characters to clipboard
/// @param __file_path file name
/// @param row starting line (1 indexed)
/// @param col starting column (0 indexed)
/// @param count number of characters to be copied
/// @param forward if set to one, characters are copied one after the other
void copyStr(char *__file_path, int row, int col, int count, int forward)
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
        printf("ERROR: Invalid file name.");
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
            printf("ERROR: File should be in root folder.\n");
            chdir(parentDir);
            return;
        }

        isFirst = 0;

        if (chdir(tok))
        {
            printf("ERROR: Invalid directory.");
            chdir(parentDir);
            return;
        }

        path = NULL;
    }

    if (!__fileExists(filename))
    {
        printf("ERROR: File %s does not exist.", path);
        chdir(parentDir);
        return;
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
    int e_ix = ix + mul * (count - 1);

    if (ix == -1)
    {
        printf("ERROR: Invalid position.");
        fclose(fp);
        fclose(nfp);
        fclose(cfp);
        free(line);
        chdir(parentDir);

        return;
    }

    if (e_ix < 0 || e_ix >= chars)
    {
        printf("ERROR: Invalid count.");
        fclose(fp);
        fclose(nfp);
        fclose(cfp);
        free(line);
        chdir(parentDir);

        return;
    }

    int s_ix = (e_ix < ix ? e_ix : ix),
        f_ix = (e_ix > ix ? e_ix : ix);

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

    /**/

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
}

/// @brief Cuts a certain number of characters to clipboard
/// @param __file_path file name
/// @param row starting line (1 indexed)
/// @param col starting column (0 indexed)
/// @param count number of characters to be cut
/// @param forward if set to one, characters are cut one after the other
void cutStr(char *__file_path, int row, int col, int count, int forward)
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
        printf("ERROR: Invalid file name.");
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
            printf("ERROR: File should be in root folder.\n");
            chdir(parentDir);
            return;
        }

        isFirst = 0;

        if (chdir(tok))
        {
            printf("ERROR: Invalid directory.");
            chdir(parentDir);
            return;
        }

        path = NULL;
    }

    if (!__fileExists(filename))
    {
        printf("ERROR: File %s does not exist.", path);
        chdir(parentDir);
        return;
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
    int e_ix = ix + mul * (count - 1);

    if (ix == -1)
    {
        printf("ERROR: Invalid position.");
        fclose(fp);
        fclose(nfp);
        fclose(cfp);
        free(line);
        chdir(parentDir);

        return;
    }

    if (e_ix < 0 || e_ix >= chars)
    {
        printf("ERROR: Invalid count.");
        fclose(fp);
        fclose(nfp);
        fclose(cfp);
        free(line);
        chdir(parentDir);

        return;
    }

    int s_ix = (e_ix < ix ? e_ix : ix),
        f_ix = (e_ix > ix ? e_ix : ix);

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

    /**/

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
}

/// @brief Pastes clipboard into the given position
/// @param __file_path file name
/// @param row starting line (1 indexed)
/// @param col starting column (0 indexed)
void pasteStr(char *__file_path, int row, int col)
{
    char *path = malloc(512 * sizeof(char));
    strcpy(path, __file_path);

    CLIPBOARD = fopen(clipboardPath, "rb");
    char *str = __toString(CLIPBOARD);

    insertStr(path, str, row, col);
    fclose(CLIPBOARD);
    free(str);
}

/// @brief Finds a pattern in the file
/// @param __file_path file name
/// @param pat_ pattern to be matched
/// @param f_type type of find (at|byword|count|all) e.g. 0110 means count byword
/// @param at if f_type is 1XXX this parameter is at's argument
/// @return Output string
char *find(char *__file_path, char *pat_, int f_type, int at)
{
    char *path = malloc(512 * sizeof(char));
    strcpy(path, __file_path);

    // Errors
    if ((f_type & (ALL & AT)) || (f_type & (ALL & COUNT)) || (f_type & (AT & COUNT)) || ((f_type & AT) && at < 0))
    {
        printf("ERROR: wrong input.");
        return "-1";
    }

    int i, foundDot = 0;
    for (i = (int)strlen(path) - 1; path[i] != '/' && i > -1; i--)
    {
        foundDot |= (path[i] == '.');
    }

    if (i == -1 || !foundDot)
    {
        printf("ERROR: Invalid file name.");
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
            printf("ERROR: File should be in root folder.\n");
            chdir(parentDir);
            return "-1";
        }

        isFirst = 0;

        if (chdir(tok))
        {
            printf("Invalid directory.");
            chdir(parentDir);
            return "-1";
        }

        path = NULL;
    }

    if (!__fileExists(filename))
    {
        printf("ERROR: File %s does not exist.", path);
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
        printf("ERROR: Invalid regex.");
        return "-1";
    }

    int cmatch[512];
    int matchno = 0;

    int _dum_ = (int)strlen(str);
    for (int ii = 0; ii < _dum_; ii++)
    {
        int c_index = __find(str, pat, pats, ii);
        if (c_index != -1)
        {
            cmatch[matchno++] = c_index;
            ii = c_index;
        }
    }

    if (!matchno)
    {
        strcat(out, "-1");
        fclose(fp);
        chdir(parentDir);
        return out;
    }

    if (f_type & AT)
    {
        if (at >= matchno)
        {
            strcat(out, "Not enough matches.");
            fclose(fp);
            chdir(parentDir);
            return out;
        }
    }

    if (f_type & COUNT)
    {
        strcat(out, __itoa(matchno));
        fclose(fp);
        chdir(parentDir);
        return out;
    }

    if (f_type & BYWORD)
    {
        __toWordArr(str, cmatch);
    }

    if (f_type & ALL)
    {
        for (int ii = 0; ii < matchno; ii++)
        {
            char *fsdfs = __itoa(cmatch[ii]);
            strcat(out, fsdfs);
            if (ii != matchno - 1)
            {
                strcat(out, ", ");
            }
        }

        fclose(fp);
        chdir(parentDir);
        return out;
    }

    if (f_type & AT)
    {
        strcat(out, __itoa(cmatch[at]));
        fclose(fp);
        chdir(parentDir);
        return out;
    }

    if (matchno)
        strcat(out, __itoa(cmatch[0]));
    else
        strcat(out, "-1");

    fclose(fp);
    chdir(parentDir);
    return out;
}

/// @brief Replaces string str with fill
/// @param __file_path file name
/// @param str pattern to be matched
/// @param fill string to be filled
/// @param at if set to n, replaces nth occurence, if set -1 replaces all
void replace(char *__file_path, char *str, char *fill, int at)
{
    char *path = malloc(512 * sizeof(char));
    strcpy(path, __file_path);

    // Errors
    if (at < -1)
    {
        printf("ERROR: wrong input.");
        return;
    }

    int i, foundDot = 0;
    for (i = (int)strlen(path) - 1; path[i] != '/' && i > -1; i--)
    {
        foundDot |= (path[i] == '.');
    }

    if (i == -1 || !foundDot)
    {
        printf("ERROR: Invalid file name.");
        chdir(parentDir);
        return;
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
            printf("ERROR: File should be in root folder.\n");
            chdir(parentDir);
            return;
        }

        isFirst = 0;

        if (chdir(tok))
        {
            printf("Invalid directory.");
            chdir(parentDir);
            return;
        }

        path = NULL;
    }

    if (!__fileExists(filename))
    {
        printf("ERROR: File %s does not exist.", path);
        chdir(parentDir);
        return;
    }

    FILE *fp = fopen(filename, "r+");

    char *out = calloc(1000, sizeof(out));

    char *fstr = __toString(fp);

    int pats;
    short *pat = __toPat(str, &pats);
    if ((pats == 1 && pat[0] == -1) || (pats == 2 && pat[0] == -1 && pat[1] == -1))
    {
        printf("ERROR: Invalid regex.");
        return;
    }

    int cmatch[512];
    int matchno = 0;

    int _dum_ = strlen(fstr);

    for (int ii = 0; ii < _dum_; ii++)
    {
        int c_index = __find(fstr, pat, pats, ii);
        if (c_index != -1)
        {
            cmatch[matchno++] = c_index;
            ii = c_index;
        }
    }

    printf("\n");

    fclose(fp);

    int purelen;
    short *ppat = __toPat(str, &purelen);

    if ((purelen == 1 && ppat[0] == -1) || (purelen == 2 && ppat[0] == -1 && ppat[1] == -1))
    {
        printf("ERROR: Invalid regex.");
        return;
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
            printf("ERROR: Not enough matches.");
        }
    }

    fclose(fp);
    free(fp);
    chdir(parentDir);
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
        printf("ERROR: Invalid file name.");
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
            printf("ERROR: File should be in root folder.\n");
            chdir(parentDir);
            return "-1";
        }

        isFirst = 0;

        if (chdir(tok))
        {
            printf("Invalid directory.");
            chdir(parentDir);
            return "-1";
        }

        path = NULL;
    }

    if (!__fileExists(filename))
    {
        printf("ERROR: File %s does not exist.", path);
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
        printf("ERROR: Invalid regex.");
        return "-1";
    }

    int cmatch[512];
    int matchno = 0;

    int _dum_ = (int)strlen(str);
    for (int ii = 0; ii < _dum_; ii++)
    {
        int c_index = __find(str, pat, pats, ii);
        if (c_index != -1)
        {
            cmatch[matchno++] = c_index;
            ii = c_index;
        }
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
/// @return output string
char *grep(char **files, int s_files, char *str, char option)
{
    char *out = calloc(100000, sizeof(char));

    if (option == 'l')
    {
        for (int i = 0; i < s_files; i++)
        {
            char *res = find(files[i], str, 0, 0);
            if (strcmp(res, "-1"))
            {
                strcat(out, files[i]);
                strcat(out, "\n");
            }
        }
    }
    else if (option == 'c')
    {
        int o = 0;
        for (int i = 0; i < s_files; i++)
        {
            char *res = find(files[i], str, 0, 0);
            if (strcmp(res, "-1"))
            {
                o++;
            }
        }
        strcat(out, __itoa(o));
    }
    else
    {
        for (int i = 0; i < s_files; i++)
        {
            char *res = __specialFind(files[i], str);
            if (strcmp(res, "-1"))
            {
                strcat(out, res);
            }
        }
    }

    return out;
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
        printf("ERROR: Invalid file name.");
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
            printf("ERROR: File should be in root folder.\n");
            chdir(parentDir);
            return;
        }

        isFirst = 0;

        if (chdir(tok))
        {
            printf("Invalid directory.");
            chdir(parentDir);
            return;
        }

        path = NULL;
    }

    if (!__fileExists(filename))
    {
        printf("ERROR: File %s does not exist.", path);
        chdir(parentDir);
        return;
    }

    FILE *fp = fopen(filename, "r");
    FILE *df = fopen("dummy.dum", "w");
    __copy(fp, df);
    fclose(fp);
    fclose(df);

    fp = fopen(filename, "w");
    FILE *fu = fopen(__undoPath(filename), "r");
    __copy(fu, fp);
    fclose(fp);
    fclose(fu);

    fu = fopen(__undoPath(filename), "w");
    df = fopen("dummy.dum", "r");
    __copy(df, fu);
    fclose(df);
    fclose(fu);

    remove("dummy.dum");
    chdir(parentDir);
}

/// @brief Indents the given file
/// @param __file_path file name
void autoIndent(char *__file_path)
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
        printf("ERROR: Invalid file name.");
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
            printf("ERROR: File should be in root folder.\n");
            chdir(parentDir);
            return;
        }

        isFirst = 0;

        if (chdir(tok))
        {
            printf("ERROR: Invalid directory.");
            chdir(parentDir);
            return;
        }

        path = NULL;
    }

    if (!__fileExists(filename))
    {
        printf("ERROR: File %s does not exist.", path);
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
            int ignorespace = 1;

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
        strcat(out, "\n");
        strcat(out, that);

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
            out[strlen(out) - 2] = '\0';
            strcat(out, "\n");
            strcat(out, outthatone);
        }
        else
        {
            strcat(out, outthis);
            out[strlen(out) - 2] = '\0';
            strcat(out, "\n");
            strcat(out, outthat);
        }
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
/// @return result
char *__cmpFile(FILE *this, FILE *that, char *this_path, char *that_path)
{
    char *out = calloc(100000, sizeof(char));

    int rthis = 1, rthat = 1;

    int _lthis = 0, _wthis, _cthis,
        _lthat = 0, _wthat, _cthat;

    // test this
    chdir(parentDir);
    __originFileLen(this_path, &_cthis, &_lthis, &_wthis);
    chdir(parentDir);
    __originFileLen(that_path, &_cthat, &_lthat, &_wthat);

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
                strcat(out, "============ #");
                strcat(out, __itoa(rthis));
                strcat(out, " ============\n");
                strcat(out, res);
                strcat(out, "\n");
            }

            rthis++;
            rthat++;
        }

        if (rthis == _lthis && rthat < _lthat)
        {
            strcat(out, ">>>>>>>>>>>> #");
            strcat(out, __itoa(rthat));
            strcat(out, " - #");
            strcat(out, __itoa(_lthat));
            strcat(out, " <<<<<<<<<<<<\n");

            strcat(out, linethat);

            while (getline(&linethat, &lenthat, that) != -1)
            {
                strcat(out, linethat);
            }
        }

        if (rthat == _lthat && rthis < _lthis)
        {
            strcat(out, ">>>>>>>>>>>> #");
            strcat(out, __itoa(rthis));
            strcat(out, " - #");
            strcat(out, __itoa(_lthis));
            strcat(out, " <<<<<<<<<<<<\n");

            strcat(out, linethis);
            while (getline(&linethis, &lenthis, this) != -1)
            {
                strcat(out, linethis);
            }
        }
    }

    return out;
}

/// @brief Compares the files
/// @param this_path first file name
/// @param that_path second file name
/// @return output string
char *compareFiles(char *this_path, char *that_path)
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
        printf("ERROR: Invalid file name.");
        chdir(parentDir);
        return "-1";
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
            printf("ERROR: File should be in root folder.\n");
            chdir(parentDir);
            return "-1";
        }

        isFirst = 0;

        if (chdir(tok))
        {
            printf("Invalid directory.");
            chdir(parentDir);
            return "-1";
        }

        path = NULL;
    }

    if (!__fileExists(filename))
    {
        printf("ERROR: File %s does not exist.", path);
        chdir(parentDir);
        return "-1";
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
        printf("ERROR: Invalid file name.");
        chdir(parentDir);
        return "-1";
    }

    char *pfilename = malloc((strlen(ppath) - 1 - i) * sizeof(char));

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
            printf("ERROR: File should be in root folder.\n");
            chdir(parentDir);
            return "-1";
        }

        pisFirst = 0;

        if (chdir(ptok))
        {
            printf("Invalid directory.");
            chdir(parentDir);
            return "-1";
        }

        ppath = NULL;
    }

    if (!__fileExists(pfilename))
    {
        printf("ERROR: File %s does not exist.", ppath);
        chdir(parentDir);
        return "-1";
    }

    FILE *thatfile = fopen(pfilename, "r+");

    return __cmpFile(thisfile, thatfile, this_path, that_path);
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

/// @brief Helper function for tree
/// @param depth current depth of tree
/// @param out output string
/// @param precspace preceding space
void __tree(int depth, char *out, int precspace)
{
    if (depth == 0)
        return;

    struct dirent *de;
    DIR *dr = opendir(".");

    if (dr == NULL)
    {
        return;
    }

    while ((de = readdir(dr)) != NULL)
    {

        if ((de->d_name)[0] == '.')
            continue;

        for (int k = 0; k < precspace; k++)
            strcat(out, " ");

        strcat(out, "|--> ");
        strcat(out, de->d_name);
        strcat(out, "\n");

        if (!__isFile(de->d_name))
        {
            chdir(de->d_name);

            __tree(depth - 1, out, precspace + 5);

            chdir("..");
        }
    }
}

/// @brief Shows directory tree
/// @param depth depth of tree, -1 means show all
/// @return output string
char *tree(int depth)
{
    char *out = calloc(10000, sizeof(char));

    chdir("root");

    strcat(out, "root\n");

    __tree(depth, out, 0);

    chdir(parentDir);
    return out;
}

/// @brief Handles input from user (doesn't handle arman)
/// @param inp input command
void handler(char *inp)
{
    char arg[128][32768];
    int argc = 0;
    int lenc = strlen(inp);

    char currarg[32768];
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
                strcpy(arg[argc], currarg);
                argc++;
                if (argc >= 128)
                {
                    printf("ERROR: Invalid number of arguments.");
                    return;
                }
                currarglen = 0;
                currarg[0] = '\0';
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
                        currarglen++;
                    }
                    else if (inp[i + 1] == 't')
                    {
                        strcat(currarg, "\t");
                        currarglen++;
                    }
                    else if (inp[i + 1] == 'v')
                    {
                        strcat(currarg, "\v");
                        currarglen++;
                    }
                    else if (inp[i + 1] != '"')
                    {
                        printf("ERROR: Invalid or unsupported escape character.");
                        return;
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
            printf("ERROR: Argument size exceeded (32768 characters)");
            return;
        }
    }

    if (currarglen)
    {
        strcpy(arg[argc], currarg);
        argc++;
        if (argc >= 128)
        {
            printf("ERROR: Invalid number of arguments.");
            return;
        }
        currarg[0] = '\0';
        currarglen = 0;
    }

    enum COMMAND com = INVALID;

    for (int i = 0; i < 15; i++)
    {
        if (!strcmp(arg[0], COMSTR[i]))
        {
            com = ixtocom[i];
            break;
        }
    }

    switch (com)
    {
    case CREATEFILE:

        if (argc != 3 || strcmp(arg[1], "--file"))
        {
            printf("Invalid command.");
            return;
        }

        createFile(arg[2]);

        break;

    case INSERTSTR:

        if (argc != 7 || strcmp(arg[1], "--file") || strcmp(arg[3], "--str") || strcmp(arg[5], "--pos"))
        {
            printf("Invalid command.");
            return;
        }

        int lineno, startpos;

        if (sscanf(arg[6], "%d:%d", &lineno, &startpos) != 2)
        {
            printf("Invalid command.");
            return;
        }

        insertStr(arg[2], arg[4], lineno, startpos);

        break;

    case CAT:

        if (argc != 3 || strcmp(arg[1], "--file"))
        {
            printf("Invalid command.");
            return;
        }

        cat(arg[2]);

        break;

    case REMOVESTR:

        if (argc != 8 || strcmp(arg[1], "--file") || strcmp(arg[3], "--pos") || strcmp(arg[5], "-size"))
        {
            printf("Invalid command.");
            return;
        }

        int lineno, startpos, forward, cnt;

        if (sscanf(arg[4], "%d:%d", &lineno, &startpos) != 2)
        {
            printf("Invalid command.");
            return;
        }

        if (sscanf(arg[6], "%d", &cnt) != 1)
        {
            printf("Invalid command.");
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
            printf("Invalid command.");
            return;
        }

        removeStr(arg[2], lineno, startpos, cnt, forward);

        break;

    case COPYSTR:

        if (argc != 8 || strcmp(arg[1], "--file") || strcmp(arg[3], "--pos") || strcmp(arg[5], "-size"))
        {
            printf("Invalid command.");
            return;
        }

        int lineno, startpos, forward, cnt;

        if (sscanf(arg[4], "%d:%d", &lineno, &startpos) != 2)
        {
            printf("Invalid command.");
            return;
        }

        if (sscanf(arg[6], "%d", &cnt) != 1)
        {
            printf("Invalid command.");
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
            printf("Invalid command.");
            return;
        }

        copyStr(arg[2], lineno, startpos, cnt, forward);

        break;

    case CUTSTR:

        if (argc != 8 || strcmp(arg[1], "--file") || strcmp(arg[3], "--pos") || strcmp(arg[5], "-size"))
        {
            printf("Invalid command.");
            return;
        }

        int lineno, startpos, forward, cnt;

        if (sscanf(arg[4], "%d:%d", &lineno, &startpos) != 2)
        {
            printf("Invalid command.");
            return;
        }

        if (sscanf(arg[6], "%d", &cnt) != 1)
        {
            printf("Invalid command.");
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
            printf("Invalid command.");
            return;
        }

        cutStr(arg[2], lineno, startpos, cnt, forward);

        break;

    case PASTESTR:

        if (argc != 5 || strcmp(arg[1], "--file") || strcmp(arg[3], "--pos"))
        {
            printf("Invalid command.");
            return;
        }

        int lineno, startpos;

        if (sscanf(arg[4], "%d:%d", &lineno, &startpos) != 2)
        {
            printf("Invalid command.");
            return;
        }

        pasteStr(arg[2], lineno, startpos);

        break;

    case FIND:

        if (argc >= 5) /*normal*/
        {
            if (strcmp(arg[1], "--str") || strcmp(arg[3], "--file"))
            {
                printf("Invalid command.");
                return;
            }

            int byword = 0, at = 0, count = 0, all = 0, atnum = -1, otherargc = argc - 5;

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
                    printf("Invalid command.");
                    return;
                }
            }

            if (byword > 1 || at > 1 || count > 1 || all > 1 || (at == 1 && atnum == -1))
            {
                printf("Invalid command.");
                return;
            }

            int type = (byword ? BYWORD : 0) +
                       (count ? COUNT : 0) +
                       (all ? ALL : 0) +
                       (at ? AT : 0);

            printf("%s\n", find(arg[4], arg[2], type, atnum));
        }
        else
        {
            printf("Invalid command.");
            return;
        }

        break;

    case REPLACE:

        if (argc >= 6) /*normal*/
        {
            if (strcmp(arg[1], "--str1") || strcmp(arg[3], "--str2") || strcmp(arg[5], "--file"))
            {
                printf("Invalid command.");
                return;
            }

            if (argc == 6)
            {
                replace(arg[6], arg[2], arg[4], -2);
            }
            else if (argc == 7)
            {
                if (!strcmp(arg[6], "-all"))
                {
                    replace(arg[6], arg[2], arg[4], -1);
                }
                else
                {
                    printf("Invalid command.");
                    return;
                }
            }
            else if (argc == 8)
            {
                if (!strcmp(arg[6], "-at") && _isnum(arg[7]))
                {
                    replace(arg[6], arg[2], arg[4], _tonum(arg[7]));
                }
                else
                {
                    printf("Invalid command.");
                    return;
                }
            }
            else
            {
                printf("Invalid command.");
                return;
            }
        }
        else
        {
            printf("Invalid command.");
            return;
        }

        break;

    case GREP:

        if (argc >= 5)
        {
            if (!strcat(arg[1], "--str"))
            {
                if (strcat(arg[3], "--files"))
                {
                    printf("Invalid command.");
                    return;
                }
                else
                {
                    printf("%s\n", grep(arg + 4, argc - 4, arg[2], 'n'));
                }
            }
            else if (!strcat(arg[1], "-c") || !strcat(arg[1], "-l"))
            {
                if (strcat(arg[4], "--files"))
                {
                    printf("Invalid command.");
                    return;
                }
                else
                {
                    printf("%s\n", grep(arg + 5, argc - 5, arg[2], arg[1][1]));
                }
            }
            else
            {
                printf("Invalid command.");
                return;
            }
        }
        else
        {
            printf("Invalid command.");
            return;
        }

        break;

    case UNDO:

        if (argc != 3 || strcmp(arg[1], "--file"))
        {
            printf("Invalid command.");
            return;
        }

        undo(arg[2]);

        break;

    case AUTOINDENT:
        
        if (argc != 2)
        {
            printf("Invalid command.");
            return;
        }

        autoIndent(arg[1]);

        break;

    case COMPARE:
        
        if (argc != 3)
        {
            printf("Invalid command.");
            return;
        }

        printf("%s\n", compareFiles(arg[1], arg[2]));

        break;

    case TREE:
        
        if (argc != 2)
        {
            printf("Invalid command.");
            return;
        }

        printf("%s\n", _tonum(arg[1]));

        break;

    case INVALID:
        /* code */
        break;

    default:
        break;
    }
}

void Handler(char *inp)
{
    int n = strlen(inp);

    char first[32768 * 128];
    char second[32768 * 128];
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
            strncpy(first, inp, i);
            strncpy(second, inp + i + 4, n - i - 4);
            isarman = 1;
            break;
        }
    }

    if (isarman)
    {
        // handle first and second
    }
    else
    {
        // handle inp
    }
}

int main()
{
    init();
    char s[32768 * 20];
    int len = 0;

    gets(s);
    printf("%s", s);

    handler(s);

    // char *s1 = "/root/bruh/wtf/this/is/a/test/myfile2.txt";
    // char *s2 = "/root/bruh/wtf/this/is/a/test/myfile1.txt";

    // printf("%s", tree(-1));

    // cat(s);
    // autoIndent(s);
    // cat(s);

    // char **f = malloc(1 * sizeof(char *));
    // f[0] = s;

    // printf("%s", grep(f, 1, "bruh", 'l'));

    // replace(s, "bruhbrdsadas", "xxx", -1);

    // createFile(s);
    // insertStr(s, "bruh bruh bruh kdlskdfsf fddsfs\nfsfsd bruh", 1, 0);
    // cat(s);

    // char* jk = find(s, "sdfsdfsdf", BYWORD | AT, -1);

    // puts(jk);

    // strcpy(str, "bd\\*dssd*");

    // cat(buf);

    //   0000000000111111111122222222223333333333444444444455555555556666666666
    //   0123456789012345678901234567890123456789012345678901234567890123456789
    // char* str = " how is this even possible? lmao wtf lol   damn hiiiii bruh jhi";
    // int a[5] = {3, 10, 18, 25, 55};

    // int strs = sizeof(str) / sizeof(str[0]);

    // __toWordArr(str, a);

    // for(int i = 0; i < 5; i++)
    //     printf("%d ", a[i]);

    finish();
}

/*

hi this is a test
wtf
12 123
123 4125235
124235 2402i3 2402i42
owpjefs ffsk sld;ln l;msd;fln
dflm;


*/