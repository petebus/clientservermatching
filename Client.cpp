#include <iostream>

#include "Client.hpp"
#include "json.hpp"

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

// "Создаём" пользователя, получаем его ID.
void Client::ProcessRegistration()
{
    std::string name;
    std::cout << "Hello! Enter your name: ";
    std::cin >> name;

    // Для регистрации Id не нужен, заполним его нулём
    Send(Requests::Registration, name);
    my_id = ReadMessage();
}

void Client::Connect()
{
    resolver = new tcp::resolver(io_service);
    query = new tcp::resolver::query(tcp::v4(), "127.0.0.1", std::to_string(port));
    iterator = new tcp::resolver::iterator(resolver->resolve(*query));
    s = make_unique<tcp::socket>(io_service);
    s->connect(**iterator);
}

