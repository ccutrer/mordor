// Copyright (c) 2009 - Mozy, Inc.

#include "../predef.h"


#include "../config.h"
#include "../daemon.h"
#include "../http/multipart.h"
#include "../http/server.h"
#include "../iomanager.h"
#include "../main.h"
#include "../socket.h"
#ifdef WINDOWS
#include "../streams/namedpipe.h"
#endif
#include "../streams/socket.h"
#include "../streams/transfer.h"

using namespace Mordor;

void streamConnection(Stream::ptr stream)
{
    try {
        transferStream(stream, stream);
    } catch (UnexpectedEofException &)
    {}
    stream->close();
}

void socketServer(Socket::ptr listen)
{
    listen->listen();

    while (true) {
        Socket::ptr socket = listen->accept();
        Stream::ptr stream(new SocketStream(socket));
        Scheduler::getThis()->schedule(std::bind(&streamConnection, stream));
    }
}

void startSocketServer(IOManager &ioManager)
{
    std::vector<Address::ptr> addresses = Address::lookup("localhost:8000");

    for (std::vector<Address::ptr>::const_iterator it(addresses.begin());
        it != addresses.end();
        ++it) {
        Socket::ptr s = (*it)->createSocket(ioManager, SOCK_STREAM);
        s->bind(*it);
        Scheduler::getThis()->schedule(std::bind(&socketServer, s));
    }

#ifndef WINDOWS
    UnixAddress echoaddress("/tmp/echo");
    Socket::ptr s = echoaddress.createSocket(ioManager, SOCK_STREAM);
    s->bind(echoaddress);
    Scheduler::getThis()->schedule(std::bind(&socketServer, s));
#endif
}

void httpRequest(HTTP::ServerRequest::ptr request)
{
    const std::string &method = request->request().requestLine.method;
    if (method == HTTP::GET || method == HTTP::HEAD || method == HTTP::PUT ||
        method == HTTP::POST) {
        request->response().entity.contentLength = request->request().entity.contentLength;
        request->response().entity.contentType = request->request().entity.contentType;
        request->response().general.transferEncoding = request->request().general.transferEncoding;
        request->response().status.status = HTTP::OK;
        request->response().entity.extension = request->request().entity.extension;
        if (request->hasRequestBody()) {
            if (request->request().requestLine.method != HTTP::HEAD) {
                if (request->request().entity.contentType.type == "multipart") {
                    Multipart::ptr requestMultipart = request->requestMultipart();
                    Multipart::ptr responseMultipart = request->responseMultipart();
                    for (BodyPart::ptr requestPart = requestMultipart->nextPart();
                        requestPart;
                        requestPart = requestMultipart->nextPart()) {
                        BodyPart::ptr responsePart = responseMultipart->nextPart();
                        responsePart->headers() = requestPart->headers();
                        transferStream(requestPart->stream(), responsePart->stream());
                        responsePart->stream()->close();
                    }
                    responseMultipart->finish();
                } else {
                    respondStream(request, request->requestStream());
                    return;
                }
            } else {
                request->finish();
            }
        } else {
            request->response().entity.contentLength = 0;
            request->finish();
        }
    } else {
        respondError(request, HTTP::METHOD_NOT_ALLOWED);
    }
}

void httpServer(Socket::ptr listen)
{
    listen->listen();

    while (true) {
        Socket::ptr socket = listen->accept();
        Stream::ptr stream(new SocketStream(socket));
        HTTP::ServerConnection::ptr conn(new HTTP::ServerConnection(stream, &httpRequest));
        Scheduler::getThis()->schedule(std::bind(&HTTP::ServerConnection::processRequests, conn));
    }
}

void startHttpServer(IOManager &ioManager)
{
    std::vector<Address::ptr> addresses = Address::lookup("localhost:80");

    for (std::vector<Address::ptr>::const_iterator it(addresses.begin());
        it != addresses.end();
        ++it) {
        Socket::ptr s = (*it)->createSocket(ioManager, SOCK_STREAM);
        s->bind(*it);
        Scheduler::getThis()->schedule(std::bind(&httpServer, s));
    }
}

#ifdef WINDOWS
void namedPipeServer(IOManager &ioManager)
{
    while (true) {
        NamedPipeStream::ptr stream(new NamedPipeStream("\\\\.\\pipe\\echo", NamedPipeStream::READWRITE, &ioManager));
        stream->accept();
        Scheduler::getThis()->schedule(std::bind(&streamConnection, stream));
    }
}
#endif

int run(int argc, char *argv[])
{
    try {
        IOManager ioManager;
        startSocketServer(ioManager);
        startHttpServer(ioManager);
#ifdef WINDOWS
        ioManager.schedule(std::bind(&namedPipeServer, std::ref(ioManager)));
        ioManager.schedule(std::bind(&namedPipeServer, std::ref(ioManager)));
#endif
        ioManager.dispatch();
    } catch (...) {
        std::cerr << boost::current_exception_diagnostic_information() << std::endl;
        return 1;
    }
    return 0;
}

MORDOR_MAIN(int argc, char *argv[])
{
    Config::loadFromEnvironment();
    return Daemon::run(argc, argv, &run);
}
