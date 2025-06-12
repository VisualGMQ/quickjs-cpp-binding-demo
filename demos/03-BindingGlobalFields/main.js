console.log("global_var: ", global_var)
global_var = 224
console.log("changed global_var: ", global_var)

console.log("const_global_var1: ", const_global_var1)
console.log("const_global_var2: ", const_global_var2)
console.log("const_global_var3: ", const_global_var3)

try {
// will cause error
    const_global_var1 = 123;
} catch(e) {
    console.log(e)
}

// NOTE: not show var_with_non_enumerable, but show var_with_configurable
console.log("globalThis keys: ", Object.keys(globalThis))

console.log("var_with_non_enumerable: ", var_with_non_enumerable)
try {
    var_with_non_enumerable = 11
} catch(e) {
    console.log(e)
}

try {
    var_with_throw = 22334
} catch(e) {
    console.log(e)
}

var_with_writable = 11

try {
    you_cant_add_this_var = 99;
} catch(e) {
    console.log(e)
}

// will trigger getter
console.log(var_with_getter_setter)

// will trigger setter
var_with_getter_setter = 33

