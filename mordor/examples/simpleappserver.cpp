// Copyright (c) 2010 - Mozy, Inc.

#include "../predef.h"

#include <iostream>

#include "../config.h"
#include "../http/server.h"
#include "../iomanager.h"
#include "../main.h"
#include "../socket.h"
#include "../streams/memory.h"
#include "../streams/socket.h"
#include "../streams/transfer.h"

using namespace Mordor;

static std::map<URI, MemoryStream::ptr> g_state;

static void httpRequest(HTTP::ServerRequest::ptr request)
{
    const std::string &method = request->request().requestLine.method;
    const URI &uri = request->request().requestLine.uri;

    if (method == HTTP::GET || method == HTTP::HEAD) {
        std::map<URI, MemoryStream::ptr>::iterator it =
            g_state.find(uri);
        if (it == g_state.end()) {
            HTTP::respondError(request, HTTP::NOT_FOUND);
        } else {
            MemoryStream::ptr copy(new MemoryStream(it->second->buffer()));
            HTTP::respondStream(request, copy);
        }
    } else if (method == HTTP::PUT) {
        MemoryStream::ptr stream(new MemoryStream());
        transferStream(request->requestStream(), stream);
        g_state[uri] = stream;
        HTTP::respondError(request, HTTP::OK);
    } else {
        HTTP::respondError(request, HTTP::METHOD_NOT_ALLOWED);
    }
}

MORDOR_MAIN(int argc, char *argv[])
{
    try {
        Config::loadFromEnvironment();
        IOManager ioManager;
        Socket s(ioManager, AF_INET, SOCK_STREAM);
        IPv4Address address(INADDR_ANY, 80);

        s.bind(address);
        s.listen();

        while (true) {
            Socket::ptr socket = s.accept();
            Stream::ptr stream(new SocketStream(socket));
            HTTP::ServerConnection::ptr conn(new HTTP::ServerConnection(stream,
                &httpRequest));
            conn->processRequests();
        }
    } catch (...) {
        std::cerr << boost::current_exception_diagnostic_information() << std::endl;
    }
    return 0;
}
