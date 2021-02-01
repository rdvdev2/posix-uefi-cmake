/*
 * stdio.c
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
 * @brief Implementing functions which are defined in stdio.h
 *
 */

#include <uefi.h>

static efi_file_handle_t *__root_dir = NULL;

void __stdio_init()
{
    efi_status_t status;
    efi_guid_t sfsGuid = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;
    efi_simple_file_system_protocol_t *sfs = NULL;
    if(!__root_dir && LIP) {
        status = uefi_call_wrapper(BS->HandleProtocol, 3, LIP->DeviceHandle, &sfsGuid, &sfs);
        if(!EFI_ERROR(status))
            status = uefi_call_wrapper(sfs->OpenVolume, 2, sfs, &__root_dir);
    }
}

int fclose (FILE *__stream)
{
    efi_status_t status = uefi_call_wrapper(__stream->Close, 1, __stream);
    free(__stream);
    return !EFI_ERROR(status);
}

int fflush (FILE *__stream)
{
    efi_status_t status = uefi_call_wrapper(__stream->Flush, 1, __stream);
    return !EFI_ERROR(status);
}

FILE *fopen (const wchar_t *__filename, const char *__modes)
{
    FILE *ret;
    efi_status_t status;
    __stdio_init();
    errno = 0;
    if(!__root_dir) {
        errno = ENODEV;
        return NULL;
    }
    ret = (FILE*)malloc(sizeof(FILE));
    if(!ret) return NULL;
    status = uefi_call_wrapper(__root_dir->Open, 5, __root_dir, ret, __filename,
        __modes[0] == L'r' ? EFI_FILE_MODE_READ : (EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE), 0);
    if(EFI_ERROR(status)) {
        switch((int)(status & 0xffff)) {
            case EFI_WRITE_PROTECTED & 0xffff: errno = EROFS; break;
            case EFI_ACCESS_DENIED & 0xffff: errno = EACCES; break;
            case EFI_VOLUME_FULL & 0xffff: errno = ENOSPC; break;
            case EFI_NOT_FOUND & 0xffff: errno = ENOENT; break;
            default: errno = EIO; break;
        }
        free(ret); ret = NULL;
    }
    if(__modes[0] == L'a') fseek(ret, 0, SEEK_END);
    return ret;
}

size_t fread (void *__ptr, size_t __size, size_t __n, FILE *__stream)
{
    uintn_t bs = __size * __n;
    efi_status_t status = uefi_call_wrapper(__stream->Read, 3, __stream, &bs, __ptr);
    if(status == EFI_END_OF_FILE) bs = 0;
    return bs / __size;
}

size_t fwrite (const void *__ptr, size_t __size, size_t __n, FILE *__stream)
{
    uintn_t bs = __size * __n;
    efi_status_t status = uefi_call_wrapper(__stream->Write, 3, __stream, &bs, __ptr);
    return bs / __size;
}

int fseek (FILE *__stream, long int __off, int __whence)
{
    uint64_t off = 0;
    efi_status_t status;
    efi_guid_t infoGuid = EFI_FILE_INFO_GUID;
    efi_file_info_t *info;
    uintn_t infosiz = sizeof(efi_file_info_t) + 16;
    switch(__whence) {
        case SEEK_END:
            status = uefi_call_wrapper(__stream->GetInfo, 4, __stream, &infoGuid, &infosiz, info);
            if(!EFI_ERROR(status)) {
                off = info->FileSize + __off;
                status = uefi_call_wrapper(__stream->SetPosition, 2, __stream, &off);
            }
            break;
        case SEEK_CUR:
            status = uefi_call_wrapper(__stream->GetPosition, 2, __stream, &off);
            if(!EFI_ERROR(status)) {
                off += __off;
                status = uefi_call_wrapper(__stream->SetPosition, 2, __stream, &off);
            }
            break;
        default:
            status = uefi_call_wrapper(__stream->SetPosition, 2, __stream, &off);
            break;
    }
    return EFI_ERROR(status) ? -1 : 0;
}

long int ftell (FILE *__stream)
{
    uint64_t off = 0;
    efi_status_t status = uefi_call_wrapper(__stream->GetPosition, 2, __stream, &off);
    return EFI_ERROR(status) ? -1 : (long int)off;
}

