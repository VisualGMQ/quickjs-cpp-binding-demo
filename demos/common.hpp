#pragma once

#include "quickjs-libc.h"
#include "quickjs.h"
#include <string>
#include <iostream>


// most quickjs return -1 as error & 0 and success
#define QJS_CALL(expr)                                                     \
    do {                                                                   \
        if ((expr) < 0) {                                                  \
            std::cerr << "QJS error when execute: " << #expr << std::endl; \
        }                                                                  \
    } while (0)

void ExecuteScript(JSContext* ctx, const std::string& filename, int flags);

void CheckJSValue(JSContext* ctx, JSValueConst value);
