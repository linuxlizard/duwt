# CMake quick start:
# 	mkdir build
# 	cd build
# 	cmake ..
#
# Debian packages install (possibly incomplete)
# apt install build-essential pkg-config cmake libnl-3-dev libnl-genl-3-dev libpoco-dev

# Fedora packages install TODO
# 

# hint: command line invocation
# cmake -DCMAKE_BUILD_TYPE=Debug ..  <-- default
# cmake -DCMAKE_BUILD_TYPE=Release ..

cmake_minimum_required(VERSION 3.5)

project(duwt)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_STANDARD 14)
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

# optional (for now) package Poco
find_package(Poco QUIET COMPONENTS Foundation JSON Net)
message("poco found=${Poco_FOUND}")
if(${Poco_FOUND})
	message("found ${Poco_CONFIG} version=${Poco_VERSION} ${Poco_LIBRARIES}" )
	add_executable(test_logging src/test_logging.cc)
	target_link_libraries(test_logging PUBLIC Poco::Foundation)
endif()

add_library(duwt STATIC src/ie.cc src/bss.cc src/cfg80211.cc src/iw.cc src/logging.cc src/jsoncpp.cc src/oui.cc)
#target_compile_features(duwt PUBLIC cxx_std_17)
target_include_directories(duwt PUBLIC ${libnl_INCLUDE_DIRS})
target_include_directories(duwt PUBLIC fmt/include)
target_include_directories(duwt PUBLIC include)
target_compile_options(duwt PUBLIC $<$<CONFIG:DEBUG>:${MY_DEBUG_OPTIONS}>)

add_executable(scandump scandump/main.cc)
#target_compile_features(scandump PUBLIC cxx_std_17)
target_link_libraries(scandump duwt)
target_link_libraries(scandump ${libnl_LDFLAGS};${libnl_genl_LDFLAGS})
target_link_libraries(scandump fmt)
#target_link_options(scandump PUBLIC ${libnl_LDFLAGS};${libnl_genl_LDFLAGS})

add_executable(scanevent scanevent/main.cc)
#target_compile_features(scanevent PUBLIC cxx_std_17)
target_link_libraries(scanevent duwt)
target_link_libraries(scanevent fmt)
target_link_libraries(scanevent ${libnl_LDFLAGS};${libnl_genl_LDFLAGS})
#target_link_options(scanevent PUBLIC ${libnl_LDFLAGS};${libnl_genl_LDFLAGS})

add_executable(test_oui src/test_oui.cc src/oui.cc)
target_include_directories(test_oui PUBLIC include)
add_test(NAME test_oui COMMAND test_oui)

add_subdirectory(libiw)
add_subdirectory(httpserv)
#add_dependencies(httpserv libiw)

