#ifndef CLIENSERVERECN_COMMON_HPP
#define CLIENSERVERECN_COMMON_HPP

#include <string>

static short port = 5555;

namespace Requests
{
    static std::string Registration = "Reg";
    static std::string Hello = "Hel";
    static std::string OrderAdd = "AddOrder";
    static std::string Balance = "Bal";
    static std::string OrderList = "List";
}

#endif //CLIENSERVERECN_COMMON_HPP
