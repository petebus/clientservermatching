#pragma once
#include <boost/asio.hpp>

#include "Common.hpp"

using boost::asio::ip::tcp;
using namespace std;

class Client {
    unique_ptr<tcp::socket> s;

    boost::asio::io_service io_service;
    tcp::resolver* resolver;
    tcp::resolver::query* query;
    tcp::resolver::iterator* iterator;

public:
    Client() : s(nullptr), resolver(nullptr), query(nullptr), iterator(nullptr)  {}
    virtual ~Client() {}
	void Connect();
	
	/*Login methods*/
	string Authorize(const std::string& Username, const std::string& Password);
    string Register(const std::string& InUsername, const std::string& InPassword);
	bool IsAuthorized();
	const atomic_bool& IsConnected() const { return bConnected; }
	
private:
	void Send(const std::string& aRequestType, const std::string& aMessage);
	string ReadMessage();

	/*Ping*/
	boost::asio::deadline_timer* PingTimer = nullptr;
	void SendPing();

	atomic_bool bConnected = false;
};