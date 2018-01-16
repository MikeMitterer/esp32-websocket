get_parent_name(PARENT_NAME)
project(${PARENT_NAME}_tests)

include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})

add_executable(${PROJECT_NAME}

        basic_tests/ws_basic.cpp
        )

target_link_libraries(${PROJECT_NAME} gtest gtest_main)

