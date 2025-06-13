#include "quickjs-libc.h"
#include "quickjs.h"

#include <fstream>
#include <iostream>
#include <sstream>

// most quickjs return -1 as error & 0 and success
#define QJS_CALL(expr)                                                     \
    do {                                                                   \
        if ((expr) < 0)                                                    \
            std::cerr << "QJS error when execute: " << #expr << std::endl; \
    } while (0)

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

void CheckJSValue(JSContext* ctx, JSValueConst value) {
    if (JS_IsException(value)) {
        js_std_dump_error(ctx);
    }
}

int Add(int a, int b) {
    return a + b; 
}

JSValue AddFnBinding(JSContext* ctx, JSValue self, int argc,
                     JSValueConst* argv) {
    if (argc != 2) {
        return JS_ThrowPlainError(ctx, "Add function must has two parameters");
    }

    JSValueConst param1 = argv[0];
    JSValueConst param2 = argv[1];
    if (!JS_IsNumber(param1) || !JS_IsNumber(param2)) {
        return JS_ThrowTypeError(ctx, "Add accept two integral");
    }

    int32_t value1, value2;
    QJS_CALL(JS_ToInt32(ctx, &value1, param1));
    QJS_CALL(JS_ToInt32(ctx, &value2, param2));

    return JS_NewInt32(ctx, Add(value1, value2));
}

void Bind(JSContext* ctx) {
    JSValue global_this = JS_GetGlobalObject(ctx);

    constexpr int FnParamCount = 2;
    JSValue fn = JS_NewCFunction(ctx, AddFnBinding, "Add", FnParamCount);
    CheckJSValue(ctx, fn);

    // also can use JS_DefinePropertyXXX
    QJS_CALL(JS_SetPropertyStr(ctx, global_this, "Add", fn));

    // don't forget cleanup
    JS_FreeValue(ctx, global_this);
}

void MagicFn1() {
    std::cout << "I'm magicFn1" << std::endl;
}

void MagicFn2() {
    std::cout << "I'm magicFn2" << std::endl;
}

JSValue BindMagicFn(JSContext* ctx, JSValue, int argc, JSValueConst* argv, int magic) {
    if (magic == 0) {
        MagicFn1();
    } else if (magic == 1) {
        MagicFn2();
    }

    return JS_UNDEFINED;
}

void BindMagic(JSContext* ctx) {
    // magic function use to gather multiple C++ functions into one binding function
    // using magic_num to distinguish them
    JSValue global_this = JS_GetGlobalObject(ctx);

    constexpr int FnParamCount = 0;
    JSValue fn1 = JS_NewCFunctionMagic(ctx, BindMagicFn, "MagicFn1", FnParamCount, JS_CFUNC_generic_magic, 0);
    JSValue fn2 = JS_NewCFunctionMagic(ctx, BindMagicFn, "MagicFn2", FnParamCount, JS_CFUNC_generic_magic, 1);
    CheckJSValue(ctx, fn1);
    CheckJSValue(ctx, fn2);

    // also can use JS_DefinePropertyXXX
    QJS_CALL(JS_SetPropertyStr(ctx, global_this, "MagicFn1", fn1));
    QJS_CALL(JS_SetPropertyStr(ctx, global_this, "MagicFn2", fn2));

    JS_FreeValue(ctx, global_this);
}

void BindFF(JSContext* ctx) {
    /* NOTE: when use JS_CFUNC_f_f, JS_CFUNC_f_f_f, JS_CFUNC_setter,
     * JS_CFUNC_getter, JS_CFUNC_next, the function signature is not same as
     * JSCFunction! They has their own type(see member of JSCFunctionType)
     */
    JSCFunctionType fn_type;
    // JS_CFUNC_f_f pass one double elem and return one double elem
    fn_type.f_f = +[](double param) -> double {
        return param += 1;
    };
    JSValue fn =
        JS_NewCFunction2(ctx, fn_type.generic, "Increase", 1, JS_CFUNC_f_f, 0);

    CheckJSValue(ctx, fn);

    JSValue global_this = JS_GetGlobalObject(ctx);
    
    QJS_CALL(JS_SetPropertyStr(ctx, global_this, "Increase", fn));

    JS_FreeValue(ctx, global_this);
}

void BindFFF(JSContext* ctx) {
    /* NOTE: when use JS_CFUNC_f_f, JS_CFUNC_f_f_f, JS_CFUNC_setter,
     * JS_CFUNC_getter, JS_CFUNC_next, the function signature is not same as
     * JSCFunction! They has their own type(see member of JSCFunctionType)
     */
    JSCFunctionType fn_type;
    // JS_CFUNC_f_f_f pass two double elem and return one double elem
    fn_type.f_f_f =
        +[](double param1, double param2) -> double { return param1 + param2; };
    JSValue fn = JS_NewCFunction2(ctx, fn_type.generic, "Sum", 1,
                                  JS_CFUNC_f_f_f, 0);

    CheckJSValue(ctx, fn);

    JSValue global_this = JS_GetGlobalObject(ctx);

    // QJS_CALL(JS_SetPropertyStr(ctx, global_this, "Sum", fn));
    JS_DefinePropertyValueStr(ctx, global_this, "Sum", fn, JS_CFUNC_f_f_f);

    JS_FreeValue(ctx, global_this);   
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

    Bind(ctx);
    BindMagic(ctx);
    BindFF(ctx);
    BindFFF(ctx);

    std::cout << "-------------non strict mode---------------" << std::endl;
    ExecuteScript(ctx, "demos/04-BindingGlobalFunctions/main.js", 0);

    std::cout << std::endl
              << "-------------strict mode---------------" << std::endl;
    ExecuteScript(ctx, "demos/04-BindingGlobalFunctions/main.js",
                  JS_EVAL_FLAG_STRICT);

    JS_FreeContext(ctx);

    // don't forget free handlers
    js_std_free_handlers(runtime);

    JS_FreeRuntime(runtime);
    return 0;
}