#include "quickjs-libc.h"
#include "quickjs.h"

#include "common.hpp"

int gGlobalVar = 123;
int gNonChangableVar = 245;

void BindMutable(JSContext* ctx) {
    // Int32 value is directly copied into JSValue(no malloc), so we don't need JS_FreeValue it
    JSValue new_obj = JS_NewInt32(ctx, gGlobalVar);
    if (JS_IsException(new_obj)) {
        js_std_dump_error(ctx);
        JS_FreeValue(ctx, new_obj);
        return;
    }

    JSValue global_this = JS_GetGlobalObject(ctx);

    // JS_WRITABLE | JS_ENUMERABLE | JS_CONFIGURABLE by default
    JS_SetPropertyStr(ctx, global_this, "global_var", new_obj);

    // don't forget cleanup
    JS_FreeValue(ctx, global_this);
}

void BindConst(JSContext* ctx) {
    JSValue new_obj = JS_NewInt32(ctx, gNonChangableVar);
    if (JS_IsException(new_obj)) {
        js_std_dump_error(ctx);
        JS_FreeValue(ctx, new_obj);
        return;
    }

    JSValue global_this = JS_GetGlobalObject(ctx);

    /*
     * benefit:
     *      * it will handle JSValue lifetime automatically(you don't need call
     * JS_FreeValue)
     *      * it can set value property
     */
    QJS_CALL(JS_DefinePropertyValueStr(ctx, global_this, "const_global_var1",
                                       new_obj, JS_PROP_ENUMERABLE));

    /* we can also use JS_DefinePropertyValueStr to change value/prop by exists
     * JSValue like re-define a new variable in js
     */
    QJS_CALL(JS_SetPropertyStr(ctx, global_this, "const_global_var2", new_obj));
    QJS_CALL(JS_DefinePropertyValueStr(ctx, global_this, "const_global_var2",
                                       new_obj, JS_PROP_ENUMERABLE));

    /* but we can't use JS_SetPropertyStr to change const value(it will return
     * -1 as error) (like as assignment operation in js)
     */
    QJS_CALL(JS_SetPropertyStr(ctx, global_this, "const_global_var2", new_obj));

    // or you can use JS_DefinePropertyValue, the difference is it need a JSAtom
    // as name JSAtom aime to reuse name(deduce string copy)
    JSAtom name = JS_NewAtom(ctx, "const_global_var3");
    JS_DefinePropertyValue(ctx, global_this, name, new_obj, JS_PROP_ENUMERABLE);
    JS_FreeAtom(ctx, name);

    JS_FreeValue(ctx, global_this);
}

void BindByDifferentProperty(JSContext* ctx) {
    JSValue new_obj = JS_NewInt32(ctx, 666);
    if (JS_IsException(new_obj)) {
        js_std_dump_error(ctx);
        JS_FreeValue(ctx, new_obj);
        return;
    }

    JSValue global_this = JS_GetGlobalObject(ctx);

    // enumerable variable can be used in `for ... in` and `Object.keys()`
    QJS_CALL(JS_DefinePropertyValueStr(ctx, global_this, "var_with_enumerable",
                                       new_obj, JS_PROP_ENUMERABLE));
    // NOTE: JS_PROP_NORMAL == 0
    QJS_CALL(JS_DefinePropertyValueStr(
        ctx, global_this, "var_with_non_enumerable", new_obj, JS_PROP_NORMAL));
    // and we can't modify it property when don't has JS_PROP_CONFIGURABLE
    // this will not work but don't throw exception
    QJS_CALL(JS_DefinePropertyValueStr(ctx, global_this,
                                       "var_with_non_enumerable", new_obj,
                                       JS_PROP_ENUMERABLE));

    // compare with var_with_non_enumerable
    QJS_CALL(JS_DefinePropertyValueStr(ctx, global_this,
                                       "var_with_configurable", new_obj,
                                       JS_PROP_CONFIGURABLE));
    QJS_CALL(JS_DefinePropertyValueStr(ctx, global_this,
                                       "var_with_configurable", new_obj,
                                       JS_PROP_ENUMERABLE));

    /* JS_PROP_THROW will throw exception when did invalid operation by:
     * JS_SetPropertyXXX()
     * JS_DeletePropertyXXX()
     *
     * (yes, it used for Cpp rather than JavaScript)
     */
    QJS_CALL(JS_DefinePropertyValueStr(ctx, global_this, "var_with_throw",
                                       new_obj, JS_PROP_THROW));
    JSValue new_obj2 = JS_NewFloat64(ctx, 3.14);
    // can't assignment due to don't has JS_PROP_WRITABLE, will return -1
    QJS_CALL(JS_SetPropertyStr(ctx, global_this, "var_with_throw", new_obj2));
    // compare with above
    QJS_CALL(JS_DefinePropertyValueStr(ctx, global_this,
                                       "var_with_throw_writable", new_obj,
                                       JS_PROP_THROW | JS_PROP_WRITABLE));
    QJS_CALL(JS_SetPropertyStr(ctx, global_this, "var_with_throw_writable",
                               new_obj2));

    // JS_PROP_GETSET can set field's getter & setter
    JSValue getter = JS_NewCFunction(
        ctx,
        +[](JSContext*, JSValue self, int argc, JSValueConst* argv) {
            std::cout << "getter function called" << std::endl;
            return JS_UNDEFINED;
        },
        "getter", 0);

    JSValue setter = JS_NewCFunction(
        ctx,
        +[](JSContext*, JSValue self, int argc, JSValueConst* argv) {
            std::cout << "setter function called" << std::endl;
            return JS_UNDEFINED;
        },
        "setter", 1);
    JSAtom name = JS_NewAtom(ctx, "var_with_getter_setter");
    QJS_CALL(JS_DefineProperty(
        ctx, global_this, name, JS_UNDEFINED, getter, setter,
        JS_PROP_GETSET | JS_PROP_C_W_E | JS_PROP_HAS_VALUE |
            // meanwhile you must set JS_PROP_HAS_SET | JS_PROP_HAS_GET
            JS_PROP_HAS_GET | JS_PROP_HAS_SET));
    
    /* or use JS_DefinePropertyGetSet to simplify it
     * JS_PROP_HAS_GET | JS_PROP_HAS_SET | JS_PROP_HAS_CONFIGURABLE |
     * JS_PROP_HAS_ENUMERABLE by default
     */
    // JS_DefinePropertyGetSet(ctx, global_this, name, getter, setter,
    //                         JS_PROP_WRITABLE);

    // here we need free, when you has JS_PROP_HAS_GET or JS_PROP_HAS_SET
    JS_FreeValue(ctx, getter);
    JS_FreeValue(ctx, setter);
    JS_FreeAtom(ctx, name);

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

    BindMutable(ctx);
    BindConst(ctx);
    BindByDifferentProperty(ctx);

    std::cout << "-------------non strict mode---------------" << std::endl;
    ExecuteScript(ctx, "demos/03-BindingGlobalFields/main.js", 0);

    std::cout << std::endl
              << "-------------strict mode---------------" << std::endl;
    ExecuteScript(ctx, "demos/03-BindingGlobalFields/main.js",
                  JS_EVAL_FLAG_STRICT);

    JS_FreeContext(ctx);

    // don't forget free handlers
    js_std_free_handlers(runtime);

    JS_FreeRuntime(runtime);
    return 0;
}