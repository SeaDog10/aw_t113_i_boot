#ifndef __XSTRING_H__
#define __XSTRING_H__

#define X_NULL 0

void *xmemset(void *dst, int val, int cnt);
int xmemcmp(const void *dst, const void *src, unsigned int cnt);
// unsigned int xstrlen(const char *str);
char *xstrcpy(char *dst, const char *src);
char *xstrcat(char *dst, const char *src);
int xstrcmp(const char *p1, const char *p2);
int xstrncmp(const char *p1, const char *p2, unsigned int cnt);
char *xstrchr(const char *s, int c);
char *xstrstr(const char *s1, const char *s2);
void *xmemchr(void *src, int val, unsigned int cnt);
void *xmemmove(void *dst, const void *src, unsigned int cnt);

#endif
