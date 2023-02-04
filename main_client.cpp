#include <iostream>

#include "Client.hpp"

int main()
{
	Client cl;

	try
	{
		cl.Connect();

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

		cl.ProcessRegistration();
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

				cl.Send(Requests::OrderAdd, command);
				std::cout << cl.ReadMessage() << std::endl;

				break;
			}
			case 2:
			{
				std::cout << "Which order you want to remove?" << std::endl;
				int32_t OrderIdx = -1;
				std::cin >> OrderIdx;
				if (OrderIdx == -1)
				{
					std::cout << "Wrong input" << std::endl;
					break;
				}

				cl.Send(Requests::OrderRemove, std::to_string(OrderIdx));
				std::cout << cl.ReadMessage() << std::endl;

				break;
			}
			case 3:
			{
				cl.Send(Requests::Balance, "");
				std::cout << cl.ReadMessage() << std::endl;
				break;
			}
			case 4:
			{
				cl.Send(Requests::OrderList, "");
				std::cout << cl.ReadMessage() << std::endl;
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