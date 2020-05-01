// gcc -g -Wall -Wpedantic -Wextra -o test_uri test_uri.cc -lstdc++ -std=c++17 -lPocoFoundation
#include "Poco/URI.h"
#include <iostream>

int main(int argc, char** argv)
{
    Poco::URI uri1("http://www.appinf.com:88/sample?example-query#frag");

    std::string scheme(uri1.getScheme()); // "http"
    std::string auth(uri1.getAuthority()); // "www.appinf.com:88"
    std::string host(uri1.getHost()); // "www.appinf.com"
    unsigned short port = uri1.getPort(); // 88
    std::string path(uri1.getPath()); // "/sample"
    std::string query(uri1.getQuery()); // "example-query"
    std::string frag(uri1.getFragment()); // "frag"
    std::string pathEtc(uri1.getPathEtc()); // "/sample?example-query#frag"

	for (int i=1 ; i<argc ; i++) {
		Poco::URI uri2(argv[i]);
		std::cout << "scheme=" << uri2.getScheme() << "\n";
		std::cout << "port=" << uri2.getPort() << "\n";
		std::cout << "path=" << uri2.getPath() << "\n";
	}

    return 0;
}
