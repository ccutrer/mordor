#ifndef __MORDOR_HTTP_OAUTH2_H__
#define __MORDOR_HTTP_OAUTH2_H__
// Copyright (c) 2011 - Mozy, Inc.

#include "broker.h"

namespace Mordor {
namespace HTTP {
namespace OAuth2 {

    void authorize(Request &nextRequest,
        const std::string &token);

    class RequestBroker : public RequestBrokerFilter
    {
    public:
        RequestBroker(HTTP::RequestBroker::ptr parent,
            std::function<bool (const URI &,
            std::shared_ptr<ClientRequest> /* priorRequest = ClientRequest::ptr() */,
            std::string & /* token */,
            size_t /* attempts */)> getCredentialsDg)
            : RequestBrokerFilter(parent),
            m_getCredentialsDg(getCredentialsDg)
        {}

        std::shared_ptr<ClientRequest> request(Request &requestHeaders,
            bool forceNewConnection = false,
            std::function<void (std::shared_ptr<ClientRequest>)> bodyDg = NULL);

    private:
        std::function<bool (const URI &, std::shared_ptr<ClientRequest>, std::string &,
            size_t)> m_getCredentialsDg;
    };

}}}

#endif // __MORDOR_HTTP_OAUTH2_H__

