#include "quickjs.h"
#include "quickjs-libc.h"
#include <fstream>
#include <iostream>
#include <sstream>

int main() {
    // create quickjs runtime
    JSRuntime* runtime = JS_NewRuntime();
    if (!runtime) {
        std::cerr << "init runtime failed" << std::endl;
        return 1;
    }

    // create context
    JSContext* ctx = JS_NewContext(runtime);
    if (!ctx) {
        std::cerr << "create context failed" << std::endl;
        JS_FreeRuntime(runtime);
        return 2;
    }

    // [optional] init quickjs internal help functions(print, console.log)
    // from quickjs-libc.h
    js_std_add_helpers(ctx, 0, NULL);

    // read js source code
    std::ifstream file("demos/01-HelloWorld/main.js",
                       std::ios::in | std::ios::binary);
    if (!file) {
        std::cerr << "open file main.js failed" << std::endl;
    }
    std::stringstream ss;
    ss << file.rdbuf();
    std::string content = ss.str();

    // run script
    JSValue result = JS_Eval(ctx, content.c_str(), content.size(), nullptr,
                             JS_EVAL_FLAG_STRICT);

    // exception handle
    if (JS_IsException(result)) {
        // from quickjs-libc.hpp, to log exception
        js_std_dump_error(ctx);
    }

    // don't forget cleanup
    JS_FreeValue(ctx, result);
    JS_FreeContext(ctx);
    JS_FreeRuntime(runtime);
    return 0;
}