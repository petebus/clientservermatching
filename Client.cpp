#include <iostream>

#include "Client.hpp"
#include "json.hpp"
#include "boost/algorithm/string.hpp"

// Отправка сообщения на сервер по шаблону.
void Client::Send(
    const std::string& aRequestType,
    const std::string& aMessage)
{
    nlohmann::json req;
    req["UserId"] = my_id;
    req["ReqType"] = aRequestType;
    req["Message"] = aMessage;

    std::string request = req.dump();
    boost::asio::write(*s, boost::asio::buffer(request, request.size()));
}

// Возвращает строку с ответом сервера на последний запрос.
std::string Client::ReadMessage()
{
    boost::asio::streambuf b;
    boost::asio::read_until(*s, b, "\0");
    std::istream is(&b);
    std::string line(std::istreambuf_iterator<char>(is), {});
    return line;
}

std::string Client::Authorize(const std::string& Username, const std::string& Password)
{
	Send(Requests::Authorization, Username + " " + Password);
	std::string Result = ReadMessage();

	std::vector<std::string> CommandList;
	boost::split(CommandList, Result, boost::is_any_of(" "));
	if(CommandList.size() == 2 && CommandList[0] == "OK")
	{
		bAuthorized = true;
		my_id = CommandList[1];
		return "Hello!\n";
	}
	return Result;
}

std::string Client::ProcessRegistration(const std::string& InUsername, const std::string& InPassword)
{
    Send(Requests::Registration, InUsername + " " + InPassword);
	std::string Result = ReadMessage();

	std::vector<std::string> CommandList;
	boost::split(CommandList, Result, boost::is_any_of(" "));
	if(CommandList.size() == 2 && CommandList[0] == "OK")
	{
		bAuthorized = true;
		my_id = CommandList[1];
		return "Hello!\n";
	}
	return Result;
}

void Client::Connect()
{
    resolver = new tcp::resolver(io_service);
    query = new tcp::resolver::query(tcp::v4(), "127.0.0.1", std::to_string(port));
    iterator = new tcp::resolver::iterator(resolver->resolve(*query));
    s = make_unique<tcp::socket>(io_service);
    s->connect(**iterator);
}

