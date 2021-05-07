#pragma once

#include <stdio.h>
#include <stdlib.h>
#include "ubsan_handlers.h"
// libc backtrace
#include <execinfo.h>

using namespace __sanitizer;
using namespace __ubsan;
namespace mycode {
    void attest(CFICheckFailData *Data);
}
