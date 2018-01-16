cmake_minimum_required(VERSION 3.2)

# ENV ist im .bashrc definiert
#   (Am Mac auf /Volumes/Daten/DevLocal/DevCPP/cmake )
set(CMAKE_MODULE_PATH $ENV{CMAKE_MODULE_PATH} ${CMAKE_MODULE_PATH})

# ┌──────────────────────────────────────────────────────────────────┐
# │  Projects Settings                                               │
# └──────────────────────────────────────────────────────────────────┘

project(WebSocket)

# Output bin, c++11 usw
include(Config)

# Password for this project
include(.config/passwords.cmake)

# The version number.
set (${PROJECT_NAME}_VERSION_MAJOR 0)
set (${PROJECT_NAME}_VERSION_MINOR 1)

# configure a header file to pass some of the CMake settings
# to the source code
configure_file (
        "${PROJECT_SOURCE_DIR}/.config/config.h.in"
        "${PROJECT_SOURCE_DIR}/src/config.h"
        @ONLY
)

# ┌──────────────────────────────────────────────────────────────────┐
# │  Precompiled headers                                             │
# └──────────────────────────────────────────────────────────────────┘

add_library(${PROJECT_NAME}_pch
        ${PROJECT_SOURCE_DIR}/src/stdafx.cpp ${PROJECT_SOURCE_DIR}/src/stdafx.h)

set_source_files_properties(${PROJECT_SOURCE_DIR}/src/stdafx.cpp
        PROPERTIES
        COMPILE_FLAGS "-include ${PROJECT_SOURCE_DIR}/src/stdafx.h")

# ┌──────────────────────────────────────────────────────────────────┐
# │  Google-Test (BasisProjekt: find_package(GTest)                  │
# └──────────────────────────────────────────────────────────────────┘

#find_package(GTest)
add_subdirectory(tests)

# ┌──────────────────────────────────────────────────────────────────┐
# │  Platform.io                                                     │
# └──────────────────────────────────────────────────────────────────┘

include(CMakeListsPrivate.txt)

add_custom_target(
        ${PROJECT_NAME}_BUILD ALL
        COMMAND ${PLATFORMIO_CMD} -f -c clion run
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

add_custom_target(
        ${PROJECT_NAME}_UPLOAD ALL
        COMMAND ${PLATFORMIO_CMD} -f -c clion run --target upload
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

add_custom_target(
        ${PROJECT_NAME}_CLEAN ALL
        COMMAND ${PLATFORMIO_CMD} -f -c clion run --target clean
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

if(IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/lib)
    add_custom_target(
            CODE_COMPLETION_PIOLIB
            SOURCES lib
    )
endif()

if(IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/.piolibdeps)
    add_custom_target(
            CODE_COMPLETION_PIOLIBDEPS
            SOURCES .piolibdeps
    )
endif()

# ┌──────────────────────────────────────────────────────────────────┐
# │  Application                                                     │
# └──────────────────────────────────────────────────────────────────┘

include_directories(include)
include_directories(src)

set(SOURCE_FILES
        src/config.h

        src/main.cpp
        src/stdafx.cpp
        src/ota.cpp
        )

add_executable(${PROJECT_NAME} ${SOURCE_FILES})
