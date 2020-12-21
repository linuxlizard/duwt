#include <iostream>
#include <thread>
#include <string>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "Poco/Net/HTTPServer.h"
#include "Poco/Net/HTTPRequestHandler.h"
#include "Poco/Net/HTTPRequestHandlerFactory.h"
#include "Poco/Net/HTTPServerRequest.h"
#include "Poco/Net/HTTPServerResponse.h"
#include "Poco/Net/ServerSocket.h"
#include "Poco/Util/ServerApplication.h"

#include "mimetypes.h"
#include "oui.h"
#include "scan.h"
#include "fs.h"

using namespace Poco;
using namespace Poco::Net;
using namespace Poco::Util;

mimetypes all_mimetypes;

class HelloRequestHandler: public HTTPRequestHandler
{
	void handleRequest(HTTPServerRequest& request, HTTPServerResponse& response)
	{
		Application& app = Application::instance();
		app.logger().information("Request from %s", request.clientAddress().toString());

		// fills out the response object on an error
		auto error = [&response] {
			response.setStatusAndReason(HTTPResponse::HTTP_NOT_FOUND);
			response.setContentLength(0);
			response.send();
		};

		// is this a file in our allowed location?
		std::string uri = request.getURI();
		app.logger().debug("request for URI=%s", uri);
		app.logger().warning("request for URI=%s", uri);

		fs::path root = fs::absolute("files");
		std::string root_str = root.string();
		fs::path path { root };
		path += uri;

		try {
			path = fs::canonical(path);
		} 
		catch (fs::filesystem_error& err) {
			// https://en.cppreference.com/w/cpp/filesystem/filesystem_error
			app.logger().error("invalid absolute path uri=%s", uri);
			error();
			return;
		};

		// is this path within our valid location?  (prevent relative path shenanigans)
		std::string path_str { path.string() };
		if ( path_str.substr(0, root_str.size()) != root_str) {
			app.logger().error("request for invalid path uri=%s", path_str);
			error();
			return;
		}

		// is this actually a file?
		if (! fs::is_regular_file(path)) {
			app.logger().error("uri=%s not a file", uri);
			error();
			return;
		}

		// look up file extension
		std::string content_type;
		try {
			content_type = all_mimetypes.at(path.extension());
		} catch (std::out_of_range& err) {
		}

		response.sendFile(path_str, content_type);
	}
};

class HelloRequestHandlerFactory: public HTTPRequestHandlerFactory
{
	HTTPRequestHandler* createRequestHandler(const HTTPServerRequest&)
	{
		return new HelloRequestHandler;
	}
};

class WebServerApp: public ServerApplication
{
	void initialize(Application& self)
	{
		loadConfiguration();
		ServerApplication::initialize(self);
	}

	int main(const std::vector<std::string>& args)
	{
		UInt16 port = static_cast<UInt16>(config().getUInt("port", 8080));

		for (auto a : args) {
			std::cout << "arg=" << a << "\n";
		}

		Logger& rlogger = Logger::root();

//		Poco::Logger::root().setLevel(Poco::Message::PRIO_DEBUG);
		std::cout << "level=" << rlogger.getLevel() << "\n";
		rlogger.setLevel(Poco::Message::PRIO_DEBUG);
		std::cout << "level=" << rlogger.getLevel() << "\n";
		rlogger.error("this is an error message");
		rlogger.warning("this is an warning message");
		rlogger.information("this is an information message");
		rlogger.notice("this is an notice message");
		rlogger.debug("this is an debug message");

		// this seems really weird+verbose but enables debug messages up in the
		// RequestHandler
//		Application::instance().logger().setLevel(Poco::Message::PRIO_DEBUG);
		Poco::Logger::setLevel("", Message::PRIO_DEBUG);

		all_mimetypes = mimetype_parse_default_file();

		ScanningThread scanner;

		HTTPServer srv(new HelloRequestHandlerFactory, port);
		srv.start();
		logger().information("HTTP Server started on port %hu.", port);
		waitForTerminationRequest();
		logger().information("Stopping HTTP Server...");
		srv.stop();

		scanner.stop();
		scanner.join();

		return Application::EXIT_OK;
	}

private:

};

POCO_SERVER_MAIN(WebServerApp)