int vsnprintf(wchar_t *dst, size_t maxlen, const wchar_t *fmt, __builtin_va_list args)
{
#define needsescape(a) (a==L'\"' || a==L'\\' || a==L'\a' || a==L'\b' || a==L'\033' || a=='\f' || \
    a==L'\r' || a==L'\n' || a==L'\t' || a=='\v')
    long int arg;
    int len, sign, i;
    wchar_t *p, *orig=dst, tmpstr[19], pad=' ';
    char *c;

    if(dst==NULL || fmt==NULL)
        return 0;

    arg = 0;
    while(*fmt && (dst - orig) + 1 < maxlen) {
        if(*fmt==L'%') {
            fmt++;
            if(*fmt==L'%') goto put;
            len=0;
            if(*fmt==L'0') pad=L'0';
            while(*fmt>=L'0' && *fmt<=L'9') {
                len *= 10;
                len += *fmt-L'0';
                fmt++;
            }
            if(*fmt==L'l') fmt++;
            if(*fmt==L'c') {
                arg = __builtin_va_arg(args, int);
                *dst++ = (wchar_t)(arg & 0xffff);
                fmt++;
                continue;
            } else
            if(*fmt==L'C') {
                c = __builtin_va_arg(args, char*);
                arg = *c;
                if((*c & 128) != 0) {
                    if((*c & 32) == 0 ) { arg = ((*c & 0x1F)<<6)|(*(c+1) & 0x3F); } else
                    if((*c & 16) == 0 ) { arg = ((*c & 0xF)<<12)|((*(c+1) & 0x3F)<<6)|(*(c+2) & 0x3F); } else
                    if((*c & 8) == 0 ) { arg = ((*c & 0x7)<<18)|((*(c+1) & 0x3F)<<12)|((*(c+2) & 0x3F)<<6)|(*(c+3) & 0x3F); }
                    else arg = L'?';
                }
                *dst++ = (wchar_t)(arg & 0xffff);
                fmt++;
                continue;
            } else
            if(*fmt==L'd') {
                arg = __builtin_va_arg(args, int);
                sign=0;
                if((int)arg<0) {
                    arg*=-1;
                    sign++;
                }
                if(arg>99999999999999999L) {
                    arg=99999999999999999L;
                }
                i=18;
                tmpstr[i]=0;
                do {
                    tmpstr[--i]=L'0'+(arg%10);
                    arg/=10;
                } while(arg!=0 && i>0);
                if(sign) {
                    tmpstr[--i]=L'-';
                }
                if(len>0 && len<18) {
                    while(i>18-len) {
                        tmpstr[--i]=pad;
                    }
                }
                p=&tmpstr[i];
                goto copystring;
            } else
            if(*fmt==L'p' || *fmt==L'P') {
                arg = __builtin_va_arg(args, uint64_t);
                len = 16; pad = L'0'; goto hex;
            } else
            if(*fmt==L'x' || *fmt==L'X' || *fmt==L'p') {
                arg = __builtin_va_arg(args, long int);
                if(*fmt==L'p') { len = 16; pad = L'0'; }
hex:            i=16;
                tmpstr[i]=0;
                do {
                    wchar_t n=arg & 0xf;
                    /* 0-9 => '0'-'9', 10-15 => 'A'-'F' */
                    tmpstr[--i]=n+(n>9?(*fmt==L'X'?0x37:0x57):0x30);
                    arg>>=4;
                } while(arg!=0 && i>0);
                /* padding, only leading zeros */
                if(len>0 && len<=16) {
                    while(i>16-len) {
                        tmpstr[--i]=L'0';
                    }
                }
                p=&tmpstr[i];
                goto copystring;
            } else
            if(*fmt==L's' || *fmt==L'q') {
                p = __builtin_va_arg(args, wchar_t*);
copystring:     if(p==NULL) {
                    p=L"(null)";
                }
                while(*p && (dst - orig) + 2 < maxlen) {
                    if(*fmt==L'q' && needsescape(*p)) {
                        *dst++ = L'\\';
                        switch(*p) {
                            case L'\a': *dst++ = L'a'; break;
                            case L'\b': *dst++ = L'b'; break;
                            case L'\e': *dst++ = L'e'; break;
                            case L'\f': *dst++ = L'f'; break;
                            case L'\n': *dst++ = L'n'; break;
                            case L'\r': *dst++ = L'r'; break;
                            case L'\t': *dst++ = L't'; break;
                            case L'\v': *dst++ = L'v'; break;
                            default: *dst++ = *p++; break;
                        }
                    } else
                        *dst++ = *p++;
                }
            } else
            if(*fmt==L'S' || *fmt==L'Q') {
                c = __builtin_va_arg(args, char*);
                if(c==NULL) goto copystring;
                while(*p && (dst - orig) + 2 < maxlen) {
                    arg = *c;
                    if((*c & 128) != 0) {
                        if((*c & 32) == 0 ) {
                            arg = ((*c & 0x1F)<<6)|(*(c+1) & 0x3F);
                            c += 1;
                        } else
                        if((*c & 16) == 0 ) {
                            arg = ((*c & 0xF)<<12)|((*(c+1) & 0x3F)<<6)|(*(c+2) & 0x3F);
                            c += 2;
                        } else
                        if((*c & 8) == 0 ) {
                            arg = ((*c & 0x7)<<18)|((*(c+1) & 0x3F)<<12)|((*(c+2) & 0x3F)<<6)|(*(c+3) & 0x3F);
                            c += 3;
                        } else
                            arg = L'?';
                    }
                    if(!arg) break;
                    if(*fmt==L'Q' && needsescape(arg)) {
                        *dst++ = L'\\';
                        switch(arg) {
                            case L'\a': *dst++ = L'a'; break;
                            case L'\b': *dst++ = L'b'; break;
                            case L'\e': *dst++ = L'e'; break;
                            case L'\f': *dst++ = L'f'; break;
                            case L'\n': *dst++ = L'n'; break;
                            case L'\r': *dst++ = L'r'; break;
                            case L'\t': *dst++ = L't'; break;
                            case L'\v': *dst++ = L'v'; break;
                            default: *dst++ = arg; break;
                        }
                    } else
                        *dst++ = (wchar_t)(arg & 0xffff);
                }
            }
        } else {
put:        *dst++ = *fmt;
        }
        fmt++;
    }
    *dst=0;
    return dst-orig;
