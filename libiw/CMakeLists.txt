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
		util.c ssid.c nlnames.c ie.c ie_he.c bss.c bug.c ie_print.c args.c
		bss_json.c)

target_include_directories(iw PUBLIC ${libnl_INCLUDE_DIRS} ${libnl_genl_INCLUDE_DIRS})

# XXX tinkering with cpprestsdk and CMake
find_package(cpprestsdk REQUIRED)
if(${cpprestsdk_FOUND})
#	get_target_properties(prop cpprestsdk::cpprest
	message("Found cpprestsdk")
	message("found ${cpprestsdk_CONFIG} version=${cpprestsdk_VERSION} lib=${cpprestsdk_LIBRARIES} inc=${cpprestsdk_INCLUDE_DIRS} ldflags=${cpprestsdk_LDFLAGS}" )
endif()

# scan-dump uses ICU 
# 	http://site.icu-project.org/
# 	http://userguide.icu-project.org/
# 	https://unicode-org.github.io/icu-docs/apidoc/released/icu4c/index.html
pkg_check_modules(icu REQUIRED icu-i18n)
message("icu found=${icu_FOUND} include=${icu_INCLUDE_DIRS} ldflags=${icu_LDFLAGS}" )

# I'm also using the jansson JSON library
pkg_check_modules(jansson REQUIRED jansson)
message("jansson found=${jansson_FOUND} include=${jansson_INCLUDE_DIRS} ldflags=${jansson_LDFLAGS}" )

add_executable(scan-dump scan-dump.c)
target_include_directories(scan-dump PUBLIC ${libnl_INCLUDE_DIRS} ${libnl_genl_INCLUDE_DIRS})
target_link_libraries(scan-dump iw ${libnl_LDFLAGS} ${libnl_genl_LDFLAGS} )
target_link_libraries(scan-dump ${icu_LDFLAGS} icuio)
target_link_libraries(scan-dump jansson)
target_link_libraries(scan-dump ${cpprestsdk_LDFLAGS})

foreach(test IN ITEMS test_ie test_bss test_decode test_bytebuf test_ie_he test_nlnames)
	message("test=${test}")
	add_executable(${test} ${test}.c)
	target_include_directories(${test} PUBLIC ${libnl_INCLUDE_DIRS} ${libnl_genl_INCLUDE_DIRS})
	target_link_libraries(${test} iw ${libnl_LDFLAGS} ${libnl_genl_LDFLAGS} )
	target_link_libraries(${test}  ${icu_LDFLAGS} icuio)
	target_link_libraries(${test} jansson)
endforeach()

add_test(NAME test_ie COMMAND test_ie)
add_test(NAME test_bss COMMAND test_bss)
add_test(NAME test_decode COMMAND test_decode)
add_test(NAME test_bytebuf COMMAND test_bytebuf)
add_test(NAME test_ie_he COMMAND test_ie_he)
add_test(NAME test_nlnames COMMAND test_nlnames)

