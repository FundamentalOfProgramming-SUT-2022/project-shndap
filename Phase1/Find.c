#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int nextspace(char *str, int i_0, int str_size)
{
    for (int i = i_0; i < str_size; i++)
    {
        if (str[i] == ' ')
        {
            return i;
        }
    }

    return str_size;
}

int nextwildcard(short *str, int i_0, int str_size)
{
    for (int i = i_0; i < str_size; i++)
    {
        if (str[i] == -1)
        {
            return i;
        }
    }

    return str_size;
}

int __matchc(short patc, char strc)
{
    if (patc == -1)
    {
        return (strc != ' ' && strc != '\n' && strc != '\0' && strc != ' ' && strc != EOF);
    }

    return (strc == patc);
}

int match_str(const char *str, const short *pat)
{
    const short *star = NULL;
    const char *ss = str;

    while (*str && *pat != -2)
    {
        if (*pat == -1)
        {
            if (__matchc(*pat, *str))
            {
                star = pat++;
                ss = str;
                continue;
            }
            else
            {
                star = NULL;
                ss = str;
                pat++;
                continue;
            }
        }
        else
        {
            if (__matchc(*pat, *str))
            {
                if (*str == ' ' || *str  == '\n' || *str == '\t')
                {
                    star = NULL;
                }
                str++;
                pat++;
                continue;
            }
        }

        if (star)
        {
            pat = star + 1;
            if (*str == ' ' || *str  == '\n' || *str == '\t')
            {
                star = NULL;
            }
            str = ++ss;
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

int next_match(char *str, short *pat, int strs, int pats, int i_0)
{
    for (int i = i_0; i < strs; i++)
    {
        if(match_str(str + i, pat))
        {
            while(i < strs && pat[0] == -1 && __matchc(pat[0], str[i]))
            {
                i++;
            }

            return i == strs ? -1 : i;
        }
    }

    return -1;
}

int main()
{

    char str[] = "is  from bis  from bible:\n\t\"let there be light\"\n\t\t\t\t-Holy Bible, psalms 1:134\ndude wtf is this\\na quote";

    short pat[] = {-1, 'a', ' ', 'q', 'u', 'o', 't', 'e', -2};

    // printf("%d", next_match(str, pat, 104, 9, 0));
    printf("%c", str[78]);

    return 0;
}