
function(git_tag_version name)
    if(DEFINED PRODUCT_VERSION)
        set(version ${PRODUCT_VERSION})
    else()
        execute_process(
            COMMAND git describe --first-parent --dirty --tags --match "v[0-9]*"
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            OUTPUT_VARIABLE git_desc
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET
        )

        if(git_desc MATCHES "^v([0-9]+)\\.([0-9]+)-?([0-9]+)?-?([a-z0-9][a-z0-9][a-z0-9][a-z0-9]+)?-?(dirty)?$")
            set(${name}_MAJOR ${CMAKE_MATCH_1} PARENT_SCOPE)
            set(${name}_MINOR ${CMAKE_MATCH_2} PARENT_SCOPE)

            if (DEFINED CMAKE_MATCH_3 AND NOT ${CMAKE_MATCH_3} STREQUAL "")
                set(${name}_PATCH ${CMAKE_MATCH_3} PARENT_SCOPE)
                set(${name}_PATCH ${CMAKE_MATCH_3})
            elseif(DEFINED CMAKE_MATCH_4 AND NOT ${CMAKE_MATCH_4} STREQUAL "" AND ${name}_PATCH STREQUAL "")
                set(${name}_PATCH ${CMAKE_MATCH_4} PARENT_SCOPE)
                set(${name}_PATCH ${CMAKE_MATCH_4})
            else()
                set(${name}_PATCH 0 PARENT_SCOPE)
                set(${name}_PATCH 0)
            endif()

            set(${name}_TWEAK 0 PARENT_SCOPE)

            if(CMAKE_MATCH_5 STREQUAL "")
                set(${name}_DIRTY false PARENT_SCOPE)
            else()
                set(${name}_DIRTY true PARENT_SCOPE)
            endif()

            set(version ${CMAKE_MATCH_1}.${CMAKE_MATCH_2}.${${name}_PATCH})
        else()
            set(${name}_MAJOR 1 PARENT_SCOPE)
            set(${name}_MINOR 0 PARENT_SCOPE)
            set(${name}_PATCH 0 PARENT_SCOPE)
            set(${name}_TWEAK 0 PARENT_SCOPE)
            set(${name}_DIRTY true PARENT_SCOPE)
            set(version "1.0.0")
        endif()
    endif()

    set(${name} "${version}" PARENT_SCOPE)
    add_compile_definitions(${name}="${version}")

    message("\nProject version: ${version}\n")
endfunction()

function(get_git_tag_version versionVar dirtyFlag)
    execute_process(
        COMMAND git describe --first-parent --dirty --tags --match "v[0-9]*"
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE version
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    if(version MATCHES "^v([0-9]+)\\.([0-9]+)(\\.[0-9]+)?-?([0-9]+)?-?([a-z0-9][a-z0-9][a-z0-9][a-z0-9]+)?-?(dirty)?$")
        set(version "${CMAKE_MATCH_1}.${CMAKE_MATCH_2}")

        if (DEFINED CMAKE_MATCH_3 AND NOT ${CMAKE_MATCH_3} STREQUAL "")
            set(patch "${CMAKE_MATCH_3}")
            if(DEFINED CMAKE_MATCH_4 AND NOT ${CMAKE_MATCH_4} STREQUAL "")
                math(EXPR patch "${CMAKE_MATCH_3} + ${CMAKE_MATCH_4}" OUTPUT_FORMAT DECIMAL)
            endif()
            set(version "${version}.${patch}")
        elseif(DEFINED CMAKE_MATCH_4 AND NOT ${CMAKE_MATCH_4} STREQUAL "")
            set(version "${version}.${CMAKE_MATCH_4}")
        else()
            set(version "${version}.0")
        endif()

        if(CMAKE_MATCH_6 STREQUAL "")
            set(${dirtyFlag} false PARENT_SCOPE)
        else()
            set(${dirtyFlag} true PARENT_SCOPE)
        endif()

        set(${versionVar} "${version}" PARENT_SCOPE)
        message("\nGit tag version: ${version}\n")
    endif()
endfunction()

function(create_git_version_tag)
    option(ENABLE_AUTO_TAG "Create a Git version tag after build" ON)
    option(AUTO_TAG_PUSH  "Push the new tag to origin" OFF)

    if(ENABLE_AUTO_TAG)
        configure_file(
            ${CMAKE_CURRENT_LIST_DIR}/cmake/AddGitTag.cmake.in
            ${CMAKE_BINARY_DIR}/AddGitTag.cmake
            @ONLY
        )

        add_custom_target(tag
            COMMAND ${CMAKE_COMMAND} -DAUTO_TAG_PUSH=${AUTO_TAG_PUSH} -P ${CMAKE_BINARY_DIR}/AddGitTag.cmake
            COMMENT "[auto-tag] Checking/creating Git tag"
            VERBATIM
        )
    endif()
endfunction()
