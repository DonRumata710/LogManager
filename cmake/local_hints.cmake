
macro(add_external_dependency localHints package)
    if(NOT EXISTS "${CMAKE_SOURCE_DIR}/3rdparty/${package}")
        list(APPEND CMAKE_PREFIX_PATH ${localHints}/${package})
        string(TOUPPER "${package}" uname)
        set(${uname}_ROOT "${localHints}${package}" CACHE PATH "path to ${package}")
        set(${package}_ROOT "${localHints}${package}" CACHE PATH "path to ${package}")
        message("Added ${package} to search paths")
    endif()
endmacro()

macro(check_local_hints)
    if(NOT "${LOCAL_HINTS}" STREQUAL "")
        if(NOT DEFINED TARGET_ARCH)
            if(CMAKE_SIZEOF_VOID_P EQUAL 4)
                set(TARGET_ARCH "x32")
            else()
                set(TARGET_ARCH "x64")
            endif()
        endif()

        if(TARGET_ARCH STREQUAL "x32")
            set(_ARCH_SUFFIX "32")
        else()
            set(_ARCH_SUFFIX "64")
        endif()

        if(WIN32)
            set(_OS_SUFFIX "win")
        else()
            set(_OS_SUFFIX "linux")
        endif()

        if(MINGW)
            set(_COMPILE_FLAG "-mingw${_ARCH_SUFFIX}")
        endif()

        set(_LOCAL_HINTS "${LOCAL_HINTS}/${_OS_SUFFIX}${_ARCH_SUFFIX}${_COMPILE_FLAG}/")

        if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
            string(APPEND _LOCAL_HINTS "Debug/")
        else()
            string(APPEND _LOCAL_HINTS "Release/")
        endif()

        list(APPEND CMAKE_PREFIX_PATH ${_LOCAL_HINTS})

        file(GLOB packages RELATIVE ${_LOCAL_HINTS} ${_LOCAL_HINTS}*)

        foreach(package ${packages})
            add_external_dependency(${_LOCAL_HINTS} ${package})
        endforeach()

        file(GLOB packages RELATIVE ${LOCAL_HINTS}/any/ ${LOCAL_HINTS}/any/*)

        foreach(package ${packages})
            add_external_dependency(${LOCAL_HINTS}/any/ ${package})
        endforeach()
    endif()
endmacro()
