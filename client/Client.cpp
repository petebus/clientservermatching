#include "Client.hpp"

#include <iostream>
#include <boost/bind/bind.hpp>

#include "Common.hpp"
#include "json.hpp"
#include "boost/algorithm/string.hpp"

void Client::Send(const std::string& aRequestType, const std::string& aMessage)
{
    nlohmann::json req;
    req["ReqType"] = aRequestType;
    req["Message"] = aMessage;

    std::string request = req.dump();
    boost::asio::write(*s, boost::asio::buffer(request, request.size()));
}
std::string Client::ReadMessage()
{
    boost::asio::streambuf b;
    boost::asio::read_until(*s, b, "\0");
    std::istream is(&b);
    std::string line(std::istreambuf_iterator<char>(is), {});
    return line;
}

void Client::SendPing()
{
	const std::lock_guard LockGuard(Lock);
	Send(Requests::Ping, "");
	std::string Result = ReadMessage();
	if(Result == "OK")
	{
		delete PingTimer;
		PingTimer = new boost::asio::deadline_timer(io_service, boost::posix_time::seconds(1));
		PingTimer->async_wait(boost::bind(&Client::SendPing, this));
	}
	else
	{
		std::cout << "Connection is failed" << std::endl;
	}
}
std::string Client::Authorize(const std::string& Username, const std::string& Password)
{
	const std::lock_guard LockGuard(Lock);
	Send(Requests::Authorization, Username + " " + Password);
	std::string Result = ReadMessage();
	if(Result == "OK")
	{
		return "Hello!\n";
	}
	return Result;
}
std::string Client::Register(const std::string& InUsername, const std::string& InPassword)
{
	const std::lock_guard LockGuard(Lock);
    Send(Requests::Registration, InUsername + " " + InPassword);
	std::string Result = ReadMessage();
	if(Result == "OK")
	{
		return "Hello!\n";
	}
	return Result;
}
bool Client::IsAuthorized()
{
	const std::lock_guard LockGuard(Lock);
	Send(Requests::AuthCheck, "");
	std::string Result = ReadMessage();
	return Result == "YES";
}

string Client::AddOrder(const std::string &OrderData)
{
	const std::lock_guard LockGuard(Lock);
	Send(Requests::AddOrder, OrderData);
	std::string Result = ReadMessage();
	return Result;
}

string Client::RemoveOrder(const std::string &OrderIdx)
{
	const std::lock_guard LockGuard(Lock);
	Send(Requests::RemoveOrder, OrderIdx);
	std::string Result = ReadMessage();
	return Result;
}

string Client::GetOrderList()
{
	const std::lock_guard LockGuard(Lock);
	Send(Requests::OrderList, "");
	std::string Result = ReadMessage();
	return Result;
}

string Client::GetBalance()
{
	const std::lock_guard LockGuard(Lock);
	Send(Requests::Balance, "");
	std::string Result = ReadMessage();
	return Result;
}

void Client::Connect()
{
	const std::lock_guard LockGuard(Lock);

    resolver = new tcp::resolver(io_service);
    query = new tcp::resolver::query(tcp::v4(), "127.0.0.1", std::to_string(port));
    iterator = new tcp::resolver::iterator(resolver->resolve(*query));
    s = make_unique<tcp::socket>(io_service);
    s->connect(**iterator);

	/*Start ping requests*/
	PingTimer = new boost::asio::deadline_timer(io_service, boost::posix_time::seconds(1));
	PingTimer->async_wait(boost::bind(&Client::SendPing, this));

	bConnected = true;
	io_service.run();
}

void Client::Disconnect()
{
	bConnected = false;
	io_service.stop();
	s->close();
	s.reset();
	delete iterator;
	delete resolver;
	delete query;
	delete PingTimer;
}

