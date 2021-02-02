/*
 * string.c
 *
 * Copyright (C) 2021 bzt (bztsrc@gitlab)
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * This file is part of the POSIX-UEFI package.
 * @brief Implementing functions which are defined in string.h
 *
 */

#include <uefi.h>

/* for gcc we use the built-in variants, but clang doesn't have those */
#ifdef __clang__
void *memcpy(void *dst, const void *src, size_t n)
{
    uint8_t *a=(uint8_t*)dst,*b=(uint8_t*)src;
    if(src && dst && n>0) {
        while(n--) *a++ = *b++;
    }
    return dst;
}

void *memmove(void *dst, const void *src, size_t n)
{
    uint8_t *a=(uint8_t*)dst,*b=(uint8_t*)src;
    if(src && dst && n>0) {
        if((a<b && a+n>b) || (b<a && b+n>a)) {
            a+=n-1; b+=n-1; while(n-->0) *a--=*b--;
        } else {
            while(n--) *a++ = *b++;
        }
    }
    return dst;
}

void *memset(void *s, int c, size_t n)
{
    uint8_t *p=(uint8_t*)s;
    if(s && n>0) {
        while(n--) *p++ = c;
    }
    return s;
}

int memcmp(const void *s1, const void *s2, size_t n)
{
    uint8_t *a=(uint8_t*)s1,*b=(uint8_t*)s2;
    if(s1 && s2 && n>0) {
        while(n--) {
            if(*a != *b) return *a - *b;
            a++; b++;
        }
    }
    return 0;
}

void *memchr (const void *s, int c, size_t n)
{
    uint8_t *e, *p=(uint8_t*)s;
    if(s && n>0) {
        for(e=p+n; p<e; p++) if(*p==(uint8_t)c) return p;
    }
    return NULL;
}

void *memrchr (const void *s, int c, size_t n)
{
    uint8_t *e, *p=(uint8_t*)s;
    if(s && n>0) {
        for(e=p+n; p<e; --e) if(*e==(uint8_t)c) return e;
    }
    return NULL;
}
#endif

void *memmem(const void *haystack, size_t hl, const void *needle, size_t nl)
{
    uint8_t *c = (uint8_t*)haystack;
    if(!haystack || !needle || !hl || !nl || nl > hl) return NULL;
    hl -= nl;
    while(hl) {
        if(!memcmp(c, needle, nl)) return c;
        c++; hl--;
    }
    return NULL;
}

void *memrmem(const void *haystack, size_t hl, const void *needle, size_t nl)
{
    uint8_t *c = (uint8_t*)haystack;
    if(!haystack || !needle || !hl || !nl || nl > hl) return NULL;
    hl -= nl;
    c += hl;
    while(hl) {
        if(!memcmp(c, needle, nl)) return c;
        c--; hl--;
    }
    return NULL;
}

wchar_t *strcpy(wchar_t *dst, const wchar_t *src)
{
    if(src && dst) {
        while(*src) {*dst++=*src++;} *dst=0;
    }
    return dst;
}

wchar_t *strncpy(wchar_t *dst, const wchar_t *src, size_t n)
{
    const wchar_t *e = src+n;
    if(src && dst && n>0) {
        while(*src && src<e) {*dst++=*src++;} *dst=0;
    }
    return dst;
}

wchar_t *strcat(wchar_t *dst, const wchar_t *src)
{
    if(src && dst) {
        dst += strlen(dst);
        while(*src) {*dst++=*src++;} *dst=0;
    }
    return dst;
}

int strcmp(const wchar_t *s1, const wchar_t *s2)
{
    if(s1 && s2 && s1!=s2) {
        do{if(*s1!=*s2){return *s1-*s2;}s1++;s2++;}while(*s1!=0);
        return *s1-*s2;
    }
    return 0;
}

wchar_t *strncat(wchar_t *dst, const wchar_t *src, size_t n)
{
    const wchar_t *e = src+n;
    if(src && dst && n>0) {
        dst += strlen(dst);
        while(*src && src<e) {*dst++=*src++;} *dst=0;
    }
    return dst;
}

int strncmp(const wchar_t *s1, const wchar_t *s2, size_t n)
{
    const wchar_t *e = s1+n;
    if(s1 && s2 && s1!=s2 && n>0) {
        do{if(*s1!=*s2){return *s1-*s2;}s1++;s2++;}while(*s1!=0 && s1<e);
        return *s1-*s2;
    }
    return 0;
}

wchar_t *strdup(const wchar_t *s)
{
    int i = (strlen(s)+1) * sizeof(wchar_t);
    wchar_t *s2 = (wchar_t *)malloc(i);
    if(s2 != NULL) memcpy(s2, (void*)s, i);
    return s2;
}

wchar_t *strchr(const wchar_t *s, int c)
{
    if(s) {
        while(*s) {
            if(*s == (wchar_t)c) return (wchar_t*)s;
            s++;
        }
    }
    return NULL;
}

wchar_t *strrchr(const wchar_t *s, int c)
{
    wchar_t *e;
    if(s) {
        e = (wchar_t*)s + strlen(s) - 1;
        while(s < e) {
            if(*e == (wchar_t)c) return e;
            s--;
        }
    }
    return NULL;
}

wchar_t *strstr(const wchar_t *haystack, const wchar_t *needle)
{
    return memmem(haystack, strlen(haystack) * sizeof(wchar_t), needle, strlen(needle) * sizeof(wchar_t));
}

wchar_t *_strtok_r(wchar_t *s, const wchar_t *d, wchar_t **p)
{
    int c, sc;
    wchar_t *tok, *sp;

    if(d == NULL || (s == NULL && (s=*p) == NULL)) return NULL;
again:
    c = *s++;
    for(sp = (wchar_t *)d; (sc=*sp++)!=0;) {
        if(c == sc) { *p=s; *(s-1)=0; return s-1; }
    }

    if (c == 0) { *p=NULL; return NULL; }
    tok = s-1;
    while(1) {
        c = *s++;
        sp = (wchar_t *)d;
        do {
            if((sc=*sp++) == c) {
                if(c == 0) s = NULL;
                else *(s-1) = 0;
                *p = s;
                return tok;
            }
        } while(sc != 0);
    }
    return NULL;
}

wchar_t *strtok(wchar_t *s, const wchar_t *delim)
{
    wchar_t *p = s;
    return _strtok_r (s, delim, &p);
}

wchar_t *strtok_r(wchar_t *s, const wchar_t *delim, wchar_t **ptr)
{
    return _strtok_r (s, delim, ptr);
}

size_t strlen (const wchar_t *__s)
{
    size_t ret;

    if(!__s) return 0;
    for(ret = 0; __s[ret]; ret++);
    return ret;
}
