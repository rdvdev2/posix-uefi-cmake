MYARCH = $(shell uname -m | sed s,i[3456789]86,ia32,)
ifeq ($(ARCH),)
ARCH = $(MYARCH)
endif
SRCS ?= $(wildcard *.c) $(wildcard *.S)
TMP = $(SRCS:.c=.o)
OBJS = $(TMP:.S=.o)
CFLAGS += -mno-red-zone -fpic -fshort-wchar -fno-strict-aliasing -ffreestanding -fno-stack-protector -fno-stack-check \
  -D__$(ARCH)__ -I/usr/include -I. -I./uefi -I/usr/include/efi -I/usr/include/efi/$(ARCH) -I/usr/include/efi/protocol
LDFLAGS += -nostdlib -shared -Bsymbolic

LIBSRCS = $(filter-out crt_$(ARCH).c,$(wildcard *.c)) $(wildcard *.S)
TMP = $(LIBSRCS:.c=.o)
LIBOBJS = $(TMP:.S=.o)

ifeq ($(USE_LLVM),)
CFLAGS += -Wno-builtin-declaration-mismatch
# see if we're cross-compiling
ifeq ($(ARCH),$(MYARCH))
CC = $(ARCH)-elf-gcc
LD = $(ARCH)-elf-ld
else
CC = gcc
LD = ld
endif
else
CFLAGS += --target=$(ARCH)-elf -Wno-builtin-requires-header -Wno-incompatible-library-redeclaration
LDFLAGS += -m elf_$(ARCH)
CC = clang
LD = ld.lld
endif
# you will need this from GNU binutils, llvm-objcopy won't cut it
OBJCOPY ?= objcopy
AR ?= ar

ifeq ($(wildcard uefi/Makefile),)
ALLTARGETS = crt_$(ARCH).o libuefi.a build
else
ALLTARGETS = uefi/crt_$(ARCH).o uefi/libuefi.a $(OBJS) $(TARGET)
endif

all: $(ALLTARGETS)

uefi/libuefi.a:
	@make --no-print-directory -C uefi libuefi.a

libuefi.a: $(LIBOBJS)
	@rm $@ 2>/dev/null || true
	$(AR) -frsv $@ $(LIBOBJS) >/dev/null

$(TARGET): $(TARGET).so
	@$(OBJCOPY) -j .text -j .sdata -j .data -j .dynamic -j .dynsym  -j .rel -j .rela -j .rel.* -j .rela.* -j .reloc --target efi-app-$(ARCH) --subsystem=10 $^ $@
	@rm $(TARGET).so

$(TARGET).so: uefi/crt_$(ARCH).o $(OBJS)
	$(LD) $(LDFLAGS) -Luefi $^ -o $@ $(LIBS) -luefi -T uefi/elf_$(ARCH)_efi.lds

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.S
	$(CC) $(CFLAGS) -c $< -o $@

build:
	@mkdir ../build ../build/uefi 2>/dev/null || true
	@cp crt_$(ARCH).o ../build/uefi/crt0.o
	@cp elf_$(ARCH)_efi.lds ../build/uefi/link.ld
	@cp libuefi.a uefi.h ../build/uefi

clean:
	@rm $(TARGET) *.o *.a $(LIBOBJS) 2>/dev/null || true

distclean: clean
ifeq ($(wildcard uefi/Makefile),)
	@rm -rf ../build 2>/dev/null || true
else
	@rm uefi/*.o uefi/libuefi.a 2>/dev/null || true
endif