#undef needsescape
}

int vsprintf(wchar_t *dst, const wchar_t *fmt, __builtin_va_list args)
{
    return vsnprintf(dst, BUFSIZ, fmt, args);
}

int sprintf(wchar_t *dst, const wchar_t* fmt, ...)
{
    __builtin_va_list args;
    __builtin_va_start(args, fmt);
    return vsnprintf(dst, BUFSIZ, fmt, args);
}

int snprintf(wchar_t *dst, size_t maxlen, const wchar_t* fmt, ...)
{
    __builtin_va_list args;
    __builtin_va_start(args, fmt);
    return vsnprintf(dst, maxlen, fmt, args);
}

int vprintf(const wchar_t* fmt, __builtin_va_list args)
{
    wchar_t dst[BUFSIZ];
    int ret;
    ret = vsnprintf(dst, sizeof(dst), fmt, args);
    uefi_call_wrapper(ST->ConOut->OutputString, 2, ST->ConOut, &dst);
    return ret;
}

int printf(const wchar_t* fmt, ...)
{
    __builtin_va_list args;
    __builtin_va_start(args, fmt);
    return vprintf(fmt, args);
}

int vfprintf (FILE *__stream, const wchar_t *__format, __builtin_va_list args)
{
    wchar_t dst[BUFSIZ];
    int ret;
    if(__stream == stdin) return 0;
    ret = vsnprintf(dst, sizeof(dst), __format, args);
    if(ret < 1) return 0;
    if(__stream == stdout)
        uefi_call_wrapper(ST->ConOut->OutputString, 2, ST->ConOut, &dst);
    else if(__stream == stderr)
        uefi_call_wrapper(ST->StdErr->OutputString, 2, ST->StdErr, &dst);
    else
        uefi_call_wrapper(__stream->Write, 3, __stream, &ret, &dst);
    return ret;
}

int fprintf (FILE *__stream, const wchar_t *__format, ...)
{
    __builtin_va_list args;
    __builtin_va_start(args, __format);
    return vfprintf(__stream, __format, args);
}

int getchar (void)
{
    efi_input_key_t key;
    efi_status_t status = uefi_call_wrapper(ST->ConIn, 2, ST->ConIn, &key);
    return EFI_ERROR(status) ? -1 : key.UnicodeChar;

}

int getchar_ifany (void)
{
    efi_input_key_t key;
    efi_status_t status = uefi_call_wrapper(BS->CheckEvent, 1, ST->ConIn->WaitForKey);
    if(!status) {
        status = uefi_call_wrapper(ST->ConIn, 2, ST->ConIn, &key);
        return EFI_ERROR(status) ? -1 : key.UnicodeChar;
    }
    return 0;
}

int putchar (int __c)
{
    wchar_t tmp[2];
    tmp[0] = (wchar_t)__c;
    tmp[1] = 0;
    uefi_call_wrapper(ST->ConOut->OutputString, 2, ST->ConOut, &tmp);
    return (int)tmp[0];
}

/* this isn't POSIX, but also uses sprintf */
void uefi_dumpmem(efi_physical_address_t address)
{
    uint8_t *mem = (uint8_t*)(address & ~0xF);
    int i, j;
    wchar_t tmp[128], *t;
    for(j = 0; j < 8; j++) {
        t = tmp + sprintf(tmp, L"%p: ");
        for(i = 0; i < 16; i++)
            t += sprintf(t, L"%02x %s", mem[i], i % 4 == 3 ? L" " : L"");
        for(i = 0; i < 16; i++)
            t += sprintf(t, L" %c", mem[i] < 32 || mem[i] >= 127 ? L'.' : mem[i]);
        uefi_call_wrapper(ST->StdErr->OutputString, 2, ST->StdErr, &tmp);
        mem += 16;
    }
}
