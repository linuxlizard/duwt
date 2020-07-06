#include <iostream>
#include <iomanip>
#include <sstream>
#include <chrono>

#define CATCH_CONFIG_MAIN
//#define CATCH_CONFIG_RUNNER
#include "catch.hpp"

#include "oui.h"

//TEST_CASE("Extended Systems", "[oui]") {
//	REQUIRE( ieeeoui::get_org_name(0x00004068) == "EXTENDED SYSTEMS");
//}

void test(ieeeoui::OUI& oui)
{
	std::string org;
	uint32_t num32;

	// Extended Systems, my first job
	num32 = 0x00004068;
	org = oui.get_org_name(num32);
	INFO("ESI==" << org);
	REQUIRE(org == "EXTENDED SYSTEMS");
//	std::cout << ieeeoui::oui_to_string(num32) << " == " << org << "\n";

	// Marvell, my N-1th job
	num32 = 0x00005043;
	org = oui.get_org_name(num32);
	INFO("MRVL==" << org);
	REQUIRE(org == "MARVELL SEMICONDUCTOR, INC.");
//	std::cout << ieeeoui::oui_to_string(num32) << " == " << org << "\n";

	// https://stackoverflow.com/questions/22387586/measuring-execution-time-of-a-function-in-c
	auto start = std::chrono::high_resolution_clock::now();

	// Microsoft (loves Linux!)
	org = oui.get_org_name(0x000050f2);

	auto end = std::chrono::high_resolution_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::microseconds>( end - start ).count();
	INFO("MSFT==" << org);
	REQUIRE(org=="MICROSOFT CORP.");
	std::cout << org << " duration=" << duration << "\n";

	// another way to format Microsoft
	// should be a faster lookup now
	unsigned char ms_oui[3] { 0x00, 0x50, 0xf2 };
	start = std::chrono::high_resolution_clock::now();
	org = oui.get_org_name(ms_oui);
	end = std::chrono::high_resolution_clock::now();
	duration = std::chrono::duration_cast<std::chrono::microseconds>( end - start ).count();
	REQUIRE(org=="MICROSOFT CORP.");
	std::cout << org << " duration=" << duration << "\n";

	// this key does not exist
	try { 
		num32 = 0x00112233;
		org = oui.get_org_name(num32);
		// if we reach this point, we failed
		REQUIRE(false);
	} 
	catch (std::out_of_range& err) {
		// expected to fail so ignore
	}


	std::cout << org << " duration=" << duration << "\n";

}

// "Premature optimization is the root of all evil." I'm converting a subset of
// hex to integer so am I faster than strtoul()?  Turns out, yes. But.
//
// godbolt.org shows my loop is being unrolled. I'm assuming a subset of hex
// chars (no lowercase) which also simplifies the conversion.
//
// Pulling this code out into a simple tst.cc with main() that reads a value
// from argv[1], I get:
//
// gcc -g -O3 -Wall -Wpedantic -o tst tst.cc -std=c++17 -lstdc++ 
//
// % ./tst 004068
// duration=10
// lookup 0x004068
// duration=24372
// strtoul 0x004068
//
static const std::array<uint8_t,32> hexlut = 
{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, // '0' through '9'
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, // ignored
 10, 11, 12, 13, 14, 15 // 'A' through 'F'
};

static uint32_t string_to_oui(std::string& s)
{
	uint32_t num32=0;
	for (size_t i=0 ; i<6 ; i++) {
		uint8_t n = hexlut.at(static_cast<int>(s[i]-'0'));
		num32 |= n;
		num32 <<= 4;
	}
	num32 >>= 4;
	return num32;
}

void prove_to_myself_my_code_is_faster(std::string& s)
{
	uint32_t num32;

	auto start = std::chrono::high_resolution_clock::now();
	for (int i=0 ; i<100000 ; i++ ) {
		num32  = string_to_oui(s);
	}
	auto end = std::chrono::high_resolution_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::microseconds>( end - start ).count();
	std::cout << "duration=" << duration << "\n";
	std::clog << "lookup 0x" << std::setw(6) << std::setfill('0') << std::hex << num32 << "\n";
    
	start = std::chrono::high_resolution_clock::now();
	for (int i=0 ; i<100000 ; i++ ) {
		num32 = strtoul(s.c_str(), nullptr, 16);
	}
	end = std::chrono::high_resolution_clock::now();
	duration = std::chrono::duration_cast<std::chrono::microseconds>( end - start ).count();
	std::cout << "duration=" << duration << "\n";
	std::clog << "strtoul 0x" << std::setw(6) << std::setfill('0') << std::hex << num32 << "\n";
}

