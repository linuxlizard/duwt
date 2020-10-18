cmake_minimum_required(VERSION 3.10.0)

find_package(PkgConfig)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# add microhttpd for the server side but let it be optional for now
#find_package(microhttpd QUIET)
message("find_package microhttpd found=${microhttpd_FOUND}")
find_library(MHTTPD microhttpd)
message("find_library microhttpd found=${MHTTPD}")
#pkg_check_modules(libmicrohttpd QUIET libmicrohttpd)
#message("pkg-config libmicrohttpd found=${libmicrohttpd_FOUND} ${libmicrohttpd_INCLUDE_DIRS} ${libmicrohttpd_LDFLAGS}" )

# I'm also using the jansson JSON library
pkg_check_modules(jansson REQUIRED jansson)
message("jansson found=${jansson_FOUND} include=${jansson_INCLUDE_DIRS} ldflags=${jansson_LDFLAGS}" )

pkg_check_modules(libnl REQUIRED libnl-3.0)
pkg_check_modules(libnl_genl REQUIRED libnl-genl-3.0)

# For debugging, aim at my own libnl build.  Requires setting LD_LIBRARY_PATH
# before running.
#set(libnl_INCLUDE_DIRS -I/home/dpoole/libnl/include/libnl3)
#set(libnl_LDFLAGS "-L/home/dpoole/libnl/lib -lnl-3")
#set(libnl_genl_LDFLAGS "-L/home/dpoole/libnl/lib -lnl-genl-3")
#message("${libnl_INCLUDE_DIRS} ${libnl_LDFLAGS} ${libnl_genl_LDFLAGS}")

add_executable(sitesurvd sitesurvd.cc )
target_link_libraries(sitesurvd ${MHTTPD})
target_link_libraries(sitesurvd iw)
target_link_libraries(sitesurvd stdc++fs)
target_include_directories(sitesurvd PUBLIC ${libnl_INCLUDE_DIRS})
target_include_directories(sitesurvd PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../libiw)
target_include_directories(sitesurvd PUBLIC ../include)
target_link_libraries(sitesurvd ${libnl_LDFLAGS};${libnl_genl_LDFLAGS})
target_link_libraries(sitesurvd duwt)
target_link_libraries(sitesurvd jansson)

