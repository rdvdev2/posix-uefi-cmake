#include <uefi.h>

/* this should accept 0x prefixes from the command line */
int main(int argc, wchar_t **argv)
{
    efi_physical_address_t address = (argc < 2 ? (efi_physical_address_t)IM : (efi_physical_address_t)atol(argv[1]));
    uefi_dumpmem(address);
    return 0;
}
