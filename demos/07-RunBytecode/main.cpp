#include "quickjs-libc.h"
#include "quickjs.h"

#include <iostream>
#include <sstream>
#include <fstream>

#include "common.hpp"

void ExecuteBinaryScript(JSContext* ctx, const std::string& filename, int flags) {
    std::ifstream file(filename, std::ios::in | std::ios::binary);
    if (!file) {
        std::cerr << "open file main.js failed" << std::endl;
    }
    std::stringstream ss;
    ss << file.rdbuf();
    std::string content = ss.str();

    JSValue obj = JS_ReadObject(ctx, (uint8_t*)content.data(), content.size(), JS_READ_OBJ_BYTECODE);
    CheckJSValue(ctx, obj);

    JSValue result = JS_EvalFunction(ctx, obj);
    CheckJSValue(ctx, result);

    JS_FreeValue(ctx, obj);
    JS_FreeValue(ctx, result);
}

int main() {
    JSRuntime* runtime = JS_NewRuntime();
    if (!runtime) {
        std::cerr << "init runtime failed" << std::endl;
        return 1;
    }

    JSContext* ctx = JS_NewContext(runtime);
    if (!ctx) {
        std::cerr << "create context failed" << std::endl;
        JS_FreeRuntime(runtime);
        return 2;
    }

    // must first add runtime handler
    js_std_init_handlers(runtime);

    js_std_add_helpers(ctx, 0, NULL);

    js_init_module_std(ctx, "std");

    ExecuteBinaryScript(ctx, "demos/07-RunBytecode/output.qjs", JS_EVAL_TYPE_MODULE);

    js_std_loop(ctx);

    JS_FreeContext(ctx);

    // don't forget free handlers
    js_std_free_handlers(runtime);

    JS_FreeRuntime(runtime);
    return 0;
}