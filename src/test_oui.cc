#include <iostream>

#include "oui.h"
//#include "catch.hpp"

//TEST_CASE("Extended Systems", "[oui]") {
//	REQUIRE( ieeeoui::get_org_name(0x00004068) == "EXTENDED SYSTEMS");
//}

void test(ieeeoui::OUI& oui)
{
	std::string org;

	// Extended Systems, my first job
	org = oui.get_org_name(0x00004068);

	// Marvell, my N-1th job
	org = oui.get_org_name(0x00005043);

	// Microsoft
	org = oui.get_org_name(0x000050f2);

	unsigned char ms_oui[3] { 0x00, 0x50, 0xf2 };
	org = oui.get_org_name(ms_oui);

}

int main(void)
{
	try {
		ieeeoui::OUI oui {"dave.csv"};
	}
	catch (const ieeeoui::OUIException& err) {
	}

	try { 
		ieeeoui::OUI oui {"oui.csv"};
		test(oui);
	}
	catch (const ieeeoui::OUIException& err) {
		std::cerr << "failed to find oui.csv in current directory\n";
	}

	// try parent directory (for cases where building in a subdir because
	// cmake)
	try {
		ieeeoui::OUI oui {"../oui.csv"};
		test(oui);
	}
	catch (const ieeeoui::OUIException& err) {
		std::cerr << "failed to find oui.csv in current directory\n";
		throw;
	}

	return 0;
}

