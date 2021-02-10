# define which architecture you're targeting
ARCH = x86_64
# define your target file here
TARGET = helloworld.efi
#define your sources here
SRCS = $(wildcard *.c)
# define your default compiler flags
CFLAGS = -pedantic -Wall -Wextra -Werror --ansi -O2
# define your default linker flags
#LDFLAGS =
# define your additional libraries here
#LIBS = -lm

# leave the hard work and all the rest to posix-uefi

# set this if you want LLVM Clang / Lld instead of GNU gcc / ld
#USE_LLVM = 1
include uefi/Makefile
