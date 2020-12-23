# CMake quick start:
# 	mkdir build
# 	cd build
# 	cmake ..
#
# Debian packages install (FIXME incomplete)
# apt install build-essential pkg-config cmake libnl-3-dev libnl-genl-3-dev libjsoncpp-dev libjansson-dev libmicrohttpd-dev libssl-dev libpoco-dev

# Fedora packages install TODO (most recent test: Fedora 32)
# sudo dnf install make cmake gcc-g++ libnl3-devel jsoncpp-devel jansson-devel libmicrohttpd-devel openssl-devel
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

add_subdirectory(fmt)

#https://github.com/lefticus/cppbestpractices/blob/master/02-Use_the_Tools_Available.md
#-Wall -Wextra -Wshadow -Wnon-virtual-dtor -pedantic
set(MY_DEBUG_OPTIONS -Wall -Wextra -Wnon-virtual-dtor -pedantic )
# TODO fmtlib has warnings with -Wshadow
#set(MY_DEBUG_OPTIONS -Wall -Wextra -Wshadow -Wnon-virtual-dtor -pedantic -Wold-style-cast)

pkg_check_modules(jsoncpp REQUIRED jsoncpp)
message("jsoncpp found=${Jsoncpp_FOUND} include=${jsoncpp_INCLUDE_DIRS} ldflags=${jsoncpp_LDFLAGS}" )

add_library(duwt STATIC src/oui.cc src/mimetypes.cc)
#target_compile_features(duwt PUBLIC cxx_std_17)
target_include_directories(duwt PUBLIC ${libnl_INCLUDE_DIRS} ${jsoncpp_INCLUDE_DIRS})
target_include_directories(duwt PUBLIC fmt/include)
target_include_directories(duwt PUBLIC include)
target_compile_options(duwt PUBLIC $<$<CONFIG:DEBUG>:${MY_DEBUG_OPTIONS}>)

add_executable(test_oui src/test_oui.cc)
target_include_directories(test_oui PUBLIC include)
target_link_libraries(test_oui duwt)
# precompiled headers added in cmake 3.16
if(${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.16")
target_compile_definitions(test_oui PUBLIC CATCH_CONFIG_MAIN)
target_precompile_headers(test_oui PRIVATE include/catch.hpp)
endif()
add_test(NAME test_oui COMMAND test_oui)

add_executable(test_mimetypes src/test_mimetypes.cc)
target_link_libraries(test_mimetypes duwt)
target_link_libraries(test_mimetypes stdc++fs)
add_test(NAME test_mimetypes COMMAND test_mimetypes)

add_subdirectory(libiw)
add_subdirectory(httpserv)
#add_dependencies(httpserv libiw)

