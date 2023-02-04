#include <iostream>
#include <boost/asio.hpp>
#include "Common.hpp"
#include "json.hpp"

using boost::asio::ip::tcp;

// Отправка сообщения на сервер по шаблону.
void SendMessage(
    tcp::socket& aSocket,
    const std::string& aId,
    const std::string& aRequestType,
    const std::string& aMessage)
{
    nlohmann::json req;
    req["UserId"] = aId;
    req["ReqType"] = aRequestType;
    req["Message"] = aMessage;

    std::string request = req.dump();
    boost::asio::write(aSocket, boost::asio::buffer(request, request.size()));
}

// Возвращает строку с ответом сервера на последний запрос.
std::string ReadMessage(tcp::socket& aSocket)
{
    boost::asio::streambuf b;
    boost::asio::read_until(aSocket, b, "\0");
    std::istream is(&b);
    std::string line(std::istreambuf_iterator<char>(is), {});
    return line;
}

// "Создаём" пользователя, получаем его ID.
std::string ProcessRegistration(tcp::socket& aSocket)
{
    std::string name;
    std::cout << "Hello! Enter your name: ";
    std::cin >> name;

    // Для регистрации Id не нужен, заполним его нулём
    SendMessage(aSocket, "0", Requests::Registration, name);
    return ReadMessage(aSocket);
}

int main()
{
	try
	{
		boost::asio::io_service io_service;

		tcp::resolver resolver(io_service);
		tcp::resolver::query query(tcp::v4(), "127.0.0.1", std::to_string(port));
		tcp::resolver::iterator iterator = resolver.resolve(query);

		tcp::socket s(io_service);
		s.connect(*iterator);

		/*SendMessage(s, "0", Requests::Registration, "1");
		std::string FirstClient = ReadMessage(s);
		SendMessage(s, "0", Requests::Registration, "2");
		std::string SecondClient = ReadMessage(s);
		SendMessage(s, "0", Requests::Registration, "3");
		std::string ThirdClient = ReadMessage(s);

		SendMessage(s, FirstClient, Requests::OrderAdd, "BUY 10 62");
		ReadMessage(s);
		SendMessage(s, SecondClient, Requests::OrderAdd, "BUY 20 63");
		ReadMessage(s);
		SendMessage(s, ThirdClient, Requests::OrderAdd, "SELL 50 61");
		ReadMessage(s);

		SendMessage(s, FirstClient, Requests::OrderList, "");
		std::cout << ReadMessage(s) << std::endl;
		SendMessage(s, SecondClient, Requests::OrderList, "");
		std::cout << ReadMessage(s) << std::endl;
		SendMessage(s, ThirdClient, Requests::OrderList, "");
		std::cout << ReadMessage(s) << std::endl;

		SendMessage(s, FirstClient, Requests::Balance, "");
		std::cout << ReadMessage(s) << std::endl;
		SendMessage(s, SecondClient, Requests::Balance, "");
		std::cout << ReadMessage(s) << std::endl;
		SendMessage(s, ThirdClient, Requests::Balance, "");
		std::cout << ReadMessage(s) << std::endl;
		return 0;*/
		
		std::string my_id = ProcessRegistration(s);
		while (true)
		{
			// Тут реализовано "бесконечное" меню.
			std::cout << "Menu:\n"
						 "1) Add order\n"
						 "2) Remove order\n"
						 "3) Balance\n"
						 "4) Order List\n"
						 "5) Exit\n"
						 << std::endl;

			short menu_option_num;
			std::cin >> menu_option_num;
			switch (menu_option_num)
			{
				case 1:
				{
					/*Add order logic*/
					std::cout << "Write order data in format: BUY/SELL USD_VAL RUB_VAL " << std::endl;
					std::string command;

					std::cin.ignore();
					std::getline(std::cin, command);
					
					SendMessage(s, my_id, Requests::OrderAdd, command);
					std::cout << ReadMessage(s) << std::endl;

					break;
				}
				case 2:
				{
					std::cout << "Which order you want to remove?" << std::endl;
					int32_t OrderIdx = -1;
					std::cin >> OrderIdx;
					if(OrderIdx == -1)
					{
						std::cout << "Wrong input" << std::endl;
						break;
					}
					
					SendMessage(s, my_id, Requests::OrderRemove, std::to_string(OrderIdx));
					std::cout << ReadMessage(s) << std::endl;
					
					break;
				}
				case 3:
				{
					SendMessage(s, my_id, Requests::Balance, "");
					std::cout << ReadMessage(s) << std::endl;
					break;
				}
				case 4:
				{
					SendMessage(s, my_id, Requests::OrderList, "");
					std::cout << ReadMessage(s) << std::endl;
					break;
				}
				case 5:
				{
					exit(0);
					break;
				}
				default:
				{
					std::cout << "Unknown menu option\n" << std::endl;
					std::cin.clear();
					std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
				}
			}
		}
	}
	catch (std::exception& e)
	{
		std::cerr << "Exception: " << e.what() << "\n";
	}

	return 0;
}