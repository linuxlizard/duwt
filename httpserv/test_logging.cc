#include <iostream>
#include <sstream>
#include <vector>

#include "Poco/Logger.h"
#include "Poco/ConsoleChannel.h"
#include "Poco/StreamChannel.h"

using Poco::Logger;
using Poco::Message;

// newer versions of Poco have this
#ifndef poco_information_f
#define poco_information_f(logger, fmt, ...) \
	if ((logger).information()) (logger).information(Poco::format((fmt), __VA_ARGS__), __FILE__, __LINE__); else (void) 0
#endif

// https://www.fluentcpp.com/2017/04/21/how-to-split-a-string-in-c/ 
std::vector<std::string> split(const std::string& s, char delimiter)
{
   std::vector<std::string> tokens;
   std::string token;
   std::istringstream tokenStream(s);
   while (std::getline(tokenStream, token, delimiter))
   {
      tokens.push_back(token);
   }
   return tokens;
}

std::vector<std::string> split(const std::string& s)
{
   std::vector<std::string> tokens;
   std::string token;
   std::istringstream tokenStream(s);
   while (std::getline(tokenStream, token, ' '))
   {
      tokens.push_back(token);
   }
   return tokens;
}

int main(void)
{
	std::cout << "hello, world\n";
	Logger& root = Logger::root();
//	root.open();
	root.setChannel(new Poco::StreamChannel(std::cout));
	std::cout << "level=" << root.getLevel() << "\n";
	root.information("This is an Information message");
	root.warning("This is a Warning message");
	root.error("This is an Error message");

	poco_warning(root, "Another warning");
	poco_information(root, Poco::format("information with args: %d %d", 42, 54));
	poco_information_f(root, "information with args: %d %d", 42, 54);

	Logger::get("ConsoleLogger").error("Another error message");

//	Logger& logger = Logger::create("iw", 

	return 0;
}

