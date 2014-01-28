#ifndef __MORDOR_MULTIPART_H__
#define __MORDOR_MULTIPART_H__
// Copyright (c) 2009 - Mozy, Inc.

#include <functional>
#include <memory>
#include <string>

#include <boost/noncopyable.hpp>

#include "http.h"

namespace Mordor {

class BodyPart;
class Stream;

struct MissingMultipartBoundaryException : virtual HTTP::Exception, virtual StreamException
{};
struct InvalidMultipartBoundaryException : virtual HTTP::Exception
{};

class Multipart : public std::enable_shared_from_this<Multipart>, boost::noncopyable
{
    friend class BodyPart;
public:
    typedef std::shared_ptr<Multipart> ptr;

    static std::string randomBoundary();

    Multipart(std::shared_ptr<Stream> stream, std::string boundary);

    std::shared_ptr<BodyPart> nextPart();
    void finish();

    std::function<void ()> multipartFinished;

private:
    void partDone();

private:
    std::shared_ptr<Stream> m_stream;
    std::string m_boundary;
    std::shared_ptr<BodyPart> m_currentPart;
    bool m_finished;
};

class BodyPart
{
    friend class Multipart;
public:
    typedef std::shared_ptr<BodyPart> ptr;

private:
    BodyPart(Multipart::ptr multipart);

public:
    HTTP::EntityHeaders &headers();
    std::shared_ptr<Stream> stream();
    Multipart::ptr multipart();

private:
    HTTP::EntityHeaders m_headers;
    Multipart::ptr m_multipart;
    std::shared_ptr<Stream> m_stream;
    Multipart::ptr m_childMultipart;
};

}

#endif
