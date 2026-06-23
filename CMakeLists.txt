cmake_minimum_required(VERSION 3.14)
project(anachrjsonistic CXX)

enable_testing()

add_executable(test_json tests/test_json.cpp)
target_include_directories(test_json PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/doctest)
target_compile_features(test_json PRIVATE cxx_std_17)
set_target_properties(test_json PROPERTIES CXX_EXTENSIONS OFF)
if(MSVC)
    target_compile_options(test_json PRIVATE /W4 /EHsc)
else()
    target_compile_options(test_json PRIVATE -Wall -Wextra)
endif()
add_test(NAME test_json COMMAND test_json)
