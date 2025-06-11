#include "quickjs-libc.h"
#include "quickjs.h"
#include <fstream>
#include <iostream>
#include <sstream>

void ExecuteScript(JSContext* ctx, const std::string& filename, int flags) {
    std::ifstream file(filename, std::ios::in | std::ios::binary);
    if (!file) {
        std::cerr << "open file main.js failed" << std::endl;
    }
    std::stringstream ss;
    ss << file.rdbuf();
    std::string content = ss.str();

    JSValue result = JS_Eval(ctx, content.c_str(), content.size(), nullptr,
                             JS_EVAL_FLAG_STRICT | flags);

    if (JS_IsException(result)) {
        js_std_dump_error(ctx);
    }

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

    // add internal module support
    js_init_module_os(ctx, "os");
    js_init_module_std(ctx, "std");
    js_init_module_bjson(ctx, "json");

    /*********** Ways to load module *******************/

    /* execute script as module.
     * benefit: you can use `import * as x from 'your-module'`
     * shortcut: when has ocurrs error, it will be silent and break!
     */
    {
        std::cout << "execute in module mode" << std::endl;
        ExecuteScript(ctx, "demos/02-UsingInternalModules/as_module.js",
                      JS_EVAL_TYPE_MODULE);
    }

    /* execute script as non-module. using js async module load syntax
     * benefit: will report error manually
     * shortcut: due to async module load, you must call `js_std_loop` to handle
     * async call and use `js_std_init_handlers` to support async
     */
    {
        std::cout << "execute in non-module mode" << std::endl;
        ExecuteScript(ctx, "demos/02-UsingInternalModules/no-module.js", 0);
        js_std_loop(ctx);
    }

    /* first execute code to load module into global space, then execute script
     * as non-module
     */
    {
        std::cout << "execute in pre-module mode" << std::endl;
        const char* module_preload_code = R"(
            import * as std from 'std';
            globalThis.std = std;
        )";
        auto value =
            JS_Eval(ctx, module_preload_code, strlen(module_preload_code),
                    nullptr, JS_EVAL_TYPE_MODULE);
        JS_FreeValue(ctx, value);
        ExecuteScript(ctx, "demos/02-UsingInternalModules/pre-module.js", 0);
        js_std_loop(ctx);
    }

    ///////////////////////////////////////////////////////

    JS_FreeContext(ctx);

    // don't forget free handlers
    js_std_free_handlers(runtime);

    JS_FreeRuntime(runtime);
    return 0;
}