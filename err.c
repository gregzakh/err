#include <windows.h>
#include <winternl.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <locale.h>

#pragma comment (lib, "ntdll")

static void try_convert_error_to_msg(DWORD code) {
    HLOCAL msg = NULL;
    DWORD sz = FormatMessage(
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        code,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPWSTR)&msg,
        0,
        NULL
    );

    if (0 == sz)
        fprintf(stderr, "[!] Unknown error: cannot interpret selected code.\n");
    else
        printf("[*] %.*ws\n", (INT)(sz - sizeof(WCHAR)), (LPWSTR)msg);

    if (LocalFree(msg) != NULL)
        fprintf(stderr, "[!] Internal error: LocalFree (%d) fatal error.\n", GetLastError());
}

static void print_ntstatus(NTSTATUS nts) {
    try_convert_error_to_msg(RtlNtStatusToDosError(nts));
}

static void print_hresult(HRESULT hres) {
    DWORD facility = (hres >> 16) & 0x7ff;
    if (facility == FACILITY_WIN32) {
        try_convert_error_to_msg(hres & 0xffff);
        return;
    }

    if (hres & 0x10000000) {
        print_ntstatus(hres & ~0x10000000u);
        return;
    }

    try_convert_error_to_msg(hres);
}

typedef struct {
    LONG number;
    char suffix;
} StringParts;

static StringParts cast_error(const char *str) {
    StringParts res = {0, '\0'};
    if (!str) return res;

    size_t sz = strlen(str);
    if (sz == 0) return res;

    unsigned char suffix = str[sz - 1];
    if (isalpha(suffix) != 0) {
        suffix = (char)tolower(suffix);

        if (suffix != 'n' && suffix != 'h')
            return res;
        
        res.suffix = suffix;
        res.number = strtoul(str, NULL, 16);
        
        return res;
    }

    res.number = strtoul(str, NULL, 10);
    return res;
}

int main(int argc, char **argv) {
    setlocale(LC_CTYPE, "");
    if (2 != argc) {
        fprintf(stderr, "[!] Argumets error: index is out of range\nUsage: err <code>\n");
        return 1;
    }

    StringParts sp = cast_error(argv[1]);
    switch (sp.suffix) {
        case 'h':
            print_hresult(sp.number);
            break;
        case 'n':
            print_ntstatus(sp.number);
            break;
        default:
            try_convert_error_to_msg(sp.number);
            break;
    }

    return 0;
}