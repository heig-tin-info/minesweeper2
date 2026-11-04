# Fitness function for the architecture: every #include must respect the
# dependency rule. Run with: cmake -DSOURCE_DIR=<src> -P check_layers.cmake
#
#   core      -> only core and the standard library, and no I/O at all
#   app       -> core and app, no I/O
#   adapters  -> anything

function(forbid layer pattern reason)
    file(GLOB_RECURSE files "${SOURCE_DIR}/${layer}/*.hpp" "${SOURCE_DIR}/${layer}/*.cpp")
    foreach(file IN LISTS files)
        file(STRINGS "${file}" includes REGEX "^[ \t]*#[ \t]*include")
        foreach(line IN LISTS includes)
            if(line MATCHES "${pattern}")
                message(SEND_ERROR "${file}: `${line}` (${reason})")
            endif()
        endforeach()
    endforeach()
endfunction()

set(io "<(iostream|fstream|cstdio|SDL)")

forbid(core "\"(app|adapters|launcher)/" "the domain depends on nothing")
forbid(core "${io}" "the domain does no I/O")
forbid(core "<chrono>" "the domain does not know the time")
forbid(app "\"(adapters|launcher)/" "the application does not know its adapters")
forbid(app "${io}" "the application does no I/O")
