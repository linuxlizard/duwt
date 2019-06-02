#ifndef IE_HPP
#define IE_HPP

//#include <iostream>
#include <string>
#include <vector>

#include "json/json.h"

// forward declare class IE so can introduce operator<< before the 'friend' in
// class IE
namespace cfg80211 {
	class IE;
}

std::ostream& operator<<(std::ostream& os, const cfg80211::IE& ie);

namespace cfg80211 {

enum class IE_ID 
{
	// TODO fill the rest of this out (yikes)
	INVALID = -1,
	SSID = 0,
	SUPPORTED_RATES = 1,
	DSSS_PARAMETER_SET = 3,
	VENDOR = 221,
	EXTENSION = 255
};

// base class of 802.11 Information Element
class IE
{
	public:
		IE(uint8_t id, uint8_t len, uint8_t *buf) ; 
		virtual ~IE() = default;

		// https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#cctor-constructors-assignments-and-destructors

		// definitely don't want copy constructors
		IE(const IE&) = delete; // Copy constructor
		IE& operator=(const IE&) = delete;  // Copy assignment operator

		// added move constructors to learn how to use move constructors
		// Move constructor
		IE(IE&& src) 
			: id{src.id}, 
			  len{src.len}, 
			  bytes{std::move(src.bytes)}, 
			  name{src.name},
			  hexdump{std::move(src.hexdump)},
			  logger{std::move(src.logger)}
		{
			// https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#c64-a-move-operation-should-move-and-leave-its-source-in-a-valid-state
			src.id = -1;
			src.len = -1;
			src.name = nullptr;
			src.hexdump.clear();
		};

		// https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#Rc-move-assignment
		IE& operator=(IE&& src) // Move assignment operator
		{
			id = src.id;
			len = src.len;
			bytes = std::move(src.bytes);
			name = src.name;
			hexdump = std::move(src.hexdump);
			logger = std::move(src.logger);

			// https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#c64-a-move-operation-should-move-and-leave-its-source-in-a-valid-state
			src.id = -1;
			src.len = -1;
			src.bytes.clear();
			src.name = nullptr;
			src.hexdump.clear();

			return *this;
		};

		virtual Json::Value make_json(void);

		friend std::ostream& ::operator<< (std::ostream &, const IE&);

		IE_ID get_id(void) { return static_cast<IE_ID>(id); };
		std::string get_id_str(void) { return std::to_string(static_cast<int>(id)); };

	protected:
		uint8_t id;
		uint8_t len;
		std::vector<uint8_t> bytes;

		const char* name;
		std::string hexdump;

		std::shared_ptr<spdlog::logger> logger;

};

class IE_SSID : public IE
{
	public:
		IE_SSID(uint8_t id_, uint8_t len_, uint8_t* buf);

		virtual Json::Value make_json(void);

	protected:
		std::string ssid;

		/* quick notes while I'm thinking of it: SSID could be utf8 
		 * https://www.gnu.org/software/libc/manual/html_node/Extended-Char-Intro.html
		 *
		 * https://utf8everywhere.org/
		 *
		 * https://stackoverflow.com/questions/55641/unicode-processing-in-c
		 * http://site.icu-project.org/ 
		 * dnf info libicu
		 * dnf info libicu-devel
		 *
		 * https://withblue.ink/2019/03/11/why-you-need-to-normalize-unicode-strings.html
		 *
		 * https://stackoverflow.com/questions/402283/stdwstring-vs-stdstring?rq=1
		 */

		// C++ string literals
		// https://en.cppreference.com/w/cpp/language/string_literal
		// "3) UTF-8 encoded string literal. The type of a u8"..." string
		// literal is const char[N] (until C++20) const char8_t[N] (since
		// C++20), where N is the size of the string in UTF-8 code units
		// including the null terminator."
		//
		// General consensus in my reading seems to be use std::string
		// to contain UTF8.

		// more notes:
		// https://stackoverflow.com/questions/4804298/how-to-convert-wstring-into-string
		// 	^^-- uses codecvt_utf8() which is deprecated in c++17
		//
		// https://stackoverflow.com/questions/2573834/c-convert-string-or-char-to-wstring-or-wchar-t/18597384#18597384
		// 
		// <codecvt> is deprecated 
		// http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2017/p0618r0.html

		// Boost to the rescue?
		// http://cppcms.com/files/nowide/html/
		// https://www.boost.org/doc/libs/1_69_0/libs/locale/doc/html/index.html

