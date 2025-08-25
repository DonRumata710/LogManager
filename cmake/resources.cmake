
function(update_file filename content)
    if(EXISTS ${fileName})
        file(READ "${fileName}" prevContent)

        if(content STREQUAL prevContent)
            return()
        endif()
    endif()

    file(WRITE ${filename} "${content}")
endfunction()

function(generate_qrc_file resources relDir aliasPrefix resFile)
    set(content ""  )
    list(APPEND content "<RCC>")
    list(APPEND content "    <qresource prefix=\"/\">")

    foreach(resource IN LISTS resources)
        file(RELATIVE_PATH rel "${relDir}" ${resource})
        list(APPEND content "        <file alias=\"${aliasPrefix}${rel}\">${rel}</file>")
    endforeach()

    list(APPEND content "    </qresource>")
    list(APPEND content "</RCC>")
    list(APPEND content "")

    string(REGEX REPLACE ";" "\n" content "${content}")

    update_file(${resFile} "${content}")
endfunction()
