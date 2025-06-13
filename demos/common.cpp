#include "common.hpp"
#include <fstream>
#include <sstream>

void ExecuteScript(JSContext* ctx, const std::string& filename, int flags) {
    std::ifstream file(filename, std::ios::in | std::ios::binary);
    if (!file) {
        std::cerr << "open file main.js failed" << std::endl;
    }
    std::stringstream ss;
    ss << file.rdbuf();
    std::string content = ss.str();

    JSValue result =
        JS_Eval(ctx, content.c_str(), content.size(), nullptr, flags);

    if (JS_IsException(result)) {
        js_std_dump_error(ctx);
    }

    JS_FreeValue(ctx, result);
}

void CheckJSValue(JSContext* ctx, JSValue value) {
    if (JS_IsException(value)) {
        js_std_dump_error(ctx);
    }
}
