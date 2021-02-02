POSIX-UEFI
==========

```
We hate that horrible and ugly UEFI API, we want POSIX!
```

This is a very small build environment that helps you to develop for UEFI under Linux (and other POSIX systems). It was
greatly inspired by [gnu-efi](https://sourceforge.net/projects/gnu-efi) (big big kudos to those guys), but it is a lot
smaller, easier to integrate (works with Clang and GNU gcc both) and easier to use because it provides a POSIX like API.

You have two options on how to integrate it into your project:

Distributing as Static Library
------------------------------

In the `uefi` directory, run
```sh
$ make
```
This will create `build/uefi` with all the necessary files in it. These are:

 - **crt0.o**, the run-time that bootstraps POSIX-UEFI
 - **link.ld**, the linker script you must use with POSIX-UEFI (same as with gnu-efi)
 - **libuefi.a**, the library itself
 - **uefi.h**, the all-in-one C / C++ header

You can use this and link your application with it, but you won't be able to recompile it, plus you're on your own with
the linking and converting.

Strictly speaking you'll only need **crt0.o** and **link.ld**, that will get you started and will call your application's
"main()", but to get libc functions like memcmp, strcpy, malloc or fopen, you'll have to link with **libuefi.a**.

Distributing as Source
----------------------

This is the preferred way, as it also provides a Makefile to set up your toolchain properly.

 1. simply copy the `uefi` directory into your source tree (or set up a git submodule). A dozen files, about 96K in total.
 2. create an extremely simple **Makefile** like the one below
 3. compile your code for UEFI by running `make`

```
TARGET = helloworld.efi

include uefi/Makefile
```
An example **helloworld.c** goes like this:
```c
#include <uefi.h>

int main(int argc, wchar_t **argv)
{
    printf(L"Hello World!\n");
    return 0;
}
```
By default it uses the host native's GNU gcc + ld, creates a shared object and converts that into .efi file. If `USE_LLVM`
is given, then LLVM CLang + lld used, and native PE is generated, no conversion involved.

### Available Makefile Options

| Variable   | Description                                                                                          |
|------------|------------------------------------------------------------------------------------------------------|
| `TARGET`   | the target application (required)                                                                    |
| `SRCS`     | list of source files you want to compile (defaults to \*.c \*.S)                                     |
| `CFLAGS`   | compiler flags you want to use (empty by default, like "-Wall -pedantic -std=c99")                   |
| `LDFLAGS`  | linker flags you want to use (I don't think you'll ever need this, just in case)                     |
| `LIBS`     | additional libraries you want to link with (like "-lm", only static .a libraries allowed)            |
| `USE_LLVM` | set this if you want LLVM Clang + Lld instead of GNU gcc + ld                                        |
| `ARCH`     | the target architecture (only x86_64 supported for now, but the toolchain can handle multiple archs) |

Here's a more advanced **Makefile** example:
```
ARCH = x86_64
TARGET = helloworld.efi
SRCS = $(wildcard *.c)
CFLAGS = -pedantic -Wall -Wextra -Werror --std=c11 -O2
LDFLAGS =
LIBS = -lm

USE_LLVM = 1
include uefi/Makefile
```

Accessing UEFI Services
-----------------------

It is very likely that you want to call UEFI specific functions directly. For that, POSIX-UEFI specifies some globals
in `uefi.h`:

| Global Variable | Description                                              |
|-----------------|----------------------------------------------------------|
| `*BS`           | *efi_boot_services_t*, pointer to the Boot Time Services |
| `*RT`           | *efi_runtime_t*, pointer to the Runtime Services         |
| `*ST`           | *efi_system_table_t*, pointer to the UEFI System Table   |
| `IM`            | *efi_handle_t* of your Loaded Image                      |

The EFI structures, enums, typedefs and defines are all converted to ANSI C standard POSIX style, for example
BOOLEAN ->  boolean_t, UINTN -> uintn_t, EFI_MEMORY_DESCRIPTOR -> efi_memory_descriptor_t, and of course
EFI_BOOT_SERVICES -> efi_boot_services_t.

Calling UEFI functions is as simple as with EDK II, just do the call, no need for "uefi_call_wrapper":
```c
    ST->ConOut->OutputString(ST->ConOut, L"Hello World!\r\n");
```

There are two additional, non-POSIX calls in the library. One is `exit_bs()` to exit Boot Services, and the other is
a non-blocking `getchar_ifany()`.

Unlike gnu-efi, POSIX-UEFI does not pollute your application's namespace with unused GUID variables. It only provides
header definitions, so you must create each GUID instance if and when you need them.

Example:
```c
efi_guid_t gopGuid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
efi_gop_t *gop = NULL;

status = BS->LocateProtocol(&gopGuid, NULL, (void**)&gop);
```

Also unlike gnu-efi, POSIX-UEFI does not provide standard EFI headers. It expects that you have installed those under
/usr/include/efi from EDK II or gnu-efi, and POSIX-UEFI makes it possible to use those system wide headers without
naming conflicts. POSIX-UEFI itself ships the very minimum set of typedefs and structs (with POSIX-ized names).
```c
#include <efi.h>
#include <uefi.h> /* this will work as expected! Both POSIX-UEFI and EDK II / gnu-efi typedefs available */
```

Notable Differences to POSIX libc
---------------------------------

This library is nowhere near as complete as glibc or musl for example. It only provides the very basic libc functions
for you, because simplicity was one of its main goals. It is the best to say this is just wrapper around the UEFI API,
rather than a POSIX compatible libc.

All strings in the UEFI environment are stored with 16 bits wide characters. The library provides `wchar_t` type for that,
so for example your main() is NOT like `main(int argc, char **argv)`, but `main(int argc, wchar_t **argv)` instead. All
the other string related libc functions (like strlen() for example) use this wide character type too. Functions that supposed
to handle characters in int type (like `getchar`, `putchar`), do not truncate to unsigned char, rather to wchar_t. For this
reason, you must specify your string literals with `L""` and characters with `L''`. There's an additional `getchar_ifany`
function, which does not block, but returns 0 when there's no key pressed.

That's about it, everything else is the same.

List of Provided POSIX Functions
--------------------------------

### stdlib.h

| Function      | Description                                                                |
|---------------|----------------------------------------------------------------------------|
| atoi          | as usual, but accepts wide char strings and understands "0x" prefix        |
| atol          | as usual, but accepts wide char strings and understands "0x" prefix        |
| strtol        | as usual, but accepts wide char strings                                    |
| malloc        | as usual                                                                   |
| calloc        | as usual                                                                   |
| realloc       | as usual (needs testing)                                                   |
| free          | as usual                                                                   |
| abort         | as usual                                                                   |
| exit          | as usual                                                                   |
| exit_bs       | leave this entire UEFI bullshit behind (exit Boot Services)                |
| mbtowc        | as usual (UTF-8 char to wchar_t)                                           |
| wctomb        | as usual (wchar_t to UTF-8 char)                                           |
| mbstowcs      | as usual (UTF-8 string to wchar_t string)                                  |
| wcstombs      | as usual (wchar_t string to UTF-8 string)                                  |

### stdio.h

| Function      | Description                                                                |
|---------------|----------------------------------------------------------------------------|
| fopen         | as usual, but accepts wide char strings, for mode L"r", L"w" and L"a" only |
| fclose        | as usual                                                                   |
| fflush        | as usual                                                                   |
| fread         | as usual, only real files accepted (no stdin)                              |
| fwrite        | as usual, only real files accepted (no stdout nor stderr)                  |
| fseek         | as usual, only real files accepted (no stdin, stdout, stderr)              |
| ftell         | as usual, only real files accepted (no stdin, stdout, stderr)              |
| fprintf       | as usual, but accepts wide char strings, max BUFSIZ, files, stdout, stderr |
| printf        | as usual, but accepts wide char strings, max BUFSIZ, stdout only           |
| sprintf       | as usual, but accepts wide char strings, max BUFSIZ                        |
| vfprintf      | as usual, but accepts wide char strings, max BUFSIZ, files, stdout, stderr |
| vprintf       | as usual, but accepts wide char strings, max BUFSIZ                        |
| vsprintf      | as usual, but accepts wide char strings, max BUFSIZ                        |
| snprintf      | as usual, but accepts wide char strings                                    |
| vsnprintf     | as usual, but accepts wide char strings                                    |
| getchar       | as usual, waits for a key, blocking, stdin only (no redirects)             |
| getchar_ifany | non-blocking, returns 0 if there was no key press, UNICODE otherwise       |
| putchar       | as usual, stdout only (no redirects)                                       |

String formating is limited; only supports padding via number prefixes, `%d`, `%x`, `%X`, `%c`, `%s`, `%q` and
`%p`. Because it operates on wchar_t, it also supports the non-standard `%C` (printing an UTF-8 character, needs
char\*), `%S` (printing an UTF-8 string), `%Q` (printing an escaped UTF-8 string). These functions don't allocate
memory, but in return the total length of the output string cannot be longer than BUFSIZ (8k if you haven't
defined otherwise), except for the variants which have a maxlen argument. For convenience, `%D` requires
`efi_physical_address_t` as argument, and it dumps memory, 16 bytes or one line at once. With the padding
modifier you can dump more lines, for example `%5D`.

### string.h

| Function      | Description                                                                |
|---------------|----------------------------------------------------------------------------|
| memcpy        | as usual, works on bytes                                                   |
| memmove       | as usual, works on bytes                                                   |
| memset        | as usual, works on bytes                                                   |
| memcmp        | as usual, works on bytes                                                   |
| memchr        | as usual, works on bytes                                                   |
| memrchr       | as usual, works on bytes                                                   |
| memmem        | as usual, works on bytes                                                   |
| memrmem       | as usual, works on bytes                                                   |
| strcpy        | works on wide char strings                                                 |
| strncpy       | works on wide char strings                                                 |
| strcat        | works on wide char strings                                                 |
| strncat       | works on wide char strings                                                 |
| strcmp        | works on wide char strings                                                 |
| strncmp       | works on wide char strings                                                 |
| strdup        | works on wide char strings                                                 |
| strchr        | works on wide char strings                                                 |
| strrchr       | works on wide char strings                                                 |
| strstr        | works on wide char strings                                                 |
| strtok        | works on wide char strings                                                 |
| strtok_r      | works on wide char strings                                                 |
| strlen        | works on wide char strings                                                 |

### time.h

| Function      | Description                                                                |
|---------------|----------------------------------------------------------------------------|
| localtime     | argument unused, always returns current time in struct tm                  |

### unistd.h

| Function      | Description                                                                |
|---------------|----------------------------------------------------------------------------|
| usleep        | the usual                                                                  |
| sleep         | the usual                                                                  |
