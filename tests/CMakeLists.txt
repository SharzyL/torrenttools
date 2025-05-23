cmake_minimum_required(VERSION 3.11)

add_executable(torrenttools-tests test_profile.cpp)

message(STATUS ${CMAKE_MODULE_PATH})
include(Catch)

target_include_directories(torrenttools-tests PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

get_target_property(torrenttools_SOURCES torrenttools SOURCES)
get_target_property(torrenttools_INCLUDE_DIRECTORIES torrenttools INCLUDE_DIRECTORIES)
get_target_property(torrenttools_LINK_LIBRARIES torrenttools LINK_LIBRARIES)

list(REMOVE_ITEM torrenttools_SOURCES src/main.cpp)
list(TRANSFORM torrenttools_SOURCES PREPEND ${PROJECT_SOURCE_DIR}/)

target_sources(torrenttools-tests PRIVATE
        main.cpp
        test_create.cpp
        test_edit.cpp
        test_verify.cpp
        test_file_matcher.cpp
        test_info.cpp
        test_magnet.cpp
        test_pad.cpp
        test_show.cpp
        test_tracker_database.cpp
        test_tree_view.cpp
        test_utils.cpp
        test_profile.cpp
        test_ls_colors.cpp
        ${torrenttools_SOURCES}
)



target_include_directories(torrenttools-tests PRIVATE ${torrenttools_INCLUDE_DIRECTORIES})
target_link_libraries(torrenttools-tests
        Catch2::Catch2
        ${torrenttools_LINK_LIBRARIES}
)

if (TORRENTTOOLS_BUILD_TESTS_COVERAGE)
    message(STATUS "Enabling coverage flags")
    list(APPEND CMAKE_CXX_FLAGS "--coverage")
    list(APPEND CMAKE_C_FLAGS "--coverage")
endif()


set(TEST_RESOUCES_DIR)
target_compile_definitions(torrenttools-tests PRIVATE
        TEST_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}\"
        TEST_RESOURCES_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}/resources\")

catch_discover_tests(torrenttools-tests)