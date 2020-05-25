cmake_minimum_required(VERSION 3.10.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# add microhttpd for the server side but let it be optional for now
#find_package(microhttpd QUIET)
message("find_package microhttpd found=${microhttpd_FOUND}")
find_library(MHTTPD microhttpd)
message("find_library microhttpd found=${MHTTPD}")
#pkg_check_modules(libmicrohttpd QUIET libmicrohttpd)
#message("pkg-config libmicrohttpd found=${libmicrohttpd_FOUND} ${libmicrohttpd_INCLUDE_DIRS} ${libmicrohttpd_LDFLAGS}" )

pkg_check_modules(jansson REQUIRED jansson)
message("jansson found=${jansson_FOUND} include=${jansson_INCLUDE_DIRS} ldflags=${jansson_LDFLAGS}" )

find_package(PkgConfig)
pkg_check_modules(libnl REQUIRED libnl-3.0)
pkg_check_modules(libnl_genl REQUIRED libnl-genl-3.0)

#message("CMAKE_CURRENT_BINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}")
#set(CMAKE_FIND_DEBUG_MODE 1)
#set(ENV{PATH} "${CMAKE_CURRENT_BINARY_DIR}/lib")
#find_library(LIBIW 
#		NAMES iw
#		PATHS ${CMAKE_CURRENT_BINARY_DIR}/../libiw
#		NO_DEFAULT_PATH)
#message("find_library libiw found=${LIBIW}")
#if(${LIBIW} STREQUAL "LIBIW-NOTFOUND")
#	message(FATAL_ERROR "libiw not found")
#endif()

add_executable(sitesurvd sitesurvd.cc mimetypes.cc)
target_link_libraries(sitesurvd ${MHTTPD} ${jansson_LDFLAGS})
target_link_libraries(sitesurvd iw)
target_link_libraries(sitesurvd stdc++fs)
target_include_directories(sitesurvd PUBLIC ${libnl_INCLUDE_DIRS})
target_include_directories(sitesurvd PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../libiw)
target_link_libraries(sitesurvd ${libnl_LDFLAGS};${libnl_genl_LDFLAGS})

add_executable(test_mimetypes test_mimetypes.cc mimetypes.cc)
target_link_libraries(test_mimetypes stdc++fs)

add_test(NAME test_mimetypes COMMAND test_mimetypes)

# optional (for now) package Poco
find_package(Poco QUIET COMPONENTS Foundation JSON Net Util)
message("poco found=${Poco_FOUND}")
if(${Poco_FOUND})
	message("found ${Poco_CONFIG} version=${Poco_VERSION} ${Poco_LIBRARIES}" )
	add_executable(test_logging test_logging.cc)
	target_link_libraries(test_logging PUBLIC Poco::Foundation)
endif()

add_executable(poco-httpd poco-httpd.cc)
target_link_libraries(poco-httpd PUBLIC Poco::Foundation Poco::Net Poco::Util)

add_executable(bss_json bss_json.cc)
target_include_directories(bss_json PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../libiw)
target_include_directories(bss_json PUBLIC ${libnl_INCLUDE_DIRS})
target_link_libraries(bss_json Poco::Foundation Poco::JSON)
target_link_libraries(bss_json ${libnl_LDFLAGS};${libnl_genl_LDFLAGS})
target_link_libraries(bss_json iw)
