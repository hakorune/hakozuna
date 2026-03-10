#pragma once

#if !defined(_WIN32)
#error "utsname shim is Windows-only"
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <stdio.h>
#include <string.h>

struct utsname {
    char sysname[65];
    char nodename[65];
    char release[65];
    char version[65];
    char machine[65];
};

static inline int uname(struct utsname *buf) {
    if (!buf) {
        return -1;
    }

    ZeroMemory(buf, sizeof(*buf));
    strcpy_s(buf->sysname, sizeof(buf->sysname), "Windows");
    strcpy_s(buf->nodename, sizeof(buf->nodename), "localhost");
    strcpy_s(buf->machine, sizeof(buf->machine), sizeof(void*) == 8 ? "x86_64" : "x86");

    OSVERSIONINFOEXA vi;
    ZeroMemory(&vi, sizeof(vi));
    vi.dwOSVersionInfoSize = sizeof(vi);
    if (GetVersionExA((OSVERSIONINFOA *)&vi)) {
        snprintf(buf->release, sizeof(buf->release), "%lu.%lu", vi.dwMajorVersion, vi.dwMinorVersion);
        snprintf(buf->version, sizeof(buf->version), "%lu", vi.dwBuildNumber);
    } else {
        strcpy_s(buf->release, sizeof(buf->release), "unknown");
        strcpy_s(buf->version, sizeof(buf->version), "unknown");
    }
    return 0;
}
