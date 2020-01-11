#include <iostream>
#include <fstream>
#include <unordered_map>
#include <string>
#include <filesystem>
#include <vector>
#include <sstream>
#include <ctime>

#include <boost/locale.hpp>
#include <boost/algorithm/string/case_conv.hpp>

namespace fs = std::filesystem;
 
std::vector<char> load_file(fs::path path)
{
	std::ifstream infile(path, std::ios::binary);

	std::vector<char> buf ;
	buf.reserve(fs::file_size(path));

	buf.resize(buf.capacity());
	infile.read(buf.data(), buf.capacity());

	return buf;
}

void test_loc(void)
{
	// https://www.boost.org/doc/libs/1_72_0/libs/locale/doc/html/hello_8cpp-example.html
	boost::locale::generator gen;

	std::locale loc=gen(""); 
	// Create system default locale

	std::locale::global(loc); 
	// Make it system global

	std::cout.imbue(loc);
	// Set as default locale for output
    
	std::cout <<
		boost::locale::format("Today {1,date} at {1,time} we had run our first localization example") % time(0) <<
		std::endl;
   
    std::cout<<"This is how we show numbers in this locale "<< boost::locale::as::number << 103.34 <<std::endl; 
    std::cout<<"This is how we show currency in this locale "<< boost::locale::as::currency << 103.34 <<std::endl; 
    std::cout<<"This is typical date in the locale "<<boost::locale::as::date << std::time(0) <<std::endl;
    std::cout<<"This is typical time in the locale "<<boost::locale::as::time << std::time(0) <<std::endl;
    std::cout<<"This is upper case "<<boost::locale::to_upper("Hello World!")<<std::endl;
    std::cout<<"This is lower case "<<boost::locale::to_lower("Hello World!")<<std::endl;
    std::cout<<"This is title case "<<boost::locale::to_title("Hello World!")<<std::endl;
    std::cout<<"This is fold case "<<boost::locale::fold_case("Hello World!")<<std::endl;
   
	// https://www.boost.org/doc/libs/1_72_0/libs/locale/doc/html/conversions_8cpp-example.html
	std::cout<<"   German grüßen correctly converted to "<< boost::locale::to_upper("grüßen")<<", instead of incorrect "
                    <<boost::to_upper_copy(std::string("grüßen"))<<std::endl;
	std::cout<<"     where ß is replaced with SS"<<std::endl;
	std::cout<<"   Greek ὈΔΥΣΣΕΎΣ is correctly converted to "<<boost::locale::to_lower("ὈΔΥΣΣΕΎΣ")<<", instead of incorrect "
                    <<boost::to_lower_copy(std::string("ὈΔΥΣΣΕΎΣ"))<<std::endl;
}
int main(int argc, char* argv[])
{
//    std::unordered_map<std::string, std::string> myMap;
//    auto foo = myMap.insert_or_assign("a", "apple"     );
//    foo = myMap.insert_or_assign("b", "bannana"   );
//    foo = myMap.insert_or_assign("c", "cherry"    );
//    auto bar = myMap.insert(std::make_pair("c", "clementine"));
// 
//    for (const auto &pair : myMap) {
//        std::cout << pair.first << " : " << pair.second << '\n';
//    }

	fs::path path;

	fs::path root = fs::absolute("build");
	std::string root_str = root.string();
	std::cout << "root=" << root << "\n";

	for (int i=1 ; i<argc ; i++) {
		path = root;
		path /= argv[i];
		std::cout << "path=" << path << "\n";
		try {
			path = fs::canonical(path);
		} 
		catch (fs::filesystem_error& err) {
			// https://en.cppreference.com/w/cpp/filesystem/filesystem_error
			std::cerr << err.what() << "\n";
			continue;
		};

		std::string pp = path.parent_path().string().substr(0,root_str.length());

		std::cout << "path=" << path << " pp=" << pp << "\n";

		size_t count = pp.length();

		std::ostringstream page;
		page << "<html><body>Found "<<
			count <<
			" BSSID</body></html>\n" ;

		std::cout << page.str() << "\n";
		printf("%s\n", page.str().c_str());

		if (pp == root && fs::is_regular_file(path)) {
			std::vector<char> buf = load_file(path);
			printf("%p\n", (void*)buf.data());
			std::cout << buf.size() << " ok\n";
		}
		else {
			std::cout << " bad!\n";
		}
	}

	// fiddling with utf8 + unicode + locale
	// https://utf8everywhere.org/
	std::string s { u8"∃y ∀x ¬(x ≺ y)" };
	std::cout << s << "\n";

	test_loc();
}
