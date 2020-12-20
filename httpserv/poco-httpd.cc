#include <iostream>
#include <thread>
#include <string>
#include <optional>

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

using namespace Poco;
using namespace Poco::Net;
using namespace Poco::Util;

mimetypes all_mimetypes;

static std::optional<std::string> make_absolute(std::string path)
{
	char abspath[PATH_MAX+1];

	char* p = realpath(path.c_str(), abspath);
	if (!p) {
		return std::nullopt;
	}

	return std::string { abspath };
}

static bool is_file(std::string& path)
{
	struct stat statbuf;

	int ret = stat(path.c_str(), &statbuf);
	if (ret != 0) {
		return false;
	}

	if ( (statbuf.st_mode & S_IFREG) != S_IFREG ) {
		return false;
	}

	return true;
}

class HelloRequestHandler: public HTTPRequestHandler
{
	void handleRequest(HTTPServerRequest& request, HTTPServerResponse& response)
	{
		Application& app = Application::instance();
		app.logger().information("Request from %s", request.clientAddress().toString());

		std::string uri = request.getURI();
		app.logger().debug("request for URI=%s", uri);
		app.logger().warning("request for URI=%s", uri);

		Poco::Path path {false}; // path will be relative
		path.pushDirectory("files");
		path.setFileName(uri);

		// poco absolute()/makeAbsolute() don't do what I hope it would do
//		path = path.absolute();
//		path.makeAbsolute();
		// so let's do this the posix-y way
		auto path_absolute = make_absolute(path.toString(Poco::Path::PATH_UNIX));
		if ( !path_absolute.has_value() ) {
			app.logger().error("invalid absolute path uri=%s", uri);
			response.setStatusAndReason(HTTPResponse::HTTP_NOT_FOUND);
			response.setContentLength(0);
			response.send();
			return;
		}

		std::string path_str = path_absolute.value();
		app.logger().information("file=%s", path_str);

		// is uri path under current dir?
		std::string cwd { Poco::Path::current() };
		app.logger().information("cwd=%s substr=%s", cwd, path_str.substr(0, cwd.size()) );

		if ( path_str.substr(0, cwd.size()) != cwd) {
			app.logger().error("request for invalid path uri=%s", path_str);
			response.setStatusAndReason(HTTPResponse::HTTP_NOT_FOUND);
			response.setContentLength(0);
			response.send();
			return;
		}

		if (!is_file(path_str)) {
			app.logger().error("uri=%s not a file", uri);
			response.setStatusAndReason(HTTPResponse::HTTP_NOT_FOUND);
			response.setContentLength(0);
			response.send();
			return;
		}

		std::string content_type;
		try {
			content_type = all_mimetypes.at(path.getExtension());
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
