#include <iostream>
#include <thread>

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
		app.logger().information("file=%s", path.toString(Poco::Path::PATH_UNIX));

		std::string ext = path.getExtension();
		std::string content_type;
		try {
			content_type = all_mimetypes.at(ext);
		} catch (std::out_of_range& err) {
		}

		response.sendFile(path.toString(Poco::Path::PATH_UNIX), content_type);
		return;

		response.setChunkedTransferEncoding(true);
		response.setContentType("text/html");

		response.send()
			<< "<html>"
			<< "<head><title>Hello</title></head>"
			<< "<body><h1>Hello from the POCO Web Server</h1></body>"
			<< "</html>\n";
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
