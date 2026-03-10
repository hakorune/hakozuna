#pragma once

#include <sys/socket.h>

#ifndef UNIX_PATH_MAX
#define UNIX_PATH_MAX 108
#endif

struct sockaddr_un {
    ADDRESS_FAMILY sun_family;
    char sun_path[UNIX_PATH_MAX];
};
