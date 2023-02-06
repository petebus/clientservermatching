#include "Server.hpp"
#include <iostream>
#include <boost/asio.hpp>
#include <pqxx/connection.hxx>
#include <pqxx/transaction.hxx>
#include <boost/bind/bind.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include "Common.hpp"

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

void ClientSession::OnOrderAdded(const std::string UserID)
{
    /*auto AddedOrderIdx = GetCore().GetUserOrdersIDList(UserID).back();
    auto& OrderList = GetCore().GetOrderList();
    auto& AddedOrderData = GetCore().GetOrderList().at(AddedOrderIdx);

    struct OrderInfoHolder
    {
        OrderInfoHolder() : OrderID(0), Info(OrderInfo()) {}
        OrderInfoHolder(const uint64_t& InOrderID, const OrderInfo& InOrderInfo) : OrderID(InOrderID), Info(InOrderInfo) {}
        uint64_t OrderID;
        OrderInfo Info;
    };
    std::vector<OrderInfoHolder> Buffer;
    for (auto& Order : OrderList)
    {
        if (Order.first == AddedOrderIdx) continue;
        if (Order.second.Type == AddedOrderData.Type) continue;
        Buffer.push_back(OrderInfoHolder(Order.first, Order.second));
    }

    std::sort(Buffer.begin(), Buffer.end(), [&](const OrderInfoHolder& Lhs, const OrderInfoHolder& Rhs)->bool
    {
		if (Lhs.Info.Val_RUB == Rhs.Info.Val_RUB) return Lhs.Info.Timestamp < Rhs.Info.Timestamp;
		return AddedOrderData.Type == OrderType::BUY ?
		        Lhs.Info.Val_RUB < Rhs.Info.Val_RUB :
		        Lhs.Info.Val_RUB > Rhs.Info.Val_RUB;
        });

    for (auto& OrderToCompare : Buffer)
    {
        auto DeltaValue = std::min(AddedOrderData.Val_USD, OrderToCompare.Info.Val_USD);

        auto FirstUserID = AddedOrderData.UserID;
        auto SecondUserID = OrderToCompare.Info.UserID;
        GetCore().UpdateOrderPrice(AddedOrderIdx, AddedOrderData.Val_USD - DeltaValue);
        GetCore().UpdateOrderPrice(OrderToCompare.OrderID, OrderToCompare.Info.Val_USD - DeltaValue);

        int64_t FirstUser_USD, FirstUser_RUB;
        int64_t SecondUser_USD, SecondUser_RUB;
        GetCore().GetUserBalanceValues(FirstUserID, FirstUser_USD, FirstUser_RUB);
        GetCore().GetUserBalanceValues(SecondUserID, SecondUser_USD, SecondUser_RUB);

        if (AddedOrderData.Type == OrderType::BUY)
        {
            GetCore().UpdateUserBalance(FirstUserID, FirstUser_USD + DeltaValue, FirstUser_RUB - DeltaValue * AddedOrderData.Val_RUB);
        }
        else
        {
            GetCore().UpdateUserBalance(FirstUserID, FirstUser_USD - DeltaValue, FirstUser_RUB + DeltaValue * OrderToCompare.Info.Val_RUB);
        }

        if (OrderToCompare.Info.Type == OrderType::BUY)
        {
            GetCore().UpdateUserBalance(SecondUserID, SecondUser_USD + DeltaValue, SecondUser_RUB - DeltaValue * OrderToCompare.Info.Val_RUB);
        }
        else
        {
            GetCore().UpdateUserBalance(SecondUserID, SecondUser_USD - DeltaValue, SecondUser_RUB + DeltaValue * AddedOrderData.Val_RUB);
        }

        if (AddedOrderData.Val_USD == 0) return;
    }*/
}

server::server(boost::asio::io_service& io_service)
    : io_service_(io_service),
    acceptor_(io_service, tcp::endpoint(tcp::v4(), port))
{
}

