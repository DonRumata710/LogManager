cmake_minimum_required(VERSION 3.15)

if (EXISTS "${CMAKE_SOURCE_DIR}/local.cmake")
    include(local.cmake)
    message(STATUS "Using local.cmake for configuration")
endif()
include(cmake/local_hints.cmake)
include(cmake/version.cmake)

get_git_tag_version(gitVersion dirtyFlag)

project(LogManager VERSION ${gitVersion} LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

check_local_hints()

find_package(Qt6 COMPONENTS Core Gui Widgets REQUIRED)
find_package(QuaZip-Qt6 COMPONENTS QuaZip)

file(GLOB ts ${CMAKE_CURRENT_SOURCE_DIR}/translations/*)
if (${ts} NOT STREQUAL "" AND QT6::LINGUIST_TOOLS_FOUND)
    qt_add_translation(QM_FILES ${ts})
    add_custom_target(translations ALL DEPENDS ${QM_FILES})
endif()

file(GLOB_RECURSE SOURCES src/*.cpp)
file(GLOB_RECURSE HEADERS src/*.h)
file(GLOB_RECURSE RESOURCE_FILE qtres/*.qrc)
file(GLOB_RECURSE UI_FILES ui/*.ui src/*.ui)
file(GLOB_RECURSE RESOURCES res/*)

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS} ${RESOURCE_FILE} ${UI_FILES} ${TS_FILES} ${RESOURCES})

target_compile_definitions(${PROJECT_NAME} PRIVATE APPLICATION_NAME="${PROJECT_NAME}"
    APPLICATION_VERSION="${PROJECT_VERSION}"
)

target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${PROJECT_SOURCE_DIR}/src
        ${CMAKE_BINARY_DIR}/${PROJECT_NAME}_autogen
)

target_link_libraries(${PROJECT_NAME}
    PUBLIC
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        QuaZip::QuaZip
)

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
    COMMENT "Deploy libraries"
)

install(TARGETS ${PROJECT_NAME}
    DESTINATION .
)

install(DIRECTORY "${CMAKE_BINARY_DIR}/deploy_folder/"
    DESTINATION .
    FILES_MATCHING PATTERN "*"
)

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
