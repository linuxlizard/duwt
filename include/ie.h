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
			  decode{std::move(src.decode)},
			  logger{std::move(src.logger)}
		{
			// https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#c64-a-move-operation-should-move-and-leave-its-source-in-a-valid-state
			src.id = -1;
			src.len = -1;
			src.name = nullptr;
		};

		// https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#Rc-move-assignment
		IE& operator=(IE&& src) // Move assignment operator
		{
			id = src.id;
			len = src.len;
			bytes = std::move(src.bytes);
			name = src.name;
			decode = std::move(src.decode);
			logger = std::move(src.logger);

			// https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#c64-a-move-operation-should-move-and-leave-its-source-in-a-valid-state
			src.id = -1;
			src.len = -1;
			src.bytes.clear();
			src.name = nullptr;

			return *this;
		};

		virtual Json::Value make_json(void);

		friend std::ostream& ::operator<< (std::ostream &, const IE&);

		uint8_t get_id(void) { return id; };

		// learning from Kismet's trackedelement.h
		using string_vec_t = std::vector<std::string>;
		using const_iterator = typename string_vec_t::const_iterator;

		const_iterator cbegin() const { return std::cbegin(decode); }
		const_iterator cend() const { return std::cend(decode); }

	protected:
		uint8_t id;
		uint8_t len;
		std::vector<uint8_t> bytes;

		const char* name;

		std::vector<std::string> decode;

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
		std::vector<int> rates;
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

std::shared_ptr<IE> make_ie(uint8_t id, uint8_t len, uint8_t* buf);

}  // end namespace cfg80211

#endif
