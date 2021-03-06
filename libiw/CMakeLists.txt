#
#  libiw/CMakeLists.txt   cmakefile for libiw
# 
#  Copyright (c) 2019-2020 David Poole <davep@mbuf.com>
# 
cmake_minimum_required(VERSION 3.10.0)

enable_testing()
enable_language(C)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wshadow -pedantic")

# XXX tinkering
get_property(c_features GLOBAL PROPERTY CMAKE_C_KNOWN_FEATURES)
#message("features=${c_features}")

# XXX tinkering
get_property(cxx_features GLOBAL PROPERTY CMAKE_CXX_KNOWN_FEATURES)
#message("features=${cxx_features}")

set(CMAKE_TRY_COMPILE_TARGET_TYPE,EXE)

set(Tests HAVE_NL80211_BSS_PAD HAVE_NUM_NL80211_BANDS HAVE_NL80211_BAND_6GHZ)
list(APPEND Tests HAVE_NL80211_STA_INFO_RX_DURATION)
list(APPEND Tests HAVE_NL80211_BSS_CHAIN_SIGNAL)
list(APPEND Tests HAVE_NL80211_RATE_INFO_HE_MCS)
list(APPEND Tests HAVE_NL80211_ATTR_HE_OBSS_PD)
list(APPEND Tests HAVE_NL80211_CMD_ABORT_SCAN)
list(APPEND Tests HAVE_NL80211_CMD_EXTERNAL_AUTH)
list(APPEND Tests HAVE_NL80211_ATTR_EXTERNAL_AUTH_ACTION)
list(APPEND Tests HAVE_NL80211_ATTR_AUTH_DATA)
list(APPEND Tests HAVE_NL80211_VLAN_ID)

# iterate through all the symbols we need, discovering if they exist
foreach(test IN LISTS Tests)
	try_compile(${test} ${CMAKE_CURRENT_BINARY_DIR} 
			SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/config.c
			COMPILE_DEFINITIONS -D${test}
			OUTPUT_VARIABLE output)
	message("try_compile ${test}=${${test}}")
endforeach()

# write the config.h with the symbols we discovered 
configure_file(config.h.in ${CMAKE_CURRENT_SOURCE_DIR}/config.h NEWLINE_STYLE UNIX)

# now start building our library
find_package(PkgConfig)
pkg_check_modules(libnl REQUIRED libnl-3.0)
pkg_check_modules(libnl_genl REQUIRED libnl-genl-3.0)

add_library(iw STATIC iw.c dev.c lib.c scan.c xassert.c log.c hdump.c
		util.c ssid.c nlnames.c ie.c ie_he.c bss.c ie_print.c args.c
		bss_json.c str.c dumpfile.c)

target_include_directories(iw PUBLIC ${libnl_INCLUDE_DIRS} ${libnl_genl_INCLUDE_DIRS})

# I'm also using the jansson JSON library
pkg_check_modules(jansson REQUIRED jansson)
message("jansson found=${jansson_FOUND} include=${jansson_INCLUDE_DIRS} ldflags=${jansson_LDFLAGS}" )

add_executable(scan-dump scan-dump.c)
target_include_directories(scan-dump PUBLIC ${libnl_INCLUDE_DIRS} ${libnl_genl_INCLUDE_DIRS})
target_link_libraries(scan-dump iw ${libnl_LDFLAGS} ${libnl_genl_LDFLAGS} )
target_link_libraries(scan-dump jansson)

add_executable(iw-link iw-link.c)
target_include_directories(iw-link PUBLIC ${libnl_INCLUDE_DIRS} ${libnl_genl_INCLUDE_DIRS})
target_link_libraries(iw-link iw ${libnl_LDFLAGS} ${libnl_genl_LDFLAGS} )

add_executable(iw-dev iw-dev.c)
target_include_directories(iw-dev PUBLIC ${libnl_INCLUDE_DIRS} ${libnl_genl_INCLUDE_DIRS})
target_link_libraries(iw-dev iw ${libnl_LDFLAGS} ${libnl_genl_LDFLAGS} )

add_executable(dumpfile-to-json dumpfile-to-json.c)
target_include_directories(dumpfile-to-json PUBLIC ${libnl_INCLUDE_DIRS} ${libnl_genl_INCLUDE_DIRS})
target_link_libraries(dumpfile-to-json iw ${libnl_LDFLAGS} ${libnl_genl_LDFLAGS} )
target_link_libraries(dumpfile-to-json jansson)

foreach(test IN ITEMS test_bss_json test_ie test_bss test_decode test_ie_he test_nlnames test_str test_utf8_ssid)
	message("test=${test}")
	add_executable(${test} ${test}.c)
	target_include_directories(${test} PUBLIC ${libnl_INCLUDE_DIRS} ${libnl_genl_INCLUDE_DIRS})
	target_link_libraries(${test} iw ${libnl_LDFLAGS} ${libnl_genl_LDFLAGS} )
	target_link_libraries(${test} jansson)
endforeach()

add_test(NAME test_ie COMMAND test_ie)
add_test(NAME test_bss COMMAND test_bss)
add_test(NAME test_decode COMMAND test_decode soup.dat)
add_test(NAME test_ie_he COMMAND test_ie_he)
add_test(NAME test_nlnames COMMAND test_nlnames)
add_test(NAME test_str COMMAND test_str)

add_test(NAME test_utf8_ssid COMMAND test_utf8_ssid)
add_test(NAME test_bss_json COMMAND test_bss_json)
