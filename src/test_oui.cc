#include <iostream>
#include <iomanip>
#include <sstream>
#include <chrono>
#include <cstddef>

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

	// Marvell, my N-1th job
	num32 = 0x00005043;
	org = oui.get_org_name(num32);
	INFO("MRVL==" << org);
	REQUIRE(org == "MARVELL SEMICONDUCTOR, INC.");

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


//	std::byte qcom_oui[3] { std::byte{0x8c},std::byte{0xfd}, std::byte{0xf0}};
//	org = oui.get_org_name(qcom_oui);
	org = oui.get_org_name(0x8cfdf0);
	INFO("QCOM==" << org );

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


