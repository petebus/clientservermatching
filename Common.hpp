#ifndef CLIENSERVERECN_COMMON_HPP
#define CLIENSERVERECN_COMMON_HPP

#include <string>

static short port = 5555;

namespace Requests
{
    static std::string Registration = "Reg";
    static std::string OrderAdd = "Add";
    static std::string OrderRemove = "Rem";
    static std::string Balance = "Bal";
    static std::string OrderList = "Lst";
}

#endif //CLIENSERVERECN_COMMON_HPP
