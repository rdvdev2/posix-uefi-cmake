MYARCH = $(shell uname -m | sed s,i[3456789]86,ia32,)
ifeq ($(ARCH),)
ARCH = $(MYARCH)
endif
SRCS ?= $(wildcard *.c) $(wildcard *.S)
TMP = $(SRCS:.c=.o)
OBJS = $(TMP:.S=.o)
CFLAGS += -mno-red-zone -fshort-wchar -fno-strict-aliasing -ffreestanding -fno-stack-protector -fno-stack-check \
  -D__$(ARCH)__ -DHAVE_USE_MS_ABI \
  -I/usr/include -I. -I./uefi -I/usr/include/efi -I/usr/include/efi/$(ARCH) -I/usr/include/efi/protocol

LIBSRCS = $(filter-out crt_$(ARCH).c,$(wildcard *.c)) $(wildcard *.S)
TMP = $(LIBSRCS:.c=.o)
LIBOBJS = $(TMP:.S=.o)

ifeq ($(USE_LLVM),)
CFLAGS += -Wno-builtin-declaration-mismatch -fpic -maccumulate-outgoing-args
LDFLAGS += -nostdlib -shared -Bsymbolic -Luefi uefi/crt_$(ARCH).o
LIBS += -o $(TARGET).so -luefi -T uefi/elf_$(ARCH)_efi.lds
# see if we're cross-compiling
ifeq ($(ARCH),$(MYARCH))
CC = $(ARCH)-elf-gcc
LD = $(ARCH)-elf-ld
else
CC = gcc
LD = ld
endif
OBJCOPY ?= objcopy
AR ?= ar
else
CFLAGS += --target=$(ARCH)-pc-win32-coff -Wno-builtin-requires-header -Wno-incompatible-library-redeclaration
LDFLAGS += -subsystem:efi_application -nodefaultlib -dll -entry:uefi_init uefi/*.o
LIBS = -out:$(TARGET)
CC = clang
LD = lld-link
OBJCOPY = true
endif

ifeq ($(wildcard uefi/Makefile),)
ALLTARGETS = crt_$(ARCH).o libuefi.a build
else
ALLTARGETS = uefi/crt_$(ARCH).o uefi/libuefi.a $(OBJS) $(TARGET)
endif

all: $(ALLTARGETS)

uefi/libuefi.a:
	make --no-print-directory -C uefi libuefi.a USE_LLVM=$(USE_LLVM)

libuefi.lib: $(LIBOBJS)

libuefi.a: $(LIBOBJS)
ifeq ($(USE_LLVM),)
	@rm $@ 2>/dev/null || true
	$(AR) -frsv $@ $(LIBOBJS) >/dev/null
else
	true
endif

$(TARGET): $(TARGET).so
ifeq ($(USE_LLVM),)
	@$(OBJCOPY) -j .text -j .sdata -j .data -j .dynamic -j .dynsym  -j .rel -j .rela -j .rel.* -j .rela.* -j .reloc --target efi-app-$(ARCH) --subsystem=10 $^ $@
	@rm $(TARGET).so
endif

$(TARGET).so: $(OBJS)
	$(LD) $(LDFLAGS) $^ $(LIBS)
	@rm *.lib 2>/dev/null || true

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
	@rm $(TARGET) *.o *.a *.lib $(LIBOBJS) 2>/dev/null || true

distclean: clean
ifeq ($(wildcard uefi/Makefile),)
	@rm -rf ../build 2>/dev/null || true
else
	@rm uefi/*.o uefi/libuefi.a 2>/dev/null || true
endif
