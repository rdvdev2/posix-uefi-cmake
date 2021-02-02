#include <uefi.h>

/**
 * Print out arguments
 */
int main(int argc, wchar_t **argv)
{
    int i;

    printf(L"I got %d argument%s:\n", argc, argc > 1 ? L"s" : L"");
    for(i = 0; i < argc; i++)
        printf(L"  argv[%d] = '%s'\n", i, argv[i]);

    return 0;
}
