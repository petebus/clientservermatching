#define BOOST_TEST_MAIN
#define BOOST_TEST_MODULE Test
#define WIN32_LEAN_AND_MEAN

#include "boost/test/included/unit_test.hpp"
#include "Server.hpp"
#include "Client.hpp"

struct ClientServerFixture
{
	ClientServerFixture()
		: cl(), srv(io_service)
	{
		ServerThread = new std::thread([&]()
		{
			srv.Start();
		});
	}

	~ClientServerFixture()
	{
		srv.Stop();
		ServerThread->join();
	}

	boost::asio::io_service io_service;
	Client cl;
	server srv;
	std::thread* ServerThread;
};
BOOST_FIXTURE_TEST_SUITE(ClientMethods, ClientServerFixture)

BOOST_AUTO_TEST_CASE(ConnectionTest)
{	
	std::jthread ClientThread{[&]()
	{
		cl.Connect();
	}};
	std::this_thread::sleep_for(chrono::milliseconds(100));
	BOOST_CHECK(cl.IsConnected());
	cl.Disconnect();
	std::this_thread::sleep_for(chrono::milliseconds(100));
	BOOST_CHECK(!cl.IsConnected());
}
BOOST_FIXTURE_TEST_CASE(Authorization, ClientServerFixture)
{
	
}
BOOST_FIXTURE_TEST_CASE(AddOrder, ClientServerFixture)
{
	
}

BOOST_AUTO_TEST_SUITE_END()