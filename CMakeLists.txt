# CMake quick start:
# 	mkdir build
# 	cd build
# 	cmake ..
#
# Debian packages install (FIXME incomplete)
# apt install build-essential pkg-config cmake libnl-3-dev libnl-genl-3-dev  libjansson-dev libmicrohttpd-dev libssl-dev 

# Fedora packages install TODO (most recent test: Fedora 32)
# sudo dnf install make cmake gcc-g++ libnl3-devel jansson-devel libmicrohttpd-devel openssl-devel 

# hint: command line invocation
# cmake -DCMAKE_BUILD_TYPE=Debug ..  <-- default
# cmake -DCMAKE_BUILD_TYPE=Release ..

cmake_minimum_required(VERSION 3.5)

project(duwt)

set(CMAKE_BUILD_TYPE Debug)

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

add_library(mimetypes STATIC mimetypes/mimetypes.c)
target_include_directories(mimetypes PUBLIC mimetypes)

# libiw requires jansson
pkg_check_modules(jansson REQUIRED jansson)

add_subdirectory(libiw)
add_subdirectory(httpserv)
#add_dependencies(httpserv libiw)

