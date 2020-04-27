cmake_minimum_required(VERSION 3.10.0)

enable_testing()
enable_language(C)

# XXX tinkering
get_property(c_features GLOBAL PROPERTY CMAKE_C_KNOWN_FEATURES)
message("features=${c_features}")

# XXX tinkering
get_property(cxx_features GLOBAL PROPERTY CMAKE_CXX_KNOWN_FEATURES)
message("features=${cxx_features}")

set(CMAKE_TRY_COMPILE_TARGET_TYPE,EXE)

set(Tests HAVE_NL80211_BSS_PAD HAVE_NUM_NL80211_BANDS HAVE_NL80211_BAND_6GHZ)
list(APPEND Tests HAVE_NL80211_STA_INFO_RX_DURATION)
list(APPEND Tests HAVE_NL80211_BSS_CHAIN_SIGNAL)

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

add_library(iw STATIC iw.c iw-scan.c bytebuf.c xassert.c log.c hdump.c
		util.c ssid.c nlnames.c ie.c ie_he.c bss.c bug.c ie_print.c args.c)

target_include_directories(iw PUBLIC ${libnl_INCLUDE_DIRS} ${libnl_genl_INCLUDE_DIRS})

add_executable(test_ie test_ie.c)
target_include_directories(test_ie PUBLIC ${libnl_INCLUDE_DIRS} ${libnl_genl_INCLUDE_DIRS})
target_link_libraries(test_ie iw ${libnl_LDFLAGS} ${libnl_genl_LDFLAGS} )

add_test(NAME test_ie COMMAND test_ie)
