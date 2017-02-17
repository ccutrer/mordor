// Copyright (c) 2009 - Mozy, Inc.

#include "../predef.h"

#include <iostream>

#include "../config.h"
#include "../iomanager.h"
#include "../socket.h"
#include "../statistics.h"

using namespace Mordor;

ConfigVar<size_t>::ptr g_perCount = Config::lookup<size_t>("dumpfrequency", 10u, "How often should statistics be dumped (packets)");

int main(int argc, char **argv)
{
    Config::loadFromEnvironment();
    if (argc != 2) {
        std::cerr << "Usage: <address>" << std::endl;
        return 1;
    }
    try {
        IOManager ioManager;
        AverageMinMaxStatistic<size_t> &stats = Statistics::registerStatistic("broadcasts",
            AverageMinMaxStatistic<size_t>("bytes", "packets"));

        std::vector<Address::ptr> addresses = Address::lookup(argv[1]);
        Socket::ptr sock = addresses[0]->createSocket(ioManager, SOCK_DGRAM);
        sock->bind(addresses[0]);
        char buf[65536];
        IPv4Address addr;
        while (true) {
            size_t read = sock->receiveFrom(buf, 65536, addr);
            stats.update(read);
            if (stats.count.count % g_perCount->val() == 0)
                Statistics::dump(std::cout);
        }
    } catch (...) {
        std::cerr << boost::current_exception_diagnostic_information() << std::endl;
        return 2;
    }
    return 0;
}
