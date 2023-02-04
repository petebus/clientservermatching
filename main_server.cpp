#include "Server.hpp"

Core& GetCore()
{
    static Core Core;
    return Core;
}

int main()
{
    try
    {
        boost::asio::io_service io_service;
        static Core Core;
        server s(io_service);
        io_service.run();
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}