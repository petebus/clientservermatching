#define BOOST_TEST_MAIN
#define BOOST_TEST_MODULE Test
#define WIN32_LEAN_AND_MEAN

#include "boost/test/included/unit_test.hpp"
#include "Server.hpp"
#include "Client.hpp"

BOOST_AUTO_TEST_CASE(TestFuncTest)
{
	
}


BOOST_AUTO_TEST_SUITE(testSuiteClient)
struct Fixture
{
	Fixture()
		//: client()
	{
	}

	~Fixture()
	{

	}
};

BOOST_FIXTURE_TEST_CASE(testClient, Fixture)
{
	BOOST_CHECK(true);
}

BOOST_AUTO_TEST_SUITE_END()