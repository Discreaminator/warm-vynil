function(wg_set_warnings target)
    if(MSVC)
        # /utf-8: source + execution charset UTF-8 (RU literals / CharPointer_UTF8 escapes).
        target_compile_options(${target} PRIVATE /W4 /permissive- /WX /utf-8)
    else()
        target_compile_options(${target} PRIVATE
            -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Werror)
    endif()
endfunction()