		// I'm using sqlite in Galileo itself.
		// https://www.sqlite.org/pragma.html#pragma_encoding
};

class IE_SupportedRates : public IE
{
	public:
		IE_SupportedRates(uint8_t id_, uint8_t len_, uint8_t* buf);

		virtual Json::Value make_json(void);

	protected:
		struct Rate {
		public:
			Rate(float rate_, bool basic_):
				rate(rate_),
				basic(basic_) {};
			Json::Value make_json(void)
			{
				Json::Value v {Json::Value(Json::objectValue)};
				v["rate"] = rate;
				v["basic"] = basic;
				return v;
			}

		private:
			float rate;
			bool basic;
		};

		std::vector<struct Rate> rates_list;
};

class IE_Integer : public IE
{
	// IE element that contains just a single 8-bit value as an integer
	public:
		IE_Integer(uint8_t id_, uint8_t len_, uint8_t* buf);

		virtual Json::Value make_json(void);

	protected:
		int value;
};

class IE_Country : public IE
{
	public:
		IE_Country(uint8_t id_, uint8_t len_, uint8_t* buf);

		virtual Json::Value make_json(void);

		enum class Environment {
			INVALID, INDOOR_ONLY, OUTDOOR_ONLY, INDOOR_OUTDOOR
		};

		static const int IEEE80211_COUNTRY_EXTENSION_ID = 201;

		// country IE triplets 
		// I'd use std::variant but I'm limited to C++14
		// I'm trying to make something easy to use, not something memory minimal.
		//
		// (My hat is off to the iw developers who understand the standard's
		// Country element docs.)
		struct Triplet
		{
			static const int IEEE80211_COUNTRY_EXTENSION_ID = 201;

			Triplet(unsigned int n1, unsigned int n2, unsigned int n3);
			Json::Value make_json(void);


			// if (ext.reg_extension_id >= IEEE80211_COUNTRY_EXTENSION_ID) {
			//    use ext
			// else
			//    use chans
			struct {
				unsigned int first_channel;
				unsigned int num_channels;
				unsigned int max_power;
				// not part of the ieee80211_country_ie_triplet but I'm adding
				// it here rather than recalculating it every time
				unsigned int end_channel;
			} chans;
			// OR
			struct {
				unsigned int reg_extension_id;
				unsigned int reg_class;
				unsigned int coverage_class;
			} ext;
		};

	protected:
		std::string country;

		Environment environment;
		// also preserve the original value
		uint8_t environment_byte;

		std::vector<struct Triplet> triplets;
};

class IE_RSN : public IE
{
	public:
		IE_RSN(uint8_t id_, uint8_t len_, uint8_t* buf);

		virtual Json::Value make_json(void);

		// 80211-2016 Table 9-131 page 884
//		enum class Suite {
//			INVALID, // use 0 as uninitialized/invalid
//			USE_GROUP_CIPHER,
//			WEP_40, TKIP, CCMP, WEP_104, BIP_CMAC_128, 
//			GROUP_TRAFFIC_FORBIDDEN,
//			GCMP_128, GCMP_256, CCMP_256, 
//			BIP_GMAC_128, BIP_GMAC_256, BIP_CMAC_256,
//		};

		struct CipherSuite
		{
			// delegate constructor (nifty!)
			CipherSuite() : CipherSuite( {0x00, 0x0f, 0xac, 0x04} ) {}
//			CipherSuite() : CipherSuite(reinterpret_cast<const uint8_t*>("\x00\x0f\0xac\x04")) {}
			CipherSuite(std::array<uint8_t,4> data);
			CipherSuite(const uint8_t* data);
//			Json::Value make_json(void);

			std::array<uint8_t,3> oui;
			uint8_t cipher;

			std::string oui_hex; // hex dump of oui
			std::string cipher_text; // human name of cipher
		};


	protected:
		void decode_rsn_ie(const char *defcipher, const char *defauth, int is_osen);

		unsigned int version;

		struct CipherSuite group_cipher;
		std::vector<struct CipherSuite> pairwise_ciphers;
};

class IE_Vendor : public IE
{
	// IE element that contains a Vendor Specific IE (id=221)
	public:
		IE_Vendor(uint8_t id_, uint8_t len_, uint8_t* buf);

		virtual Json::Value make_json(void);

	protected:
		// http://standards-oui.ieee.org/oui/oui.txt
		// http://standards-oui.ieee.org/oui/oui.csv
		std::string oui;
		std::string orgname;
};

std::shared_ptr<IE> make_ie(uint8_t id, uint8_t len, uint8_t* buf);

}  // end namespace cfg80211

#endif
