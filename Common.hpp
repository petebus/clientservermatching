#pragma once
#include <string>
static short port = 5555;
static std::string connectionString = "host=localhost port=5433 dbname=postgres user=postgres password=11111";
static uint16_t ConnectionTimeoutDelay = 5000;

namespace Requests
{
    static std::string Authorization = "Auth";
    static std::string Registration = "Reg";
    static std::string AuthCheck = "IsAuth";
    static std::string Ping = "Ping";
    static std::string AddOrder = "Add";
    static std::string RemoveOrder = "Rem";
    static std::string Balance = "Bal";
    static std::string OrderList = "Lst";
}
