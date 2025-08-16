cmake_minimum_required(VERSION 3.15)

if (EXISTS "${CMAKE_SOURCE_DIR}/local.cmake")
    include(local.cmake)
    message(STATUS "Using local.cmake for configuration")
endif()
include(cmake/local_hints.cmake)
include(cmake/version.cmake)
include(cmake/resources.cmake)
list(INSERT CMAKE_MODULE_PATH 0 "${CMAKE_SOURCE_DIR}/cmake/modules")

get_git_tag_version(gitVersion dirtyFlag)

project(LogManager VERSION ${gitVersion} LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

check_local_hints()

find_package(Qt6 COMPONENTS Core Gui Widgets Test REQUIRED)
find_package(QuaZip-Qt6 COMPONENTS QuaZip)
find_package(Boost COMPONENTS headers REQUIRED)

file(GLOB ts ${CMAKE_CURRENT_SOURCE_DIR}/translations/*)
if (${ts} NOT STREQUAL "" AND QT6::LINGUIST_TOOLS_FOUND)
    qt_add_translation(QM_FILES ${ts})
    add_custom_target(translations ALL DEPENDS ${QM_FILES})
endif()

file(GLOB_RECURSE SOURCES src/*.cpp)
file(GLOB_RECURSE HEADERS src/*.h)
file(GLOB_RECURSE UI_FILES ui/*.ui src/*.ui)
file(GLOB_RECURSE RESOURCES res/*)
set(NON_MAIN_SOURCES ${SOURCES})
list(FILTER NON_MAIN_SOURCES EXCLUDE REGEX "src/main.cpp")

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/qtres)
    file(GLOB_RECURSE RESOURCE_FILE qtres/*.qrc)
    if("${RESOURCE_FILE}" STREQUAL "")
        file(GLOB_RECURSE QT_RESOURCES CONFIGURE_DEPENDS
            ${CMAKE_CURRENT_SOURCE_DIR}/qtres/*
        )

        if(NOT "${QT_RESOURCES}" STREQUAL "")
            set(qrcName "${TARGET}_resources")
            set(RESOURCE_FILE "${CMAKE_CURRENT_SOURCE_DIR}/qtres/${qrcName}.qrc")

            generate_qrc_file("${QT_RESOURCES}" "${CMAKE_CURRENT_SOURCE_DIR}/qtres" "${PROJECT_NAME}/" "${RESOURCE_FILE}")
        endif()
    endif()
endif()

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS} ${RESOURCE_FILE} ${QT_RESOURCES} ${UI_FILES} ${TS_FILES} ${RESOURCES})

target_compile_definitions(${PROJECT_NAME}
    PRIVATE
        APPLICATION_NAME="${PROJECT_NAME}"
        APPLICATION_VERSION="${PROJECT_VERSION}"
)

target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${PROJECT_SOURCE_DIR}/src
        ${PROJECT_SOURCE_DIR}/src/FormatCreation
        ${CMAKE_BINARY_DIR}/${PROJECT_NAME}_autogen
)

target_link_libraries(${PROJECT_NAME}
    PUBLIC
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        QuaZip::QuaZip
        Boost::headers
)

enable_testing()
file(GLOB TEST_SOURCES tests/*.cpp)
foreach(test_src ${TEST_SOURCES})
    get_filename_component(test_name ${test_src} NAME_WE)
    add_executable(${test_name} ${test_src} ${NON_MAIN_SOURCES} ${HEADERS})

    target_compile_definitions(${test_name}
        PRIVATE
            APPLICATION_NAME="${PROJECT_NAME}"
            APPLICATION_VERSION="${PROJECT_VERSION}"
    )

    target_include_directories(${test_name} PRIVATE ${PROJECT_SOURCE_DIR}/src)
    target_include_directories(${test_name} PRIVATE ${PROJECT_SOURCE_DIR}/src/FormatCreation)
    target_link_libraries(${test_name} Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Test QuaZip::QuaZip Boost::headers)
    add_test(NAME ${test_name} COMMAND ${test_name})
endforeach()

get_target_property(_qt5_qmake_location Qt::qmake IMPORTED_LOCATION)

execute_process(
    COMMAND "${_qt5_qmake_location}" -query QT_INSTALL_PREFIX
    RESULT_VARIABLE return_code
    OUTPUT_VARIABLE qt5_install_prefix
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

set(imported_location "${qt5_install_prefix}/bin/windeployqt.exe")
add_custom_command(TARGET ${PROJECT_NAME}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E remove_directory "$<TARGET_FILE_DIR:${PROJECT_NAME}>/deploy_folder"
    COMMAND ${imported_location} --dir "${CMAKE_BINARY_DIR}/deploy_folder" "$<TARGET_FILE_DIR:${PROJECT_NAME}>/$<TARGET_FILE_NAME:${PROJECT_NAME}>"
    COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:${PROJECT_NAME}>" "${CMAKE_BINARY_DIR}/deploy_folder"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_RUNTIME_DLLS:${PROJECT_NAME}>" "${CMAKE_BINARY_DIR}/deploy_folder"
    COMMAND_EXPAND_LISTS
    COMMENT "Deploy libraries"
)

install(TARGETS ${PROJECT_NAME}
    DESTINATION .
)

install(DIRECTORY "${CMAKE_BINARY_DIR}/deploy_folder/"
    DESTINATION .
    FILES_MATCHING PATTERN "*"
)

create_git_version_tag()

include (CPackComponent)

set(outputDir "${CMAKE_BINARY_DIR}/output")
set(outputFile "${outputFile}.zip")

set(CPACK_GENERATOR "ZIP")
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF)
set(CPACK_ARCHIVE_COMPONENT_INSTALL ON)
set(CPACK_COMPONENTS_GROUPING ONE_PER_GROUP)
set(CPACK_OUTPUT_FILE_PREFIX "${outputDir}")

include(CPack)

add_custom_target(release_packet
    COMMAND cpack --verbose --debug
)
