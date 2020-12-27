# CMake quick start:
# 	mkdir build
# 	cd build
# 	cmake ..
#
# Debian packages install (FIXME incomplete)
# apt install build-essential pkg-config cmake libnl-3-dev libnl-genl-3-dev libjsoncpp-dev libjansson-dev libmicrohttpd-dev libssl-dev libpoco-dev

# Fedora packages install TODO (most recent test: Fedora 32)
# sudo dnf install make cmake gcc-g++ libnl3-devel jsoncpp-devel jansson-devel libmicrohttpd-devel openssl-devel poco-devel
# 

# hint: command line invocation
# cmake -DCMAKE_BUILD_TYPE=Debug ..  <-- default
# cmake -DCMAKE_BUILD_TYPE=Release ..

cmake_minimum_required(VERSION 3.5)

project(duwt)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

enable_testing()

find_package(PkgConfig)
pkg_check_modules(libnl REQUIRED libnl-3.0)
message("libnl found=${libnl_FOUND} ${libnl_INCLUDE_DIRS} ${libnl_LDFLAGS}" )
pkg_check_modules(libnl_genl REQUIRED libnl-genl-3.0)
message("libnl_genl_genl found=${libnl_genl_FOUND} ${libnl_genl_INCLUDE_DIRS} ${libnl_genl_LDFLAGS}" )

#https://github.com/lefticus/cppbestpractices/blob/master/02-Use_the_Tools_Available.md
#-Wall -Wextra -Wshadow -Wnon-virtual-dtor -pedantic
set(MY_DEBUG_OPTIONS -Wall -Wextra -Wshadow -Wnon-virtual-dtor -pedantic )

#pkg_check_modules(jsoncpp REQUIRED jsoncpp)
#message("jsoncpp found=${Jsoncpp_FOUND} include=${jsoncpp_INCLUDE_DIRS} ldflags=${jsoncpp_LDFLAGS}" )

add_library(duwt STATIC src/oui.cc src/mimetypes.cc src/survey.cc)
target_include_directories(duwt PUBLIC ${libnl_INCLUDE_DIRS} ${jsoncpp_INCLUDE_DIRS})
target_include_directories(duwt PUBLIC include libiw)
target_compile_options(duwt PUBLIC $<$<CONFIG:DEBUG>:${MY_DEBUG_OPTIONS}>)

# libiw requires jansson
pkg_check_modules(jansson REQUIRED jansson)

foreach(test IN ITEMS test_oui test_survey)
	message("test=${test}")
	# strip the "test_" from the test name to get the src we're trying to test
	string(REPLACE "test_" "" test_target ${test})
	message("test_target=${test_target}")

	add_executable(${test} src/${test}.cc src/${test_target}.cc src/${test}_main.cc)

	target_include_directories(${test} PUBLIC include)
	target_link_libraries(${test} duwt jansson)
	target_include_directories(${test} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/libiw)
	target_link_libraries(${test} iw ${libnl_LDFLAGS} ${libnl_genl_LDFLAGS} )

	add_test(NAME ${test} COMMAND ${test} )
endforeach()

# TODO move test_mimetypes to catch.hpp
add_executable(test_mimetypes src/test_mimetypes.cc)
target_link_libraries(test_mimetypes duwt)
target_link_libraries(test_mimetypes stdc++fs)
add_test(NAME test_mimetypes COMMAND test_mimetypes)

add_subdirectory(libiw)
add_subdirectory(httpserv)
#add_dependencies(httpserv libiw)

