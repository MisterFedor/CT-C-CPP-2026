cmake_minimum_required(VERSION 3.20)
project(lab4)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

enable_testing()

find_package(GTest CONFIG REQUIRED)

add_executable(
	lab4_tests
	test/tests.cpp
	test/tests_functional_rb.cpp
	test/tests_functional_avl.cpp
	test/tests_reqs_rb.cpp
	test/tests_reqs_avl.cpp
)

target_include_directories(
	lab4_tests
	PRIVATE
	lib
	test
)

target_link_libraries(
	lab4_tests
	PRIVATE
	GTest::gtest
)

add_test(NAME lab4_tests COMMAND lab4_tests)
