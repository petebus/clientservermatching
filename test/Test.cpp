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
	cl.Connect();
	std::this_thread::sleep_for(chrono::milliseconds(100));
	BOOST_CHECK(cl.IsConnected());
	cl.Disconnect();
	std::this_thread::sleep_for(chrono::milliseconds(100));
	BOOST_CHECK(!cl.IsConnected());
}

BOOST_FIXTURE_TEST_CASE(AuthorizationOrRegister, ClientServerFixture)
{
	cl.Connect();
	std::this_thread::sleep_for(chrono::milliseconds(100));
	BOOST_CHECK(cl.IsConnected());
	cl.Authorize("1", "1");
	if (!cl.IsAuthorized()) cl.Register("1", "1");
	BOOST_CHECK(cl.IsAuthorized());
	cl.Disconnect();
}

BOOST_FIXTURE_TEST_CASE(AddOrder, ClientServerFixture)
{
	cl.Connect();
	std::this_thread::sleep_for(chrono::milliseconds(100));
	BOOST_CHECK(cl.IsConnected());
	cl.Authorize("1", "1");
	if (!cl.IsAuthorized()) cl.Register("1", "1");
	BOOST_CHECK(cl.IsAuthorized());
	cl.AddOrder("BUY 100 76");
	cl.Disconnect();
}

BOOST_FIXTURE_TEST_CASE(RemoveOrder, ClientServerFixture)
{
	cl.Connect();
	std::this_thread::sleep_for(chrono::milliseconds(100));
	BOOST_CHECK(cl.IsConnected());
	cl.Authorize("1", "1");
	if (!cl.IsAuthorized()) cl.Register("1", "1");
	BOOST_CHECK(cl.IsAuthorized());
	cl.RemoveOrder("1");
	cl.Disconnect();
}

BOOST_FIXTURE_TEST_CASE(TestScenario, ClientServerFixture)
{
	cl.Connect();
	std::this_thread::sleep_for(chrono::milliseconds(100));
	BOOST_CHECK(cl.IsConnected());
	cl.Authorize("1", "1");
	if(!cl.IsAuthorized()) cl.Register("1", "1");
	BOOST_CHECK(cl.IsAuthorized());
	cl.AddOrder("BUY 10 62");
	cl.Disconnect();

	cl.Connect();
	std::this_thread::sleep_for(chrono::milliseconds(100));
	BOOST_CHECK(cl.IsConnected());
	cl.Authorize("2", "2");
	if(!cl.IsAuthorized()) cl.Register("2", "2");
	BOOST_CHECK(cl.IsAuthorized());
	cl.AddOrder("BUY 20 63");
	cl.Disconnect();

	cl.Connect();
	std::this_thread::sleep_for(chrono::milliseconds(100));
	BOOST_CHECK(cl.IsConnected());
	cl.Authorize("3", "3");
	if(!cl.IsAuthorized()) cl.Register("3", "3");
	BOOST_CHECK(cl.IsAuthorized());
	cl.AddOrder("SELL 50 63");
	cl.GetQuotes();
	cl.Disconnect();
}

BOOST_FIXTURE_TEST_CASE(TestSpread, ClientServerFixture)
{
	cl.Connect();
	std::this_thread::sleep_for(chrono::milliseconds(100));
	BOOST_CHECK(cl.IsConnected());
	cl.Authorize("1", "1");
	if (!cl.IsAuthorized()) cl.Register("1", "1");
	BOOST_CHECK(cl.IsAuthorized());
	cl.AddOrder("BUY 10 62");
	cl.Disconnect();

	cl.Connect();
	std::this_thread::sleep_for(chrono::milliseconds(100));
	BOOST_CHECK(cl.IsConnected());
	cl.Authorize("2", "2");
	if (!cl.IsAuthorized()) cl.Register("2", "2");
	BOOST_CHECK(cl.IsAuthorized());
	cl.AddOrder("BUY 20 70");
	cl.Disconnect();
}

BOOST_AUTO_TEST_SUITE_END()