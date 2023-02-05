#ifndef CLIENSERVERECN_COMMON_HPP
#define CLIENSERVERECN_COMMON_HPP

#include <string>

static short port = 5555;

static std::string connectionString = "host=localhost port=5433 dbname=postgres user=postgres password=11111";

namespace Requests
{
    static std::string Authorization = "Auth";
    static std::string Registration = "Reg";
    static std::string OrderAdd = "Add";
    static std::string OrderRemove = "Rem";
    static std::string Balance = "Bal";
    static std::string OrderList = "Lst";
}

#endif //CLIENSERVERECN_COMMON_HPP
