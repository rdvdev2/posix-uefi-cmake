/*
 * crt_x86_64.c
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
 * @brief C runtime, bootstraps an EFI application to call standard main()
 *
 */

#include <uefi.h>

/* this is implemented by the application */
extern int main(int argc, wchar_t **argv);

/* definitions for elf relocations */
typedef uint64_t Elf64_Xword;
typedef	int64_t  Elf64_Sxword;
typedef uint64_t Elf64_Addr;
typedef struct
{
  Elf64_Sxword  d_tag;              /* Dynamic entry type */
  union
    {
      Elf64_Xword d_val;            /* Integer value */
      Elf64_Addr d_ptr;             /* Address value */
    } d_un;
} Elf64_Dyn;
#define DT_NULL             0       /* Marks end of dynamic section */
#define DT_RELA             7       /* Address of Rela relocs */
#define DT_RELASZ           8       /* Total size of Rela relocs */
#define DT_RELAENT          9       /* Size of one Rela reloc */
typedef struct
{
  Elf64_Addr    r_offset;           /* Address */
  Elf64_Xword   r_info;             /* Relocation type and symbol index */
} Elf64_Rel;
#define ELF64_R_TYPE(i)     ((i) & 0xffffffff)
#define R_X86_64_RELATIVE   8       /* Adjust by program base */

/* globals to store system table pointers */
efi_handle_t IM = NULL;
efi_system_table_t *ST = NULL;
efi_boot_services_t *BS = NULL;
efi_runtime_services_t *RT = NULL;
efi_loaded_image_protocol_t *LIP = NULL;

/* we only need one .o file, so use inline Assembly here */
void bootstrap()
{
    __asm__ __volatile__ (
    /* call init in C */
    "	.align 4;"
#ifndef __clang__
    "	.globl _start;"
    "_start:"
    "	lea ImageBase(%rip), %rdi;"
    "	lea _DYNAMIC(%rip), %rsi;"
    "	call uefi_init;"
    "	ret;"

    /* fake a relocation record, so that EFI won't complain */
    " 	.data;"
    "dummy:	.long	0;"
    " 	.section .reloc, \"a\";"
    "label1:;"
    "	.long	dummy-label1;"
    " 	.long	10;"
    "    .word 0;"
    ".text;"
#else
    "	.globl __chkstk;"
    "__chkstk:"
    "	ret;"
#endif
    /* setjmp and longjmp */
    "	.globl	setjmp;"
    "setjmp:"
    "	pop	%rsi;"
    "	movq	%rbx,0x00(%rdi);"
    "	movq	%rsp,0x08(%rdi);"
    "	push	%rsi;"
    "	movq	%rbp,0x10(%rdi);"
    "	movq	%r12,0x18(%rdi);"
    "	movq	%r13,0x20(%rdi);"
    "	movq	%r14,0x28(%rdi);"
    "	movq	%r15,0x30(%rdi);"
    "	movq	%rsi,0x38(%rdi);"
    "	xor	%rax,%rax;"
    "	ret;"
    "	.globl	longjmp;"
    "longjmp:"
    "	movl	%esi, %eax;"
    "	movq	0x00(%rdi), %rbx;"
    "	movq	0x08(%rdi), %rsp;"
    "	movq	0x10(%rdi), %rbp;"
    "	movq	0x18(%rdi), %r12;"
    "	movq	0x20(%rdi), %r13;"
    "	movq	0x28(%rdi), %r14;"
    "	movq	0x30(%rdi), %r15;"
    "	xor	%rdx,%rdx;"
    "	mov	$1,%rcx;"
    "	cmp	%rax,%rdx;"
    "	cmove	%rcx,%rax;"
    "	jmp	*0x38(%rdi);"
    );
}

/**
 * Initialize POSIX-UEFI and call the application's main() function
 */
int uefi_init (
#ifndef __clang__
    uintptr_t ldbase, Elf64_Dyn *dyn, efi_system_table_t *systab, efi_handle_t image
#else
    efi_handle_t image, efi_system_table_t *systab
#endif
) {
    efi_guid_t shpGuid = EFI_SHELL_PARAMETERS_PROTOCOL_GUID;
    efi_shell_parameters_protocol_t *shp = NULL;
    efi_guid_t shiGuid = SHELL_INTERFACE_PROTOCOL_GUID;
    efi_shell_interface_protocol_t *shi = NULL;
    efi_guid_t lipGuid = EFI_LOADED_IMAGE_PROTOCOL_GUID;
    efi_status_t status;
    int argc = 0;
    wchar_t **argv = NULL;
#ifndef __clang__
    int i;
    long relsz = 0, relent = 0;
    Elf64_Rel *rel = 0;
    uintptr_t *addr;
    /* handle relocations */
    for (i = 0; dyn[i].d_tag != DT_NULL; ++i) {
        switch (dyn[i].d_tag) {
            case DT_RELA: rel = (Elf64_Rel*)((unsigned long)dyn[i].d_un.d_ptr + ldbase); break;
            case DT_RELASZ: relsz = dyn[i].d_un.d_val; break;
            case DT_RELAENT: relent = dyn[i].d_un.d_val; break;
            default: break;
        }
    }
    if (rel && relent) {
        while (relsz > 0) {
            if(ELF64_R_TYPE (rel->r_info) == R_X86_64_RELATIVE)
                { addr = (unsigned long *)(ldbase + rel->r_offset); *addr += ldbase; break; }
            rel = (Elf64_Rel*) ((char *) rel + relent);
            relsz -= relent;
        }
    }
#endif
    /* save EFI pointers and loaded image into globals */
    IM = image;
    ST = systab;
    BS = systab->BootServices;
    RT = systab->RuntimeServices;
    BS->HandleProtocol(image, &lipGuid, (void **)&LIP);
    /* get command line arguments */
    status = BS->OpenProtocol(image, &shpGuid, (void **)&shp, image, NULL, EFI_OPEN_PROTOCOL_GET_PROTOCOL);
    if(!EFI_ERROR(status) && shp) { argc = shp->Argc; argv = shp->Argv; }
    else {
        /* if shell 2.0 failed, fallback to shell 1.0 interface */
        status = BS->OpenProtocol(image, &shiGuid, (void **)&shi, image, NULL, EFI_OPEN_PROTOCOL_GET_PROTOCOL);
        if(!EFI_ERROR(status) && shi) { argc = shi->Argc; argv = shi->Argv; }
    }
    /* call main */
    return main(argc, argv);
}
