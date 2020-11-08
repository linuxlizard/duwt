cmake_minimum_required(VERSION 3.10.0)

find_package(PkgConfig)

enable_testing()
enable_language(C)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wshadow -pedantic")

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

# openssl
pkg_check_modules(ssl REQUIRED openssl)
message("ssl found=${ssl_FOUND} include=${ssl_INCLUDE_DIRS} ldflags=${ssl_LDFLAGS}" )

add_executable(sitesurvd sitesurvd.c bytebuf.c)
target_link_libraries(sitesurvd ${MHTTPD})
target_link_libraries(sitesurvd iw)
target_include_directories(sitesurvd PUBLIC ${libnl_INCLUDE_DIRS})
target_include_directories(sitesurvd PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../libiw)
target_include_directories(sitesurvd PUBLIC ../include)
target_link_libraries(sitesurvd ${libnl_LDFLAGS};${libnl_genl_LDFLAGS})
target_link_libraries(sitesurvd duwt)
target_link_libraries(sitesurvd jansson)

foreach(test IN ITEMS test_bytebuf)
	message("test=${test}")
	# strip the "test_" from the test name to get the src we're trying to test
	string(REPLACE "test_" "" test_target ${test})
	message("test_target=${test_target}")
	add_executable(${test} ${test}.c ${test_target}.c)
	target_include_directories(${test} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../libiw)
	target_include_directories(${test} PUBLIC ${libnl_INCLUDE_DIRS} ${libnl_genl_INCLUDE_DIRS})
	target_link_libraries(${test} iw ${libnl_LDFLAGS} ${libnl_genl_LDFLAGS} )
	target_link_libraries(${test} jansson)
	target_link_libraries(${test} ${ssl_LDFLAGS})
endforeach()

add_test(NAME test_bytebuf COMMAND test_bytebuf)
