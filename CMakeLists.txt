# hint: command line invocation
# cmake -DCMAKE_BUILD_TYPE=Debug ..
# cmake -DCMAKE_BUILD_TYPE=Release ..

cmake_minimum_required(VERSION 3.7)

project(duwt)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_STANDARD 17)

enable_testing()

find_package(PkgConfig)
pkg_check_modules(libnl REQUIRED libnl-3.0)
message("hello libnl found=${libnl_FOUND} ${libnl_INCLUDE_DIRS} ${libnl_LDFLAGS}" )
pkg_check_modules(libnl_genl REQUIRED libnl-genl-3.0)
message("hello libnl_genl_genl found=${libnl_genl_FOUND} ${libnl_genl_INCLUDE_DIRS} ${libnl_genl_LDFLAGS}" )

add_subdirectory(fmt)

add_library(duwt OBJECT src/cfg80211.cc src/ie.cc src/iw.cc src/logging.cc)
target_compile_features(duwt PUBLIC cxx_std_17)
target_include_directories(duwt PUBLIC ${libnl_INCLUDE_DIRS})
target_include_directories(duwt PUBLIC fmt/include)
target_include_directories(duwt PUBLIC include)

add_executable(scandump scandump/main.cc)
target_compile_features(scandump PUBLIC cxx_std_17)
target_link_libraries(scandump duwt)
target_link_libraries(scandump fmt)
target_link_options(scandump PUBLIC ${libnl_LDFLAGS};${libnl_genl_LDFLAGS})
#target_link_options(scandump PUBLIC ${libnl_genl_LDFLAGS})


