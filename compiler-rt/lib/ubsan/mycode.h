#pragma once

#include <stdio.h>
#include <stdlib.h>
#include "ubsan_handlers.h"
// libc backtrace
#include <execinfo.h>
// fork, getpid()
#include <unistd.h>
#include <signal.h>
// inotify
#include <errno.h>
#include <poll.h>
#include <sys/inotify.h>
#include <string.h>
#include <limits.h>

using namespace __sanitizer;
using namespace __ubsan;


namespace mycode {
    void attest(CFICheckFailData *Data);
}
