#include "Server.hpp"
#include <iostream>
#include <boost/asio.hpp>
#include <pqxx/connection.hxx>
#include <pqxx/transaction.hxx>
#include <boost/bind/bind.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include "Common.hpp"

static const auto SpreadValue = 6;

void ClientSession::Start()
{
    Socket.async_read_some(boost::asio::buffer(DataBuffer, MaxBufferLength),
        boost::bind(&ClientSession::handle_read, this,
            boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred));
	
	PingTimeoutHandle = new boost::asio::deadline_timer(io_service, boost::posix_time::milliseconds(ConnectionTimeoutDelay));
	PingTimeoutHandle->async_wait(boost::bind(&ClientSession::handle_connection_timeout, this, boost::asio::placeholders::error));
}

#define MAKE_SIMPLE_CALLBACK_REQUEST(Server, ExecuteName, Message) \
{ \
	(Server)->Request(this, Message, \
		boost::bind(&server::Execute_##ExecuteName, Server, this, boost::placeholders::_2), \
		boost::bind(&ClientSession::Callback, this, boost::placeholders::_1));\
}\

#define MAKE_LABMDA_CALLBACK_REQUEST(Server, ExecuteName, Message, Lambda) \
{ \
	(Server)->Request(this, Message, \
		boost::bind(&server::Execute_##ExecuteName, Server, this, boost::placeholders::_2), \
		boost::bind(Lambda, boost::placeholders::_1));\
}\

void ClientSession::handle_read(const boost::system::error_code& error,
    size_t bytes_transferred)
{
    if (!error)
    {
    	std::lock_guard LockGuard(Lock);

	    /*Parse json*/
    	DataBuffer[bytes_transferred] = '\0';
    	
    	auto j = nlohmann::json::parse(DataBuffer);
    	auto reqType = j["ReqType"];
    	
    	std::string reply_body = "Error! Unknown request type";
    	if(reqType == Requests::Ping)
    	{
    		PingTimeoutHandle->cancel();
    		delete PingTimeoutHandle;
    		PingTimeoutHandle = new boost::asio::deadline_timer(io_service, boost::posix_time::milliseconds(ConnectionTimeoutDelay));
    		PingTimeoutHandle->async_wait(boost::bind(&ClientSession::handle_connection_timeout, this, boost::asio::placeholders::error));

    		/*Reset timer*/
    		std::cout << "Connection is OK" << std::endl;
    		reply_body = "OK";
    	}
    	else if(reqType == Requests::AuthCheck)
    	{
    		reply_body = bAuthorized ? "YES" : "NO";
    	}
    	else if (reqType == Requests::Registration)
    	{
    		CallbackSignature Lambda = [&](std::string Result)
    		{
    			std::vector<std::string> CommandList;
    			boost::split(CommandList, Result, boost::is_any_of(" "));
    			if(CommandList.size() == 2 && CommandList[0] == "OK")
    			{
    				bAuthorized = true;
    				UserID = std::atoi(CommandList[1].c_str());

    				std::string res = "OK";
    				Callback(res);
    			}
    			else
    			{
    				Callback(Result);
    			}
    		};
    		MAKE_LABMDA_CALLBACK_REQUEST(OuterServer, Registration, j["Message"], Lambda)
    		return;
    	}
    	else if(reqType == Requests::Authorization)
    	{
    		CallbackSignature Lambda = [&](std::string Result)
    		{
    			std::vector<std::string> CommandList;
    			boost::split(CommandList, Result, boost::is_any_of(" "));
    			if(CommandList.size() == 2 && CommandList[0] == "OK")
    			{
    				bAuthorized = true;
    				UserID = std::atoi(CommandList[1].c_str());
    				std::string res = "OK";
    				Callback(res);
    			}
    			else
    			{
    				Callback(Result);
    			}
    		};
    		MAKE_LABMDA_CALLBACK_REQUEST(OuterServer, Authorization, j["Message"], Lambda);
    		return;
    	}
    	else if (reqType == Requests::RemoveOrder)
    	{
    		MAKE_SIMPLE_CALLBACK_REQUEST(OuterServer, RemoveOrder, j["Message"]);
			return;
    	}
    	else if (reqType == Requests::AddOrder)
    	{
    		MAKE_SIMPLE_CALLBACK_REQUEST(OuterServer, AddOrder, j["Message"]);
			return;
    	}
    	else if (reqType == Requests::Balance)
    	{
    		MAKE_SIMPLE_CALLBACK_REQUEST(OuterServer, Balance, j["Message"]);
    		return;
    	}
    	else if (reqType == Requests::OrderList)
    	{
    		MAKE_SIMPLE_CALLBACK_REQUEST(OuterServer, OrderList, j["Message"]);
    		return;
    	}
		else if (reqType == Requests::Quotes)
		{
			MAKE_SIMPLE_CALLBACK_REQUEST(OuterServer, Quotes, j["Message"]);
			return;
		}
    	Callback(reply_body);
    }
    else
    {
        delete this;
    }
}
void ClientSession::Callback(std::string Result)
{
	Output = Result;
	boost::asio::async_write(Socket,
				boost::asio::buffer(Output, Output.size()),
				boost::bind(&ClientSession::handle_write, this,
					boost::asio::placeholders::error));
}
void ClientSession::handle_write(const boost::system::error_code& error)
{
    if (!error)
    {
        Socket.async_read_some(boost::asio::buffer(DataBuffer, MaxBufferLength),
            boost::bind(&ClientSession::handle_read, this,
                boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred));
    }
    else
    {
        delete this;
    }
}
void ClientSession::handle_connection_timeout(const boost::system::error_code& error)
{
	//const std::lock_guard LockGuard(Lock);
	if(error.failed()) return;
	std::cout << "Connection is lost" << std::endl;
}

server::server(boost::asio::io_service& io_service)
    : io_service(io_service),
    acceptor_(io_service, tcp::endpoint(tcp::v4(), port))
{
	ConnectionObject = std::make_shared<pqxx::connection>(connectionString.c_str());
}

void server::Start()
{
	std::cout << "Server started! Listen " << port << " port" << std::endl;
	ClientSession* new_session = new ClientSession(this, io_service);
	acceptor_.async_accept(new_session->GetSocket(),
		boost::bind(&server::handle_accept, this, new_session,
			boost::asio::placeholders::error));

	io_service.run();
}

void server::Stop()
{
	io_service.stop();
	acceptor_.close();
}

void server::handle_accept(ClientSession* new_session, const boost::system::error_code& error)
{
    if (!error)
    {
        new_session->Start();

        /*do ping every N seconds*/
        new_session = new ClientSession(this, io_service);
        acceptor_.async_accept(new_session->GetSocket(),
            boost::bind(&server::handle_accept, this, new_session,
                boost::asio::placeholders::error));
    }
    else
    {
        delete new_session;
    }
}

void server::handle_end_connection(ClientSession *InSession)
{
}

void server::Request(ClientSession* InSession, std::string Msg, ExecuteSignature InExecuteMethod,
	CallbackSignature SessionCallback)
{
	//const std::lock_guard LockGuard(RequestLock);
	
	RequestQueue.push(QueuedRequest(InSession, InExecuteMethod, SessionCallback));
	if(RequestQueue.front().Session)
	{
		RequestQueue.front().ExecuteMethod(InSession, Msg);
	}
	RequestQueue.pop();
}

void server::Execute_AddOrder(ClientSession* InSession, std::string Msg)
{
	std::vector<std::string> CommandList;
	boost::split(CommandList, Msg, boost::is_any_of(" "));

	std::string reply_body = "Unknown error";
	if (CommandList.size() < 3 || CommandList[0] != "BUY" && CommandList[0] != "SELL")
	{
		reply_body = "Error: mismatching template";
	}
	else
	{
		const int64_t Value = std::atoi(CommandList[1].c_str());
		const int64_t Price = std::atoi(CommandList[2].c_str());
		
		/*add order to order table*/
		auto r = SQL_Request(true, "INSERT INTO orders (type, value, price, user_id) VALUES ('%s', %i, %i, %i)",
			CommandList[0].c_str(), Value, Price, InSession->GetUserID());
		r = SQL_Request(false, "SELECT MAX(order_id) FROM orders WHERE user_id = %d", InSession->GetUserID());
		
		/*add order to user list*/
		SQL_Request(true, "UPDATE users SET active_orders = ARRAY[%d] || active_orders WHERE id = %d",std::atoi( r[0][0].c_str()), InSession->GetUserID());
		
		reply_body = "Order successfully added";
		
		Handle_OrderAdded(InSession);
	}
	RequestQueue.front().SessionCallback(reply_body);
}
void server::Execute_RemoveOrder(ClientSession* InSession, std::string Msg)
{
	std::string reply_body = "Error: row not found \n";

	const int64_t OrderIdx = std::atoi(Msg.c_str());
	auto r = SQL_Request(true, "UPDATE orders SET status = 'canceled' WHERE user_id = %d AND order_id = %d", InSession->GetUserID(), OrderIdx);
	if(r.affected_rows() > 0)
	{
		SQL_Request(true, "UPDATE users SET active_orders = ARRAY_REMOVE(active_orders, %d) WHERE id = %d", OrderIdx, InSession->GetUserID());
		reply_body = "Order was removed";
	}
	RequestQueue.front().SessionCallback(reply_body);
}

void server::Execute_Registration(ClientSession* InSession, std::string Msg)
{
	std::string reply_body = "Unknown error \n";

	/*Parse commands */
	std::vector<std::string> CommandList;
	boost::split(CommandList, Msg, boost::is_any_of(" "));
        	
	/*if username is empty - allow to create*/
	pqxx::result r = SQL_Request(false, "SELECT id FROM users WHERE username = '%s'", CommandList[0].c_str());
	if(r.affected_rows() == 0)
	{
		r = SQL_Request(false, "SELECT MAX(id) FROM users");
		SQL_Request(true, "INSERT INTO users (username, password) VALUES ('%s','%s')", CommandList[0].c_str(), CommandList[1].c_str());
		auto ID = SQL_Request(true, "SELECT id FROM users WHERE username = '%s'", CommandList[0].c_str())[0][0].c_str();
		reply_body = "OK " + std::string(ID);
	}
	else
	{
		reply_body = "Error: name is already created";
	}
	RequestQueue.front().SessionCallback(reply_body);
}
void server::Execute_Authorization(ClientSession *InSession, std::string Msg)
{
	std::vector<std::string> CommandList;
	boost::split(CommandList, Msg, boost::is_any_of(" "));
	std::string reply_body = "Unknown error \n";

	/*if username is empty - allow to create*/
	pqxx::result r = SQL_Request(false, "SELECT id FROM users WHERE username = '%s' AND password = '%s'", CommandList[0].c_str(), CommandList[1].c_str());
	if(r.affected_rows() != 0)
	{
		const uint64_t ID = std::atoi(r[0][0].c_str());
		reply_body = "OK " + std::to_string(ID);
	}
	else
	{
		reply_body = "Error: wrong username or pass";
	}
	RequestQueue.front().SessionCallback(reply_body);
}

void server::Execute_Balance(ClientSession* InSession, std::string Msg)
{
	auto User = InSession->GetUserID();
	std::string reply_body = "Unknown error \n";

	pqxx::result r = SQL_Request(false, "SELECT balance_usd, balance_rub FROM users WHERE id = '%d'", User);
	if(r.affected_rows() != 0)
	{
		reply_body = std::format("USD: {} RUB: {}", r[0][0].c_str(), r[0][1].c_str());
	}
	else
	{
		reply_body = "Error: wrong username or pass";
	}
	RequestQueue.front().SessionCallback(reply_body);
}

void server::Execute_OrderList(ClientSession *InSession, std::string Msg)
{
	auto User = InSession->GetUserID();

	pqxx::result r = SQL_Request(false, "SELECT * FROM orders WHERE user_id = '%d' AND status = 'active'", User);
	std::string reply_body = "\n";
	for (uint64_t OrderIt = 0; OrderIt < r.affected_rows(); OrderIt++)
	{
		reply_body += r[OrderIt][6].c_str() + std::string(" ");
		reply_body += r[OrderIt][0].c_str() + std::string(" ");
		reply_body += r[OrderIt][1].c_str() + std::string(" USD ");
		reply_body += r[OrderIt][2].c_str() + std::string(" RUB ");
		reply_body += "\n";
	}
	RequestQueue.front().SessionCallback(reply_body);
}

void server::Execute_Quotes(ClientSession* InSession, std::string Msg)
{
	std::string reply_body = "Unknown error \n";
	if (Bid > 0 && Offer > 0)
	{
		reply_body = std::format("Bid: {} Offer: {}", Bid, Offer);
	}
	else
	{
		reply_body = "Error: we have no market";
	}
	RequestQueue.front().SessionCallback(reply_body);
}

void server::Handle_OrderAdded(ClientSession *InSession)
{
	auto r = SQL_Request(false, "SELECT MAX(order_id) FROM orders WHERE user_id = %d AND status = 'active'", InSession->GetUserID());

	/*Get all orders, which is not of current user, sorted by lowest (for buyer) or highest(for salers), but not more/less*/
	uint32_t OrderID = r[0][0].as<uint32_t>();
	auto Order = SQL_Request(false, "SELECT * FROM orders WHERE order_id = %d", OrderID)[0];

	std::string Type = Order[0].as<std::string>();
	const bool bIsBuyer = Type == "BUY";
	if(bIsBuyer)
	{
		r = SQL_Request(false, "SELECT * FROM orders WHERE type = 'SELL' AND user_id != %d AND status = 'active' ORDER BY price ASC, order_id DESC", InSession->GetUserID());
	}
	else
	{
		r = SQL_Request(false, "SELECT * FROM orders WHERE type = 'BUY' AND user_id != %d AND status = 'active' ORDER BY price DESC, order_id ASC", InSession->GetUserID());
	}

	/*while its still enough money - make a deals*/
	for(uint64_t It = 0; It < r.affected_rows(); It++)
	{
		if(bIsBuyer)
		{
			Handle_MakeMatch(Order, r[It]);
		}
		else
		{
			Handle_MakeMatch(r[It], Order);
		}
		auto Status = SQL_Request(false, "SELECT status FROM orders WHERE order_id = %d", OrderID)[0][0].as<std::string>();
		if(Status != "active") break;

		/*update order data*/
		Order = SQL_Request(false, "SELECT * FROM orders WHERE order_id = %d", OrderID)[0];
	}
}

void server::Handle_MakeMatch(const pqxx::row& BuyOrder, const pqxx::row& SellOrder)
{
	/*Buyer*/
	const auto BuyerValue = std::atoi(BuyOrder[1].c_str());
	const auto BuyerPrice = std::atoi(BuyOrder[2].c_str());

	/*Seller*/
	const auto SellerValue = std::atoi(SellOrder[1].c_str());
	const auto SellerPrice = std::atoi(SellOrder[2].c_str());
	
	const auto DeltaValue = std::min(BuyerValue, SellerValue);

	const auto BuyerID = BuyOrder[3].as<int64_t>();
	const auto BuyOrderID = BuyOrder[6].as<int64_t>();
	const auto SellerID = SellOrder[3].as<int64_t>();
	const auto SellOrderID = SellOrder[6].as<int64_t>();

	const auto Spread = abs(BuyerPrice - SellerPrice);

	/*We will don't match orders if spread above SpreadValue*/
	if (Spread > SpreadValue)
		return;

	/*Update order value*/
	SQL_Request(true, "UPDATE orders SET value = %d WHERE order_id = %d", BuyerValue - DeltaValue, BuyOrderID);
	SQL_Request(true, "UPDATE orders SET value = %d WHERE order_id = %d", SellerValue - DeltaValue, SellOrderID);

	/*If order is reached 0 - close it*/
	auto BuyOrderUpdated = SQL_Request(false, "SELECT value FROM orders WHERE order_id = '%d'", BuyOrderID);
	if(BuyOrderUpdated[0][0].as<int64_t>() == 0)
	{
		/*CloseOrder*/
		auto r = SQL_Request(true, "UPDATE orders SET status = 'completed' WHERE order_id = %d", BuyOrderID);
		if(r.affected_rows() > 0)
		{
			SQL_Request(true, "UPDATE users SET active_orders = ARRAY_REMOVE(active_orders, %d) WHERE id = %d", BuyOrderID, BuyerID);
		}
	}
	auto SellOrderUpdated = SQL_Request(false, "SELECT value FROM orders WHERE order_id = '%d'", SellOrderID);
	if(SellOrderUpdated[0][0].as<int64_t>() == 0)
	{
		/*CloseOrder*/
		auto r = SQL_Request(true, "UPDATE orders SET status = 'completed' WHERE order_id = %d", SellOrderID);
		if(r.affected_rows() > 0)
		{
			SQL_Request(true, "UPDATE users SET active_orders = ARRAY_REMOVE(active_orders, %d) WHERE id = %d", SellOrderID, SellerID);
		}
	}
		
	/*Update balances*/
	SQL_Request(true, "UPDATE users SET balance_rub = balance_rub - %d, balance_usd = balance_usd + %d WHERE id = %d",
		DeltaValue * BuyerPrice, DeltaValue, BuyerID);
	SQL_Request(true, "UPDATE users SET balance_rub = balance_rub + %d, balance_usd = balance_usd - %d WHERE id = %d",
		DeltaValue * BuyerPrice, DeltaValue, SellerID);

	SQL_Request(true, "INSERT INTO matches (buyer, seller, value, price) VALUES (%i, %i, %i, %i)",
		BuyerID, SellerID, DeltaValue, DeltaValue * BuyerPrice);

	/*Market make*/
	Bid = DeltaValue;
	Offer = std::max(BuyerValue, SellerValue);

	//const auto Offer = std::max(BuyerValue, SellerValue);

	//auto Quote = DeltaValue + floor((Offer - DeltaValue) / 2);
}
