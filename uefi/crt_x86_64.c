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
#include <uefielf.h>

/* this is implemented by the application */
extern int main(int argc, wchar_t **argv);

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
    "	.globl _start;"
    "_start:"
    "	lea ImageBase(%rip), %rdi;"
    "	lea _DYNAMIC(%rip), %rsi;"
    "	call uefi_init;"
    "	ret;"

    /*** the following code snipplets are borrowed from gnu-efi ***/
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
    /* for uefi_call_wrapper */
    "	.globl	uefi_call0;"
    "uefi_call0:"
    "	subq $40, %rsp;"
    "	call *%rdi;"
    "	addq $40, %rsp;"
    "	ret;"
    "	.globl	uefi_call1;"
    "uefi_call1:"
    "	subq $40, %rsp;"
    "	mov  %rsi, %rcx;"
    "	call *%rdi;"
    "	addq $40, %rsp;"
    "	ret;"
    "	.globl	uefi_call2;"
    "uefi_call2:"
    "	subq $40, %rsp;"
    "	mov  %rsi, %rcx;"
    "	call *%rdi;"
    "	addq $40, %rsp;"
    "	ret;"
    "	.globl	uefi_call3;"
    "uefi_call3:"
    "	subq $40, %rsp;"
    "	mov  %rcx, %r8;"
    "	mov  %rsi, %rcx;"
    "	call *%rdi;"
    "	addq $40, %rsp;"
    "	ret;"
    "	.globl	uefi_call4;"
    "uefi_call4:"
    "	subq $40, %rsp;"
    "	mov %r8, %r9;"
    "	mov %rcx, %r8;"
    "	mov %rsi, %rcx;"
    "	call *%rdi;"
    "	addq $40, %rsp;"
    "	ret;"
    "	.globl	uefi_call5;"
    "uefi_call5:"
    "	subq $40, %rsp;"
    "	mov %r9, 32(%rsp);"
    "	mov %r8, %r9;"
    "	mov %rcx, %r8;"
    "	mov %rsi, %rcx;"
    "	call *%rdi;"
    "	addq $40, %rsp;"
    "	ret;"
    "	.globl	uefi_call6;"
    "uefi_call6:"
    "	subq $56, %rsp;"
    "	mov 56+8(%rsp), %rax;"
    "	mov %rax, 40(%rsp);"
    "	mov %r9, 32(%rsp);"
    "	mov %r8, %r9;"
    "	mov %rcx, %r8;"
    "	mov %rsi, %rcx;"
    "	call *%rdi;"
    "	addq $56, %rsp;"
    "	ret;"
    "	.globl	uefi_call7;"
    "uefi_call7:"
    "	subq $56, %rsp;"
    "	mov 56+16(%rsp), %rax;"
    "	mov %rax, 48(%rsp);"
    "	mov 56+8(%rsp), %rax;"
    "	mov %rax, 40(%rsp);"
    "	mov %r9, 32(%rsp);"
    "	mov %r8, %r9;"
    "	mov %rcx, %r8;"
    "	mov %rsi, %rcx;"
    "	call *%rdi;"
    "	addq $56, %rsp;"
    "	ret;"
    "	.globl	uefi_call8;"
    "uefi_call8:"
    "	subq $72, %rsp;"
    "	mov 72+24(%rsp), %rax;"
    "	mov %rax, 56(%rsp);"
    "	mov 72+16(%rsp), %rax;"
    "	mov %rax, 48(%rsp);"
    "	mov 72+8(%rsp), %rax;"
    "	mov %rax, 40(%rsp);"
    "	mov %r9, 32(%rsp);"
    "	mov %r8, %r9;"
    "	mov %rcx, %r8;"
    "	mov %rsi, %rcx;"
    "	call *%rdi;"
    "	addq $72, %rsp;"
    "	ret;"
    "	.globl	uefi_call9;"
    "uefi_call9:"
    "	subq $72, %rsp;"
    "	mov 72+32(%rsp), %rax;"
    "	mov %rax, 64(%rsp);"
    "	mov 72+24(%rsp), %rax;"
    "	mov %rax, 56(%rsp);"
    "	mov 72+16(%rsp), %rax;"
    "	mov %rax, 48(%rsp);"
    "	mov 72+8(%rsp), %rax;"
    "	mov %rax, 40(%rsp);"
    "	mov %r9, 32(%rsp);"
    "	mov %r8, %r9;"
    "	mov %rcx, %r8;"
    "	mov %rsi, %rcx;"
    "	call *%rdi;"
    "	addq $72, %rsp;"
    "	ret;"
    "	.globl	uefi_call10;"
    "uefi_call10:"
    "	subq $88, %rsp;"
    "	mov 88+40(%rsp), %rax;"
    "	mov %rax, 72(%rsp);"
    "	mov 88+32(%rsp), %rax;"
    "	mov %rax, 64(%rsp);"
    "	mov 88+24(%rsp), %rax;"
    "	mov %rax, 56(%rsp);"
    "	mov 88+16(%rsp), %rax;"
    "	mov %rax, 48(%rsp);"
    "	mov 88+8(%rsp), %rax;"
    "	mov %rax, 40(%rsp);"
    "	mov %r9, 32(%rsp);"
    "	mov %r8, %r9;"
    "	mov %rcx, %r8;"
    "	mov %rsi, %rcx;"
    "	call *%rdi;"
    "	addq $88, %rsp;"
    "	ret;"
    );
}

