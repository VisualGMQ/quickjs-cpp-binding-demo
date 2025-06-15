#include "quickjs-libc.h"
#include "quickjs.h"

#include "common.hpp"

struct Person {
    static int ID;

    char name[512] = {0};
    float height;
    float weight;
    int age;

    Person(const std::string& name, float height, int age, float weight)
        : height{height}, age{age}, weight{weight} {
        ChangeName(name);
    }

    void Introduce() const {
        std::cout << "I am " << name << ", age " << age << ", height " << height
                  << ", weight " << weight << std::endl;
    }

    float GetBMI() const { return weight / (height * height); }

    void ChangeName(const std::string& name) {
        strcpy(this->name, name.data());
    }
};

///////////////// Class Binding related code BEGINE ////////////////////////////

int Person::ID = 1;

JSClassID gClassID = 0;

JSValue IntroduceBinding(JSContext* ctx, JSValue self, int argc,
                         JSValueConst* args) {
    // I'm lazy to check type :-)
    const Person* p = static_cast<const Person*>(JS_GetOpaque(self, gClassID));
    p->Introduce();
    return JS_UNDEFINED;
}

JSValue NameGetter(JSContext* ctx, JSValue self) {
    // I'm lazy to check type :-)
    const Person* p = static_cast<const Person*>(JS_GetOpaque(self, gClassID));
    return JS_NewString(ctx, p->name);
}

JSValue NameSetter(JSContext* ctx, JSValue self, JSValueConst param) {
    // I'm lazy to check type :-)
    Person* p = static_cast<Person*>(JS_GetOpaque(self, gClassID));
    p->ChangeName(JS_ToCString(ctx, param));
    return JS_UNDEFINED;
}

JSValue ConstructorBinding(JSContext* ctx, JSValue self, int argc,
                           JSValueConst* argv) {
    // I'm lazy to check argv type :-)
    const char* name = JS_ToCString(ctx, argv[0]);
    double height;
    QJS_CALL(JS_ToFloat64(ctx, &height, argv[1]));
    int age;
    QJS_CALL(JS_ToInt32(ctx, &age, argv[2]));
    double weight;
    QJS_CALL(JS_ToFloat64(ctx, &weight, argv[3]));

    Person* person = new Person(name, height, age, weight);
    JSValue result = JS_NewObjectClass(ctx, gClassID);
    CheckJSValue(ctx, result);
    QJS_CALL(JS_SetOpaque(result, person));
    return result;
}

JSValue BMIBinding(JSContext* ctx, JSValue self) {
    // I'm lazy to check type :-)
    Person* p = static_cast<Person*>(JS_GetOpaque(self, gClassID));
    return JS_NewFloat64(ctx, p->GetBMI());
}

// This lifetime must longer than script JSValue
const JSCFunctionListEntry entries[] = {
    // bind member function
    JS_CFUNC_DEF("introduce", 0, IntroduceBinding),

    // name getter&setter
    JS_CGETSET_DEF("name", NameGetter, NameSetter),
    // lazy to bind other members :-)
    // ...

    // define member varaible by getter
    JS_CGETSET_DEF("bmi", BMIBinding, nullptr),

};

JSValue gClassConstructor;

void PrepareBindClass(JSRuntime* runtime, JSContext* ctx) {
    // NOTE: id must not nullptr
    // class id is unique id for class
    gClassID = JS_NewClassID(runtime, &gClassID);
    if (gClassID == 0) {
        std::cerr << "create class id failed" << std::endl;
    }

    const char* class_name = "Person";

    // don't forget zero-initialize
    JSClassDef def{};
    // will call when value be freed
    def.finalizer = +[](JSRuntime*, JSValue self) {
        if (!JS_IsObject(self)) {
            std::cerr << "in finalizer, self is not object" << std::endl;
        }

        Person* opaque = static_cast<Person*>(JS_GetOpaque(self, gClassID));
        if (!opaque) {
            std::cerr << "self is nullptr" << std::endl;
        }

        delete opaque;
    };
    def.class_name = class_name;

    QJS_CALL(JS_NewClass(runtime, gClassID, &def));

    JSValue proto = JS_NewObject(ctx);
    CheckJSValue(ctx, proto);

    // binding member variable
    // must use getter/setter function
    {
        // using function list entry to simplify property binding
        // WARNING: entries is passed by reference, so it can't be free before
        // script eval
        // NOTE: using JS_CGETSET_DEF must under C++20 standard due to syntax
        // require
        JS_SetPropertyFunctionList(ctx, proto, entries, std::size(entries));
    }

    // register constructor
    {
        gClassConstructor = JS_NewCFunction2(ctx, ConstructorBinding, class_name, 4,
                                       JS_CFUNC_constructor, 0);
        CheckJSValue(ctx, gClassConstructor);
    }

    // global field directly register to constructor rather than proto
    JSValue id_value = JS_NewInt32(ctx, Person::ID);
    QJS_CALL(JS_SetPropertyStr(ctx, gClassConstructor, "ID", id_value));

    // create class prototype
    JS_SetClassProto(ctx, gClassID, proto);
}

//////////////////// Class Binding related code END ////////////////////////////

// function to bind on module
JSValue AddFnBinding(JSContext* ctx, JSValue self, int argc,
                     JSValueConst* argv) {
    if (argc != 2) {
        return JS_UNDEFINED;
    }

    int32_t a, b;
    QJS_CALL(JS_ToInt32(ctx, &a, argv[0]));
    QJS_CALL(JS_ToInt32(ctx, &b, argv[1]));

    return JS_NewInt32(ctx, a + b);
}

int ModuleInitFn(JSContext* ctx, JSModuleDef* m) {
    // set JSValue to module
    QJS_CALL(JS_SetModuleExport(ctx, m, "Add",
                                JS_NewCFunction(ctx, AddFnBinding, "Add", 2)));
    QJS_CALL(JS_SetModuleExport(ctx, m, "Person", gClassConstructor));
    // 0 - success
    // < 0 - failed
    return 0;
}

void BindingModule(JSContext* ctx) {
    JSModuleDef* module_def = JS_NewCModule(ctx, "MyModule", ModuleInitFn);
    if (!module_def) {
        std::cerr << "create C module failed" << std::endl;
        return;
    }

    // set member in module which you want to export
    JS_AddModuleExport(ctx, module_def, "Add");
    JS_AddModuleExport(ctx, module_def, "Person");
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

    PrepareBindClass(runtime, ctx);
    BindingModule(ctx);
    std::cout << "-------------non strict mode---------------" << std::endl;
    ExecuteScript(ctx, "demos/06-Module/main.js", JS_EVAL_TYPE_MODULE);

    std::cout << std::endl
              << "-------------strict mode---------------" << std::endl;
    ExecuteScript(ctx, "demos/06-Module/main.js",
                  JS_EVAL_TYPE_MODULE | JS_EVAL_FLAG_STRICT);

    js_std_loop(ctx);

    JS_FreeContext(ctx);

    // don't forget free handlers
    js_std_free_handlers(runtime);

    JS_FreeRuntime(runtime);
    return 0;
}