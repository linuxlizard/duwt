#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <memory>
#include <iomanip>

#include <linux/netlink.h>
#include <netlink/attr.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/ctrl.h>
#include <linux/nl80211.h>

#include "json/json.h"

#include "bss.h"
#include "bss_json.h"
//#include "nlnames.h"

static const uint32_t SCAN_DUMP_COOKIE = 0x64617665;

class BSSPointer
{
public:
	BSSPointer(struct BSS* p) { bss=p; };
	~BSSPointer() { bss_free(&bss); bss=nullptr; };

	const struct BSS* get(void) { return bss; };

private:
	struct BSS* bss;
};

void parse(std::istream& infile, std::vector<std::unique_ptr<BSSPointer>>& bss_list)
{
	if (infile.fail()) {
		throw std::runtime_error("file open failed");
	}

	char c;
	do {
		c = infile.get();
		if (infile.fail()) {
			throw std::runtime_error("file fail bit set");
		}
	} while(c != 0x0a);

	while ( true ) { 
		uint32_t size, cookie;
		infile.read((char *)&cookie, sizeof(uint32_t));

		if (infile.eof() ) {
			break;
		}

		infile.read((char *)&size, sizeof(uint32_t));
		if (infile.fail()) {
			throw std::runtime_error("premature end of file");
		}
		if (cookie != SCAN_DUMP_COOKIE) {
			throw std::runtime_error("file format error; missing magic");
		}
		if (size > 4095) {
			throw std::runtime_error("file format error; invalid size");
		}

		uint8_t buf[4096];
		infile.read((char *)buf, size);
		if (infile.fail()) {
			throw std::runtime_error("not enough data for size");
		}

		int attrlen = (int) size;
		struct nlattr* attr = (struct nlattr*)buf;
		struct nlattr* tb_msg[NL80211_ATTR_MAX + 1];
		int err = nla_parse(tb_msg, NL80211_ATTR_MAX, attr, attrlen, NULL);

	//	peek_nla_attr(tb_msg, NL80211_ATTR_MAX);

		struct BSS* bss = NULL;
		err = bss_from_nlattr(tb_msg, &bss);

		printf("bss=%p\n", bss);

		bss_list.push_back(std::make_unique<BSSPointer>(bss));
	}
}

int main(void)
{
	std::cout << "hello, world\n";

	std::ifstream infile;
	infile.open("scan-dump.dat", std::ios_base::binary|std::ios_base::in);

	std::vector<std::unique_ptr<BSSPointer>> bss_list;

	parse(infile, bss_list);
	infile.close();

	for (auto& bss_ptr: bss_list) {
		const struct BSS* bss = bss_ptr.get()->get();

		Json::Value obj = bss_to_json(bss);
//		std::ostringstream str;
		Json::StreamWriterBuilder builder;
		builder["indentation"] = "";
		const std::string json_file = Json::writeString(builder, obj);
//		obj.stringify(str);
		std::cout << json_file << "\n";
//		obj.stringify(std::cout, 5);
	}

	return 0;
}


