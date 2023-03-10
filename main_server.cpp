#include <iostream>
#include "Server.hpp"

int main()
{
    try
    {
        boost::asio::io_service io_service;
        server s(io_service);
    	s.Start();
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}