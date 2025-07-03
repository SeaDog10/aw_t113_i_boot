/***************************************************************************
 * Copyright (c) 2025 HGBOOT Authors
 *
 * This program and the accompanying materials are made available under the
 * terms of the MIT License which is available at
 * https://opensource.org/licenses/MIT.
 *
 * SPDX-License-Identifier: MIT
 **************************************************************************/

#include "shell/xstring.h"

#define X_NULL 0

/**
 * @brief Set a block of memory to a specified value.
 * @param dst Pointer to the destination memory block.
 * @param val Value to set.
 * @param cnt Number of bytes to set.
 * @return Pointer to the destination memory block.
 */
void *xmemset(void *dst, int val, int cnt)
{
    char *d = (char *)dst;

    while (cnt--)
    {
        *d++ = (char)val;
    }

    return dst;
}

/**
 * @brief Compare two memory blocks.
 * @param dst Pointer to the first memory block.
 * @param src Pointer to the second memory block.
 * @param cnt Number of bytes to compare.
 * @return 0 if equal, otherwise the difference between the first differing bytes.
 */
int xmemcmp(const void *dst, const void *src, unsigned int cnt)
{
    const char *d = (const char *)dst;
    const char *s = (const char *)src;
    int            r = 0;

    while (cnt-- && (r = *d++ - *s++) == 0);

    return r;
}

/**
 * @brief Get the length of a string.
 * @param str Pointer to the string.
 * @return Length of the string (excluding the null terminator).
 */
unsigned int xstrlen(const char *str)
{
    int i = 0;

    while (str[i++] != '\0');

    return i - 1;
}

/**
 * @brief Copy a string.
 * @param dst Pointer to the destination buffer.
 * @param src Pointer to the source string.
 * @return Pointer to the destination buffer.
 */
char *xstrcpy(char *dst, const char *src)
{
    char *bak = dst;

    while ((*dst++ = *src++) != '\0');

    return bak;
}

/**
 * @brief Copy a string with a maximum length.
 * @param dst Pointer to the destination buffer.
 * @param src Pointer to the source string.
 * @param n Maximum number of bytes to copy.
 * @return Pointer to the destination buffer.
 */
char *xstrncpy(char *dst, const char *src, unsigned int n)
{
    char *bak = dst;
    char *d = dst;
    const char *s = src;

    if (n != 0)
    {
        do
        {
            if ((*d++ = *s++) == 0)
            {
                /* NUL pad the remaining n-1 bytes */
                while (--n != 0)
                    *d++ = 0;
                break;
            }
        } while (--n != 0);
    }

    return bak;
}

/**
 * @brief Concatenate two strings.
 * @param dst Pointer to the destination buffer.
 * @param src Pointer to the source string.
 * @return Pointer to the destination buffer.
 */
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

/**
 * @brief Compare two strings.
 * @param p1 Pointer to the first string.
 * @param p2 Pointer to the second string.
 * @return 0 if equal, <0 if p1 < p2, >0 if p1 > p2.
 */
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

/**
 * @brief Compare two strings up to a specified length.
 * @param p1 Pointer to the first string.
 * @param p2 Pointer to the second string.
 * @param cnt Maximum number of bytes to compare.
 * @return 0 if equal, <0 if p1 < p2, >0 if p1 > p2.
 */
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

/**
 * @brief Locate the first occurrence of a character in a string.
 * @param s Pointer to the string.
 * @param c Character to locate.
 * @return Pointer to the found character, or X_NULL if not found.
 */
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

/**
 * @brief Locate a substring in a string.
 * @param s1 Pointer to the main string.
 * @param s2 Pointer to the substring.
 * @return Pointer to the first occurrence of s2 in s1, or X_NULL if not found.
 */
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

/**
 * @brief Locate the first occurrence of a value in a memory block.
 * @param src Pointer to the memory block.
 * @param val Value to locate.
 * @param cnt Number of bytes to search.
 * @return Pointer to the found value, or X_NULL if not found.
 */
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

/**
 * @brief Move a block of memory, handling overlap.
 * @param dst Pointer to the destination memory block.
 * @param src Pointer to the source memory block.
 * @param cnt Number of bytes to move.
 * @return Pointer to the destination memory block.
 */
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

/**
 * @brief Convert a string to a long integer.
 * @param str Pointer to the string.
 * @param out Pointer to the output integer.
 * @return 0 on success, negative value on error.
 */
int xstrtol(const char *str, int *out)
{
    int base = 10;
    int result = 0;
    int sign = 1;
    char c = 0;
    int digit = 0;

    while (*str == ' ' || *str == '\t')
    {
        str++;
    }

    if (*str == '-')
    {
        sign = -1;
        str++;
    }
    else if (*str == '+')
    {
        str++;
    }

    if (*str == '0' && (*(str + 1) == 'x' || *(str + 1) == 'X'))
    {
        base = 16;
        str += 2;
    }

    while (*str)
    {
        c = *str++;

        if (c >= '0' && c <= '9')
        {
            digit = c - '0';
        }
        else if (c >= 'a' && c <= 'f')
        {
            digit = 10 + (c - 'a');
        }
        else if (c >= 'A' && c <= 'F')
        {
            digit = 10 + (c - 'A');
        }
        else
        {
            return -2;
        }

        if (digit >= base)
        {
            return -3;
        }

        result = result * base + digit;
    }

    *out = result * sign;

    return 0;
}
