// Minimal DLL injector for hz3 HookBox (IAT mode).

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char* kHz3ReadyEnv = "HZ3_HOOK_IAT_READY";

static void hz3_inject_abort(const char* where, DWORD err) {
    fprintf(stderr, "[HZ3_INJECT_FAIL] %s err=%lu\n", where ? where : "unknown", (unsigned long)err);
    ExitProcess(1);
}

static int hz3_needs_quotes(const char* s) {
    for (; *s; s++) {
        if (*s == ' ' || *s == '\t') {
            return 1;
        }
        if (*s == '"') {
            return 1;
        }
    }
    return 0;
}

static size_t hz3_append_arg(char* dst, size_t pos, const char* arg) {
    int quote = hz3_needs_quotes(arg);
    if (quote) {
        dst[pos++] = '"';
    }
    for (const char* p = arg; *p; p++) {
        if (*p == '"') {
            dst[pos++] = '\\';
            dst[pos++] = '"';
            continue;
        }
        dst[pos++] = *p;
    }
    if (quote) {
        dst[pos++] = '"';
    }
    return pos;
}

static char* hz3_build_cmdline(int argc, char** argv, int start) {
    size_t len = 0;
    for (int i = start; i < argc; i++) {
        len += strlen(argv[i]) + 3;
    }
    char* cmd = (char*)malloc(len + 1);
    if (!cmd) {
        return NULL;
    }
    size_t pos = 0;
    for (int i = start; i < argc; i++) {
        if (i > start) {
            cmd[pos++] = ' ';
        }
        pos = hz3_append_arg(cmd, pos, argv[i]);
    }
    cmd[pos] = '\0';
    return cmd;
}

static void hz3_make_ready_event_name(char* buf, size_t cap) {
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);
    unsigned long long ts = ((unsigned long long)ft.dwHighDateTime << 32) | (unsigned long long)ft.dwLowDateTime;
    _snprintf_s(buf, cap, _TRUNCATE, "Global\\HZ3_IAT_READY_%lu_%llu",
                (unsigned long)GetCurrentProcessId(), ts);
}

int main(int argc, char** argv) {
    if (argc < 3) {
        fprintf(stderr, "usage: %s <dll> <exe> [args...]\n", argv[0]);
        return 1;
    }

    char dll_path[MAX_PATH];
    if (!GetFullPathNameA(argv[1], MAX_PATH, dll_path, NULL)) {
        hz3_inject_abort("dll_fullpath", GetLastError());
    }

    char exe_path[MAX_PATH];
    if (!GetFullPathNameA(argv[2], MAX_PATH, exe_path, NULL)) {
        hz3_inject_abort("exe_fullpath", GetLastError());
    }

    char* cmdline = hz3_build_cmdline(argc, argv, 2);
    if (!cmdline) {
        hz3_inject_abort("alloc_cmdline", GetLastError());
    }

    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    ZeroMemory(&pi, sizeof(pi));
    si.cb = sizeof(si);

    char ready_name[128];
    hz3_make_ready_event_name(ready_name, sizeof(ready_name));
    HANDLE ready_event = CreateEventA(NULL, TRUE, FALSE, ready_name);
    if (!ready_event) {
        free(cmdline);
        hz3_inject_abort("CreateEvent", GetLastError());
    }
    if (!SetEnvironmentVariableA(kHz3ReadyEnv, ready_name)) {
        CloseHandle(ready_event);
        free(cmdline);
        hz3_inject_abort("SetEnvironmentVariable", GetLastError());
    }

    if (!CreateProcessA(exe_path, cmdline, NULL, NULL, FALSE, CREATE_SUSPENDED, NULL, NULL, &si, &pi)) {
        SetEnvironmentVariableA(kHz3ReadyEnv, NULL);
        CloseHandle(ready_event);
        free(cmdline);
        hz3_inject_abort("CreateProcess", GetLastError());
    }
    SetEnvironmentVariableA(kHz3ReadyEnv, NULL);

    SIZE_T dll_len = strlen(dll_path) + 1;
    void* remote = VirtualAllocEx(pi.hProcess, NULL, dll_len, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    if (!remote) {
        TerminateProcess(pi.hProcess, 1);
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
        CloseHandle(ready_event);
        free(cmdline);
        hz3_inject_abort("VirtualAllocEx", GetLastError());
    }

    SIZE_T written = 0;
    if (!WriteProcessMemory(pi.hProcess, remote, dll_path, dll_len, &written) || written != dll_len) {
        TerminateProcess(pi.hProcess, 1);
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
        CloseHandle(ready_event);
        free(cmdline);
        hz3_inject_abort("WriteProcessMemory", GetLastError());
    }

    HMODULE kernel = GetModuleHandleA("kernel32.dll");
    FARPROC loadlib = kernel ? GetProcAddress(kernel, "LoadLibraryA") : NULL;
    if (!loadlib) {
        TerminateProcess(pi.hProcess, 1);
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
        CloseHandle(ready_event);
        free(cmdline);
        hz3_inject_abort("GetProcAddress(LoadLibraryA)", GetLastError());
    }

    HANDLE thread = CreateRemoteThread(pi.hProcess, NULL, 0,
                                       (LPTHREAD_START_ROUTINE)loadlib,
                                       remote, 0, NULL);
    if (!thread) {
        TerminateProcess(pi.hProcess, 1);
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
        CloseHandle(ready_event);
        free(cmdline);
        hz3_inject_abort("CreateRemoteThread", GetLastError());
    }

    WaitForSingleObject(thread, INFINITE);
    DWORD load_result = 0;
    GetExitCodeThread(thread, &load_result);
    CloseHandle(thread);
    VirtualFreeEx(pi.hProcess, remote, 0, MEM_RELEASE);

    if (load_result == 0) {
        TerminateProcess(pi.hProcess, 1);
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
        CloseHandle(ready_event);
        free(cmdline);
        hz3_inject_abort("LoadLibraryA(remote)", GetLastError());
    }

    DWORD wait_res = WaitForSingleObject(ready_event, 5000);
    if (wait_res != WAIT_OBJECT_0) {
        DWORD err = (wait_res == WAIT_FAILED) ? GetLastError() : wait_res;
        TerminateProcess(pi.hProcess, 1);
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
        CloseHandle(ready_event);
        free(cmdline);
        hz3_inject_abort("WaitForReadyEvent", err);
    }
    CloseHandle(ready_event);

    ResumeThread(pi.hThread);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    free(cmdline);
    return 0;
}