void server::Start()
{
	std::cout << "Server started! Listen " << port << " port" << std::endl;
	ClientSession* new_session = new ClientSession(this, io_service_);
	acceptor_.async_accept(new_session->GetSocket(),
		boost::bind(&server::handle_accept, this, new_session,
			boost::asio::placeholders::error));

	ConnectionObject = std::make_shared<pqxx::connection>(connectionString.c_str());
	Transaction = std::make_shared<pqxx::work>(*ConnectionObject);
}

void server::Stop()
{
	acceptor_.close();
	ConnectionObject.reset();
	Transaction.reset();
}

void server::handle_accept(ClientSession* new_session, const boost::system::error_code& error)
{
    if (!error)
    {
        new_session->Start();

        /*do ping every N seconds*/
        new_session = new ClientSession(this, io_service_);
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
		SQL_Request("INSERT INTO orders (type, value, price, user_id) VALUES ('%s', %i, %i, %i)",
			CommandList[0].c_str(), Value, Price, InSession->GetUserID());

		auto r = SQL_Request("SELECT COUNT(*) FROM orders");
		
		/*add order to user list*/
		SQL_Request("UPDATE users SET active_orders = ARRAY[%d] || active_orders WHERE id = %d", r.affected_rows(), InSession->GetUserID());
		reply_body = "Order successfully added";
		
		//OnOrderAdded(j["UserId"]);
	}
	RequestQueue.front().SessionCallback(reply_body);
}
void server::Execute_RemoveOrder(ClientSession* InSession, std::string Msg)
{
	std::string reply_body = "Unknown error \n";
	/*const auto OrderList = GetCore().GetUserOrdersIDList(j["UserId"]);
	if (OrderIdx >= 0 && OrderList.size() > OrderIdx)
	{
		GetCore().RemoveOrder(j["UserId"], OrderList[OrderIdx]);
		NewReply.reply_body = "Order was removed successfully \n";
	}*/
	RequestQueue.front().SessionCallback(reply_body);
}

void server::Execute_Registration(ClientSession* InSession, std::string Msg)
{
	std::string reply_body = "Unknown error \n";

	/*Parse commands */
	std::vector<std::string> CommandList;
	boost::split(CommandList, Msg, boost::is_any_of(" "));
        	
	/*if username is empty - allow to create*/
	pqxx::result r = SQL_Request("SELECT id FROM users WHERE username = '%s'", CommandList[0].c_str());
	if(r.affected_rows() == 0)
	{
		r = SQL_Request("SELECT MAX(id) FROM users");
		uint64_t ID = 0;
		if(!r.empty())
		{
			ID = std::atoi(r[0][0].c_str()) + 1;
		}
		SQL_Request("INSERT INTO users (id, username, password) VALUES (%d,'%s','%s')", ID, CommandList[0].c_str(), CommandList[1].c_str());
		Transaction->commit();
		reply_body = "OK " + std::to_string(ID);
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
	pqxx::result r = SQL_Request("SELECT id FROM users WHERE username = '%s' AND password = '%s'", CommandList[0].c_str(), CommandList[1].c_str());
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

	pqxx::result r = SQL_Request("SELECT balance_usd, balance_rub FROM users WHERE id = '%d'", User);
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
	/*NewReply.user = GetCore().GetUsername(j["UserId"]);
			 NewReply.reply_body = "OrderList:\n";
 
			 const auto OrderList = GetCore().GetUserOrderList(j["UserId"]);
			 for (uint64_t OrderIt = 0; OrderIt < OrderList.size(); OrderIt++)
			 {
				 NewReply.reply_body += std::to_string(OrderIt) + " ";
 
				 const OrderInfo& Order = OrderList[OrderIt];
				 NewReply.reply_body += Order.Type == OrderType::BUY ? "BUY " : "SELL ";
				 NewReply.reply_body += std::to_string(Order.Val_USD) + " USD ";
				 NewReply.reply_body += std::to_string(Order.Val_RUB) + " RUB ";
				 NewReply.reply_body += to_simple_string(Order.Timestamp);
				 NewReply.reply_body += "\n";
			 }*/
}