TEST_CASE("Conversion", "[conversion]") {
	REQUIRE(ieeeoui::oui_to_string(0x112233) == "0X112233");
	REQUIRE(ieeeoui::string_to_oui(ieeeoui::oui_to_string(0x112233)) == 0x112233);
	REQUIRE(ieeeoui::oui_to_string(0xABCDEF) == "0XABCDEF");
	REQUIRE(ieeeoui::string_to_oui(ieeeoui::oui_to_string(0xabcdef)) == 0xabcdef);
	REQUIRE(ieeeoui::oui_to_string(0xabcdef) == "0XABCDEF");
	REQUIRE(ieeeoui::string_to_oui(ieeeoui::oui_to_string(0xabcdef)) == 0xabcdef);
	REQUIRE(ieeeoui::string_to_oui("hello, world") == 0);
}

TEST_CASE("CSV File Missing", "[csv][!shouldfail]") {
	SECTION("non-existent file") {
		try {
			ieeeoui::OUI_CSV oui {"dave.csv"};
			REQUIRE(false);
		}
		catch (const ieeeoui::OUIException& err) {
			// file should not exist so should fail
			throw;
		}
	}
}

TEST_CASE("CSV File Maybe", "[csv][!mayfail]") {
	SECTION("this CSV file might not exist") {
		ieeeoui::OUI_CSV oui {"oui.csv"};
		std::string org;
		uint32_t num32;

		// Extended Systems, my first job
		num32 = 0x00004068;
		org = oui.get_org_name(num32);
		REQUIRE(org == "EXTENDED SYSTEMS");
		// run it again (should be faster this time now that the pump is primed)
		org = oui.get_org_name(num32);
		REQUIRE(org == "EXTENDED SYSTEMS");

		// Marvell, my N-1th job
		num32 = 0x00005043;
		org = oui.get_org_name(num32);
		REQUIRE(org == "MARVELL SEMICONDUCTOR, INC.");
		org = oui.get_org_name(num32);
		REQUIRE(org == "MARVELL SEMICONDUCTOR, INC.");

		// another way to format Microsoft
		// should be a faster lookup now
		unsigned char ms_oui[3] { 0x00, 0x50, 0xf2 };
		org = oui.get_org_name(ms_oui);
		REQUIRE(org == "MICROSOFT CORP.");

	}
}

TEST_CASE("CSV Files", "[csv]") {
	SECTION("this CSV file should exist") {
		ieeeoui::OUI_CSV oui {"../oui.csv"};
		std::string org;
		uint32_t num32;

		// Extended Systems, my first job
		num32 = 0x00004068;
		org = oui.get_org_name(num32);
		REQUIRE(org == "EXTENDED SYSTEMS");
		// run it again (should be faster this time now that the pump is primed)
		org = oui.get_org_name(num32);
		REQUIRE(org == "EXTENDED SYSTEMS");

		// Marvell, my N-1th job
		num32 = 0x00005043;
		org = oui.get_org_name(num32);
		REQUIRE(org == "MARVELL SEMICONDUCTOR, INC.");
		org = oui.get_org_name(num32);
		REQUIRE(org == "MARVELL SEMICONDUCTOR, INC.");

		// another way to format Microsoft
		// should be a faster lookup now
		unsigned char ms_oui[3] { 0x00, 0x50, 0xf2 };
		org = oui.get_org_name(ms_oui);
		REQUIRE(org == "MICROSOFT CORP.");

		test(oui);
	}
}

TEST_CASE("MA Parse", "[ma]") {
	SECTION("test MA file") {
		INFO("testing MA parse");
		// if this fails, install the hwdata package
		std::string path = "/usr/share/hwdata/oui.txt";
		ieeeoui::OUI_MA oui { path };
		test(oui);
	}

}