/**
 * Initialize POSIX-UEFI and call teh application's main() function
 */
int uefi_init (efi_handle_t image, efi_system_table_t *systab, uintptr_t ldbase, Elf64_Dyn *dyn) {
    long relsz = 0, relent = 0;
    Elf64_Rel *rel = 0;
    uintptr_t *addr;
    efi_guid_t shpGuid = EFI_SHELL_PARAMETERS_PROTOCOL_GUID;
    efi_shell_parameters_protocol_t *shp = NULL;
    efi_guid_t shiGuid = SHELL_INTERFACE_PROTOCOL_GUID;
    efi_shell_interface_protocol_t *shi = NULL;
    efi_guid_t lipGuid = EFI_LOADED_IMAGE_PROTOCOL_GUID;
    efi_status_t status;
    int i, argc = 0;
    wchar_t **argv = NULL;
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
    /* save EFI pointers and loaded image into globals */
    IM = image;
    ST = systab;
    BS = systab->BootServices;
    RT = systab->RuntimeServices;
    uefi_call_wrapper(BS->HandleProtocol, 3, image, &lipGuid, (void **) &LIP);
    /* get command line arguments */
    status = uefi_call_wrapper(BS->OpenProtocol, 6, image, &shpGuid, &shp, image, NULL, EFI_OPEN_PROTOCOL_GET_PROTOCOL);
    if(!EFI_ERROR(status) && shp) { argc = shp->Argc; argv = shp->Argv; }
    else {
        /* if shell 2.0 failed, fallback to shell 1.0 interface */
        status = uefi_call_wrapper(BS->OpenProtocol, 6, image, &shiGuid, &shi, image, NULL, EFI_OPEN_PROTOCOL_GET_PROTOCOL);
        if(!EFI_ERROR(status) && shi) { argc = shi->Argc; argv = shi->Argv; }
    }

    /* call main */
    return main(argc, argv);
}

/**
 * Exit Boot Services
 */
boolean_t uefi_exit_bs()
{
    efi_status_t status;
    efi_memory_descriptor_t *memory_map = NULL;
    uintn_t cnt = 3, memory_map_size=0, map_key=0, desc_size=0;

    while(cnt--) {
        status = uefi_call_wrapper(BS->GetMemoryMap, 5, &memory_map_size, memory_map, &map_key, &desc_size, NULL);
        if (status!=EFI_BUFFER_TOO_SMALL) break;
        status = uefi_call_wrapper(BS->ExitBootServices, 2, IM, map_key);
        if(!EFI_ERROR(status)) return 1;
    }
    return 0;
}
