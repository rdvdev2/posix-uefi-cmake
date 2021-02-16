/*
 * stat.c
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
 * @brief Implementing functions which are defined in sys/stat.h
 *
 */

#include <uefi.h>

extern void __stdio_seterrno(efi_status_t status);
extern time_t __mktime_efi(efi_time_t *t);

int fstat (FILE *__f, struct stat *__buf)
{
    uint64_t off = 0;
    efi_guid_t infGuid = EFI_FILE_INFO_GUID;
    efi_file_info_t info;
    uintn_t fsiz = (uintn_t)sizeof(efi_file_info_t);
    efi_status_t status;

    if(!__f || !__buf) {
        errno = EINVAL;
        return -1;
    }
    memset(__buf, 0, sizeof(struct stat));
    if(__f == stdin) {
        __buf->st_mode = S_IREAD | S_IFIFO;
        return 0;
    }
    if(__f == stdout || __f == stderr) {
        __buf->st_mode = S_IWRITE | S_IFIFO;
        return 0;
    }
    status = __f->GetInfo(__f, &infGuid, &fsiz, &info);
    if(EFI_ERROR(status)) {
        __stdio_seterrno(status);
        return -1;
    }
    __buf->st_mode = S_IREAD |
        (info.Attribute & EFI_FILE_READ_ONLY ? 0 : S_IWRITE) |
        (info.Attribute & EFI_FILE_DIRECTORY ? S_IFDIR : S_IFREG);
    __buf->st_size = (off_t)info.FileSize;
    __buf->st_blocks = (blkcnt_t)info.PhysicalSize;
    __buf->st_atime = __mktime_efi(&info.LastAccessTime);
    __buf->st_mtime = __mktime_efi(&info.ModificationTime);
    __buf->st_ctime = __mktime_efi(&info.CreateTime);
    return 0;
}

int stat (const char_t *__file, struct stat *__buf)
{
    int ret;
    FILE *f;

    if(!__file || !*__file || !__buf) {
        errno = EINVAL;
        return -1;
    }
    f = fopen(__file, CL("*"));
    if(!f) {
        memset(__buf, 0, sizeof(struct stat));
        return -1;
    }
    ret = fstat(f, __buf);
    fclose(f);
    return ret;
}

extern int mkdir (const char_t *__path, mode_t __mode)
{
    FILE *f;
    (void)__mode;
    if(!__path || !*__path) {
        errno = EINVAL;
        return -1;
    }
    f = fopen(__path, CL("wd"));
    if(!f) {
        return -1;
    }
    fclose(f);
    return 0;
}
