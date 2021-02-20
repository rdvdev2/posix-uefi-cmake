ELF loader
==========

This is an example ELF loader. Note that UEFI wastes lots and lots of memory, so be sure you link your ELF
at an address which isn't used by UEFI. The safest would be 64M, but using 32M looks ok.

Compilation
-----------

To compile the loader
```sh
$ make
```

To compile the "kernel":
```sh
$ make -f Makefile.kernel kernel.elf
```
First compile the loader, then the kernel. This is tricky because compiling the kernel includes the normal Makefile to
get gcc options and flags. Normally you should have the kernel in completely separated repository with its own Makefile.
