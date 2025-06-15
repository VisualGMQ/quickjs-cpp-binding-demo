AddDemo(07_run_bytecode)

add_custom_command(TARGET 07_run_bytecode
    PRE_BUILD
    COMMAND $<TARGET_FILE:qjsc> -b -n "main.js" -o output.qjs main.js
    COMMENT "compiling qjs code..."
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    VERBATIM)