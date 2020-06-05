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

pkg_check_modules(libnl REQUIRED libnl-3.0)
pkg_check_modules(libnl_genl REQUIRED libnl-genl-3.0)

pkg_check_modules(jsoncpp REQUIRED jsoncpp)
message("jsoncpp found=${Jsoncpp_FOUND} include=${jsoncpp_INCLUDE_DIRS} ldflags=${jsoncpp_LDFLAGS}" )

add_executable(sitesurvd sitesurvd.cc mimetypes.cc bss_json.cc)
target_link_libraries(sitesurvd ${MHTTPD})
target_link_libraries(sitesurvd iw)
target_link_libraries(sitesurvd stdc++fs)
target_include_directories(sitesurvd PUBLIC ${libnl_INCLUDE_DIRS} ${jsoncpp_INCLUDE_DIRS})
target_include_directories(sitesurvd PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../libiw)
target_link_libraries(sitesurvd ${libnl_LDFLAGS};${libnl_genl_LDFLAGS})
target_link_libraries(sitesurvd ${jsoncpp_LDFLAGS})

add_executable(test_mimetypes test_mimetypes.cc mimetypes.cc)
target_link_libraries(test_mimetypes stdc++fs)

add_test(NAME test_mimetypes COMMAND test_mimetypes)

add_executable(test_bss_json test_bss_json.cc bss_json.cc)
target_include_directories(test_bss_json PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../libiw)
target_include_directories(test_bss_json PUBLIC ${libnl_INCLUDE_DIRS} ${jsoncpp_INCLUDE_DIRS})
target_link_libraries(test_bss_json ${libnl_LDFLAGS};${libnl_genl_LDFLAGS})
target_link_libraries(test_bss_json ${jsoncpp_LDFLAGS})
target_link_libraries(test_bss_json iw)
