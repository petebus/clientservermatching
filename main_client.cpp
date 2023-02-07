#include <iostream>
#include "Client.hpp"
#include <boost/algorithm/string.hpp>

#include "Common.hpp"

int main()
{
	Client cl;

	try
	{
		cl.Connect();
		while(!cl.IsConnected())
		{
			std::this_thread::sleep_for(chrono::milliseconds(100));
		}
		
		/*Authorization menu*/
		while(!cl.IsAuthorized())
		{
			std::cout << "Hello:\n"
				"1) Authorize\n"
				"2) Register\n"
				<< std::endl;
			short menu_option_num;
			std::cin >> menu_option_num;
			switch (menu_option_num)
			{
				case 1:
				{
					std::cout << "Login with format: USERNAME PASSWORD" << std::endl;
					std::string command;
					std::cin.ignore();
					std::getline(std::cin, command);

					std::vector<std::string> CommandList;
					boost::split(CommandList, command, boost::is_any_of(" "));

					if (CommandList.size() != 2)
					{
						std::cout << "Error: mismatching template" << endl;
						break;
					}
					std::string Result = cl.Authorize(CommandList[0], CommandList[1]);
					if(!cl.IsAuthorized())
					{
						std::cout << Result << endl;
					}
					break;
				}
				case 2:
				{
					std::cout << "Register with format: USERNAME PASSWORD" << std::endl;
					std::string command;
					std::cin.ignore();
					std::getline(std::cin, command);

					std::vector<std::string> CommandList;
					boost::split(CommandList, command, boost::is_any_of(" "));

					if (CommandList.size() != 2)
					{
						std::cout << "Error: mismatching template" << endl;
						break;
					}
					
					std::string Result = cl.Register(CommandList[0], CommandList[1]);
					if(!cl.IsAuthorized())
					{
						std::cout << Result << endl;
					}
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
		
		/*Main navigation*/
		while (cl.IsAuthorized())
		{
			std::cout << "Menu:\n"
				"1) Add order\n"
				"2) Remove order\n"
				"3) Balance\n"
				"4) Order List\n"
				"5) Show quotes\n"
				"6) Exit\n"
				<< std::endl;

			short menu_option_num;
			std::cin >> menu_option_num;
			switch (menu_option_num)
			{
				case 1:
				{
					std::cout << "Write order data in format: BUY/SELL USD_VAL RUB_VAL " << std::endl;
					std::string command;

					std::cin.ignore();
					std::getline(std::cin, command);

					std::cout << cl.AddOrder(command) << std::endl;

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
					std::cout << cl.RemoveOrder(std::to_string(OrderIdx)) << std::endl;
					break;
				}
				case 3:
				{
					std::cout << cl.GetBalance() << std::endl;
					break;
				}
				case 4:
				{
					std::cout << cl.GetOrderList() << std::endl;
					break;
				}
				case 5:
				{
					std::cout << cl.GetQuotes() << std::endl;
					break;
				}
				case 6:
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
