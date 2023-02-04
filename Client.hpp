#pragma once
#include <boost/asio.hpp>

#include "Common.hpp"

using boost::asio::ip::tcp;
using namespace std;

class Client {
    unique_ptr<tcp::socket> s;
    std::string my_id;

    boost::asio::io_service io_service;
    tcp::resolver* resolver;
    tcp::resolver::query* query;
    tcp::resolver::iterator* iterator;

public:
    Client() : s(nullptr), my_id(""), resolver(nullptr), query(nullptr), iterator(nullptr) {
    };
    virtual ~Client() {};

    void Send(const std::string& aRequestType, const std::string& aMessage);
    string ReadMessage();
    void ProcessRegistration();
    void Connect();
};