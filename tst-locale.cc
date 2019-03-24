/* tinkering with whar_t
 *  gcc -std=c++17 -g -Wall -Wextra -Wshadow -pedantic -o tst tst.cc -lstdc++ -lboost_locale
 */

#include <iostream>
#include <string>
#include <codecvt>
#include <clocale>
#include <cstring>
#include <cwchar>

#include <boost/locale.hpp>
#include <boost/algorithm/string/case_conv.hpp>

// notes: https://stackoverflow.com/questions/4804298/how-to-convert-wstring-into-string
// 			^^-- uses codecvt_utf8() which is deprecated in c++17
// https://stackoverflow.com/questions/2573834/c-convert-string-or-char-to-wstring-or-wchar-t/18597384#18597384
// 
class SSID
{
	public:
		SSID(std::wstring s)
			: name{s}
		{};

	private:
		std::wstring name;
};

// from https://en.cppreference.com/w/cpp/string/multibyte/mbrtowc
void print_mb(const char* ptr)
{
	std::mbstate_t state = std::mbstate_t(); // initial state
	const char* end = ptr + std::strlen(ptr);
	int len;
	wchar_t wc;
	while((len = std::mbrtowc(&wc, ptr, end-ptr, &state)) > 0) {
		std::wcout << "Next " << len << " bytes are the character " << wc << '\n';
		ptr += len;
	}
}

int main(void)
{
//	std::wstring sw { U"Académie française" };   // doesn't compile
//	std::wcout << sw << "\n";

	std::u32string s32 { U"Académie française" };
//	std::wcout << s32 << "\n";

//	SSID ssid { "Académie française" };
//	SSID ssid { U"Académie française" };
//	ssid = SSID(u8"(╯°□°）╯︵ ┻━┻")
//	ssid = SSID(u8"💩")

	// How should I represent SSID? It comes to me as a possibly UTF8 encoded
	// blob. u8string isn't until C++20
	//
	std::u16string s16 { u"Académie française" };
//	std::wcout << s16 << "\n";

	// "NULL Terminated Multibyte Strings (NTMBS)"
	// via https://en.cppreference.com/w/cpp/string/multibyte/mbrtowc
	std::setlocale(LC_ALL, "en_US.utf8");

	const char* str = u8"z\u00df\u6c34\U0001d10b"; // or u8"zß水𝄋"
							  // or "\x7a\xc3\x9f\xe6\xb0\xb4\xf0\x9d\x84\x8b";
	print_mb(str);
	print_mb(u8"(╯°□°）╯︵ ┻━┻");
	print_mb(u8"💩");

	// <codecvt> is deprecated let's play with libicu
	// boost::locale? wraps libicu
	// https://www.boost.org/doc/libs/1_69_0/libs/locale/doc/html/index.html
	// "In order to achieve this goal Boost.Locale uses the-state-of-the-art
	// Unicode and Localization library: ICU - International Components for
	// Unicode."

	// dnf info boost-locale

	// https://www.boost.org/doc/libs/1_69_0/libs/locale/doc/html/whello_8cpp-example.html
    using namespace boost::locale;
    using namespace std;
    
    // Create system default locale
    generator gen;
    locale loc=gen(""); 
    locale::global(loc);
    wcout.imbue(loc);
    
    // This is needed to prevent C library to
    // convert strings to narrow 
    // instead of C++ on some platforms
    std::ios_base::sync_with_stdio(false);
    
    wcout <<wformat(L"Today {1,date} at {1,time} we had run our first localization example") % time(0) 
          <<endl;
   
    wcout<<L"This is how we show numbers in this locale "<<as::number << 103.34 <<endl; 
    wcout<<L"This is how we show currency in this locale "<<as::currency << 103.34 <<endl; 
    wcout<<L"This is typical date in the locale "<<as::date << std::time(0) <<endl;
    wcout<<L"This is typical time in the locale "<<as::time << std::time(0) <<endl;
    wcout<<L"This is upper case "<<to_upper(L"Hello World!")<<endl;
    wcout<<L"This is lower case "<<to_lower(L"Hello World!")<<endl;
    wcout<<L"This is title case "<<to_title(L"Hello World!")<<endl;
    wcout<<L"This is fold case "<<fold_case(L"Hello World!")<<endl;


	// https://www.boost.org/doc/libs/1_69_0/libs/locale/doc/html/wconversions_8cpp-example.html
    wcout<<L"Correct case conversion can't be done by simple, character by character conversion"<<endl;
    wcout<<L"because case conversion is context sensitive and not 1-to-1 conversion"<<endl;
    wcout<<L"For example:"<<endl;
    wcout<<L"   German grüßen correctly converted to "<<to_upper(L"grüßen")<<L", instead of incorrect "
                    <<boost::to_upper_copy(std::wstring(L"grüßen"))<<endl;
    wcout<<L"     where ß is replaced with SS"<<endl;
    wcout<<L"   Greek ὈΔΥΣΣΕΎΣ is correctly converted to "<<to_lower(L"ὈΔΥΣΣΕΎΣ")<<L", instead of incorrect "
                    <<boost::to_lower_copy(std::wstring(L"ὈΔΥΣΣΕΎΣ"))<<endl;
    wcout<<L"     where Σ is converted to σ or to ς, according to position in the word"<<endl;
    wcout<<L"Such type of conversion just can't be done using std::toupper that work on character base, also std::toupper is "<<endl;
    wcout<<L"not fully applicable when working with variable character length like in UTF-8 or UTF-16 limiting the correct "<<endl;
    wcout<<L"behavoir to BMP or ASCII only"<<endl;

	// https://www.boost.org/doc/libs/1_69_0/libs/locale/doc/html/wboundary_8cpp-example.html
    wstring text=L"Hello World! あにま! Linux2.6 and Windows7 is word and number. שָלוֹם עוֹלָם!";
    wcout<<text<<endl;
    boundary::wssegment_index index(boundary::word,text.begin(),text.end());
    boundary::wssegment_index::iterator p,e;
    for(p=index.begin(),e=index.end();p!=e;++p) {
        wcout<<L"Part ["<<*p<<L"] has ";
        if(p->rule() & boundary::word_number)
            wcout<<L"number(s) ";
        if(p->rule() & boundary::word_letter)
            wcout<<L"letter(s) ";
        if(p->rule() & boundary::word_kana)
            wcout<<L"kana character(s) ";
        if(p->rule() & boundary::word_ideo)
            wcout<<L"ideographic character(s) ";
        if(p->rule() & boundary::word_none)
            wcout<<L"no word characters";
        wcout<<endl;
    }
    index.map(boundary::character,text.begin(),text.end());
    for(p=index.begin(),e=index.end();p!=e;++p) {
        wcout<<L"|" <<*p ;
    }
    wcout<<L"|\n\n";
    index.map(boundary::line,text.begin(),text.end());
    for(p=index.begin(),e=index.end();p!=e;++p) {
        wcout<<L"|" <<*p ;
    }
    wcout<<L"|\n\n";
    index.map(boundary::sentence,text.begin(),text.end());
    for(p=index.begin(),e=index.end();p!=e;++p) {
        wcout<<L"|" <<*p ;
    }
    wcout<<"|\n\n";
	return 0;
}

