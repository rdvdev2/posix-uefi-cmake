#include <uefi.h>

int main(int argc, wchar_t **argv)
{
    int i;
    printf(L"I got %d arguments:\n", argc);
    for(i = 0; i < argc + 1; i++)
        printf(L"  argv[%d] = '%s'\n", argv[i]);
    return 0;
}
