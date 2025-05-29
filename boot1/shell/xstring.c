#include "xstring.h"

void *xmemset(void *dst, int val, int cnt)
{
    char *d = (char *)dst;

    while (cnt--)
    {
        *d++ = (char)val;
    }

    return dst;
}

int xmemcmp(const void *dst, const void *src, unsigned int cnt)
{
    const char *d = (const char *)dst;
    const char *s = (const char *)src;
    int            r = 0;

    while (cnt-- && (r = *d++ - *s++) == 0);

    return r;
}

unsigned int xstrlen(const char *str)
{
    int i = 0;

    while (str[i++] != '\0');

    return i - 1;
}

char *xstrcpy(char *dst, const char *src)
{
    char *bak = dst;

    while ((*dst++ = *src++) != '\0');

    return bak;
}

char *xstrcat(char *dst, const char *src)
{
    char *p = dst;

    while (*dst != '\0')
    {
        dst++;
    }

    while ((*dst++ = *src++) != '\0');

    return p;
}

int xstrcmp(const char *p1, const char *p2)
{
    unsigned char c1, c2;

    while (1)
    {
        c1 = *p1++;
        c2 = *p2++;
        if (c1 != c2)
            return c1 < c2 ? -1 : 1;
        if (!c1)
            break;
    }

    return 0;
}

int xstrncmp(const char *p1, const char *p2, unsigned int cnt)
{
    unsigned char c1, c2;

    while (cnt--)
    {
        c1 = *p1++;
        c2 = *p2++;

        if (c1 != c2)
        {
            return c1 < c2 ? -1 : 1;
        }

        if (!c1)
        {
            break;
        }
    }

    return 0;
}

char *xstrchr(const char *s, int c)
{
    for (; *s != (char)c; ++s)
    {
        if (*s == '\0')
        {
            return X_NULL;
        }
    }

    return (char *)s;
}

/* NOTE: This is the simple-minded O(len(s1) * len(s2)) worst-case approach. */

char *xstrstr(const char *s1, const char *s2)
{
    register const char *s = s1;
    register const char *p = s2;

    do {
        if (!*p)
        {
            return (char *)s1;
        }

        if (*p == *s)
        {
            ++p;
            ++s;
        }
        else
        {
            p = s2;
            if (!*s)
            {
                return X_NULL;
            }
            s = ++s1;
        }
    } while (1);
}

void *xmemchr(void *src, int val, unsigned int cnt)
{
    char *p = X_NULL;
    char *s = (char *)src;

    while (cnt)
    {
        if (*s == val)
        {
            p = s;
            break;
        }
        s++;
        cnt--;
    }

    return p;
}

void *xmemmove(void *dst, const void *src, unsigned int cnt)
{
    char *p, *s;

    if (dst <= src)
    {
        p = (char *)dst;
        s = (char *)src;
        while (cnt--)
        {
            *p++ = *s++;
        }
    }
    else
    {
        p = (char *)dst + cnt;
        s = (char *)src + cnt;
        while (cnt--)
        {
            *--p = *--s;
        }
    }

    return dst;
}
