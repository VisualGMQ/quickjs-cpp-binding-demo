import('std').then(module => {
    module.puts("I am std module in non-module mode\n");
}).catch(err => {
    print("Error loading module:", err);
});