#include "Server.hpp"

#include <pqxx/connection.hxx>
#include <pqxx/transaction.hxx>

std::string Core::RegisterNewUser(const std::string& aUserName)
{
    size_t newUserId = mUsers.size();
    mUsers[newUserId] = aUserName;

    return std::to_string(newUserId);
}

std::string Core::GetUsername(const std::string& aUserId)
{
    const auto userIt = mUsers.find(std::stoi(aUserId));
    if (userIt == mUsers.cend())
    {
        return "Error! Unknown User";
    }
    else
    {
        return userIt->second.UserName;
    }
}

std::string Core::GetUserBalance(const std::string& aUserId)
{
    const auto userIt = mUsers.find(std::stoi(aUserId));
    if (userIt == mUsers.cend())
    {
        return "Error! Unknown User";
    }
    else
    {
        return std::to_string(userIt->second.Balance_USD) + " USD " + std::to_string(userIt->second.Balance_RUB) + " RUB";
    }
}

void Core::GetUserBalanceValues(const std::string& aUserId, int64_t& OutUSD, int64_t& OutRUB)
{
    const auto userIt = mUsers.find(std::stoi(aUserId));
    if (userIt == mUsers.cend()) return;
    OutUSD = userIt->second.Balance_USD;
    OutRUB = userIt->second.Balance_RUB;
}

void Core::UpdateUserBalance(const std::string& aUserId, const int64_t& InUSD, const int64_t& InRUB)
{
    const auto& userIt = mUsers.find(std::stoi(aUserId));
    if (userIt == mUsers.cend()) return;

    userIt->second.Balance_USD = InUSD;
    userIt->second.Balance_RUB = InRUB;
}

void Core::AddNewOrder(const std::string& InUserID, const OrderType& InOrderType, const int64_t& InUSD, const int64_t InRUB)
{
    const auto& userIt = mUsers.find(std::stoi(InUserID));
    if (userIt == mUsers.cend()) return;

    OrderInfo NewOrder;
    NewOrder.Type = InOrderType;
    NewOrder.UserID = InUserID;
    NewOrder.Val_USD = InUSD;
    NewOrder.Val_RUB = InRUB;
    NewOrder.Timestamp = boost::posix_time::second_clock::local_time();
    Orders.insert(std::make_pair(LastOrderIdx, NewOrder));
    userIt->second.OrderList.push_back(LastOrderIdx);
    LastOrderIdx += 1;
}

void Core::RemoveOrder(const std::string& InUserID, const uint64_t OrderID)
{
    const auto& userIt = mUsers.find(std::stoi(InUserID));
    if (userIt == mUsers.cend()) return;

    userIt->second.OrderList.remove(OrderID);
    Orders.erase(OrderID);
}

void Core::UpdateOrderPrice(const uint64_t OrderID, const int64_t& InUSD)
{
    Orders[OrderID].Val_USD = InUSD;
    if (Orders[OrderID].Val_USD <= 0)
    {
        auto UserID = Orders[OrderID].UserID;
        RemoveOrder(UserID, OrderID);
    }
}

std::vector<uint64_t> Core::GetUserOrdersIDList(const std::string& InUserID) const
{
    std::vector<uint64_t> Result;
    const auto userIt = mUsers.find(std::stoi(InUserID));
    if (userIt == mUsers.cend())
    {
        return Result;
    }
    for (auto OrderID : userIt->second.OrderList)
    {
        Result.push_back(OrderID);
    }
    return Result;
}
std::vector<OrderInfo> Core::GetUserOrderList(const std::string& InUserID) const
{
    std::vector<OrderInfo> Result;
    const auto userIt = mUsers.find(std::stoi(InUserID));
    if (userIt == mUsers.cend())
    {
        return Result;
    }
    for (auto OrderID : userIt->second.OrderList)
    {
        Result.push_back(Orders.at(OrderID));
    }
    return Result;
}

void session::start()
{
    socket_.async_read_some(boost::asio::buffer(data_, max_length),
        boost::bind(&session::handle_read, this,
            boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred));
}

void session::handle_read(const boost::system::error_code& error,
    size_t bytes_transferred)
{
    if (!error)
    {
        /*Parse json*/
        data_[bytes_transferred] = '\0';

        auto j = nlohmann::json::parse(data_);
        auto reqType = j["ReqType"];

        Reply NewReply;
        NewReply.reply_body = "Error! Unknown request type";
        if (reqType == Requests::Registration)
        {
        	std::string Msg = j["Message"];
        	std::vector<std::string> CommandList;

        	boost::split(CommandList, Msg, boost::is_any_of(" "));
        	pqxx::connection connectionObject(connectionString.c_str());
        	pqxx::work txn{ connectionObject };
        	
        	/*if username is empty - allow to create*/
        	auto Request = "SELECT username FROM users WHERE username = '" + CommandList[0] + "'";
        	pqxx::result r = txn.exec(Request);
        	if(r.affected_rows() == 0)
        	{
        		r = txn.exec("SELECT MAX(id) FROM users");
        		uint64_t ID = 0;
        		if(!r.empty())
        		{
        			ID = std::atoi(r[0][0].c_str());
        		}
        		Request = std::format("INSERT INTO users (id, username, password) VALUES ({},'{}','{}')", ID, CommandList[0], CommandList[1]);
        		txn.exec(Request);
        		txn.commit();
        		NewReply.reply_body = "OK " + std::to_string(ID);
        	}
        	else
        	{
        		NewReply.reply_body = "Error: name is already created";
        	}
        }
    	else if(reqType == Requests::Authorization)
    	{
    		std::string Msg = j["Message"];
    		std::vector<std::string> CommandList;
    		boost::split(CommandList, Msg, boost::is_any_of(" "));
        	
        	pqxx::connection connectionObject(connectionString.c_str());
    		pqxx::work txn{connectionObject};

    		/*if username is empty - allow to create*/
    		auto Request = "SELECT id FROM users WHERE username = '" + CommandList[0] + "'" " AND password = '" + CommandList[1] + "'";
    		pqxx::result r = txn.exec(Request);
    		if(r.affected_rows() != 0)
    		{
    			const uint64_t ID = std::atoi(r[0][0].c_str());
    			NewReply.reply_body = "OK " + std::to_string(ID);
    		}
    		else
    		{
    			NewReply.reply_body = "Error: wrong username or pass";
    		}
    	}
        else if (reqType == Requests::OrderRemove)
        {
            NewReply.user = GetCore().GetUsername(j["UserId"]);
            NewReply.reply_body = "Order not found \n";

            std::string Msg = j["Message"];
            int64_t OrderIdx = std::atoi(Msg.c_str());
            const auto OrderList = GetCore().GetUserOrdersIDList(j["UserId"]);
            if (OrderIdx >= 0 && OrderList.size() > OrderIdx)
            {
                GetCore().RemoveOrder(j["UserId"], OrderList[OrderIdx]);
                NewReply.reply_body = "Order was removed successfully \n";
            }
        }
        else if (reqType == Requests::OrderAdd)
        {
            std::string Msg = j["Message"];
            std::vector<std::string> CommandList;
            boost::split(CommandList, Msg, boost::is_any_of(" "));

            if (CommandList.size() < 3 || CommandList[0] != "BUY" && CommandList[0] != "SELL")
            {
                NewReply.reply_body = "Error: mismatching template";
            }
            else
            {
                const int64_t Amount_USD = std::atoi(CommandList[1].c_str());
                const int64_t Amount_RUB = std::atoi(CommandList[2].c_str());

                GetCore().AddNewOrder(j["UserId"], CommandList[0] == "BUY" ? OrderType::BUY : OrderType::SELL, Amount_USD, Amount_RUB);
                NewReply.user = GetCore().GetUsername(j["UserId"]);
                NewReply.reply_body = "Order successfully added";

                OnOrderAdded(j["UserId"]);
            }
        }
        else if (reqType == Requests::Balance)
        {
            NewReply.user = GetCore().GetUsername(j["UserId"]);
            NewReply.reply_body = "Balance is: " + GetCore().GetUserBalance(j["UserId"]);
        }
        else if (reqType == Requests::OrderList)
        {
            NewReply.user = GetCore().GetUsername(j["UserId"]);
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
            }
        }

        replies_queue.push(NewReply);
        if (replies_queue.size() == 1)
        {
            boost::asio::async_write(socket_,
                boost::asio::buffer(replies_queue.back().reply_body, replies_queue.back().reply_body.size()),
                boost::bind(&session::handle_write, this,
                    boost::asio::placeholders::error));
        }
    }
    else
    {
        delete this;
    }
}

void session::handle_write(const boost::system::error_code& error)
{
    if (!error)
    {
        replies_queue.pop();
        if (replies_queue.size() > 0)
        {
            boost::asio::async_write(socket_,
                boost::asio::buffer(replies_queue.back().reply_body, replies_queue.back().reply_body.size()),
                boost::bind(&session::handle_write, this,
                    boost::asio::placeholders::error));
        }
        socket_.async_read_some(boost::asio::buffer(data_, max_length),
            boost::bind(&session::handle_read, this,
                boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred));
    }
    else
    {
        delete this;
    }
}

void session::OnOrderAdded(const std::string UserID)
{
    auto AddedOrderIdx = GetCore().GetUserOrdersIDList(UserID).back();
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

    /*Sort orders by price (lower-> upper for buy, opposite for sell)*/
    std::sort(Buffer.begin(), Buffer.end(), [&](const OrderInfoHolder& Lhs, const OrderInfoHolder& Rhs)->bool
        {
            /*Take latest order*/
            if (Lhs.Info.Val_RUB == Rhs.Info.Val_RUB) return Lhs.Info.Timestamp < Rhs.Info.Timestamp;

    return AddedOrderData.Type == OrderType::BUY ?
        Lhs.Info.Val_RUB < Rhs.Info.Val_RUB :
        Lhs.Info.Val_RUB > Rhs.Info.Val_RUB;
        });

    for (auto& OrderToCompare : Buffer)
    {
        /*take min value, both orders should decrease their order to this*/
        auto DeltaValue = std::min(AddedOrderData.Val_USD, OrderToCompare.Info.Val_USD);

        auto FirstUserID = AddedOrderData.UserID;
        auto SecondUserID = OrderToCompare.Info.UserID;
        GetCore().UpdateOrderPrice(AddedOrderIdx, AddedOrderData.Val_USD - DeltaValue);
        GetCore().UpdateOrderPrice(OrderToCompare.OrderID, OrderToCompare.Info.Val_USD - DeltaValue);

        /*Update balances*/
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

        /*if new order still active - continue*/
        if (AddedOrderData.Val_USD == 0) return;
    }
}

server::server(boost::asio::io_service& io_service)
    : io_service_(io_service),
    acceptor_(io_service, tcp::endpoint(tcp::v4(), port))
{
    std::cout << "Server started! Listen " << port << " port" << std::endl;

    session* new_session = new session(io_service_);
    acceptor_.async_accept(new_session->socket(),
        boost::bind(&server::handle_accept, this, new_session,
            boost::asio::placeholders::error));
}

void server::handle_accept(session* new_session, const boost::system::error_code& error)
{
    if (!error)
    {
        new_session->start();

        /*do ping every N seconds*/
        new_session = new session(io_service_);
        acceptor_.async_accept(new_session->socket(),
            boost::bind(&server::handle_accept, this, new_session,
                boost::asio::placeholders::error));
    }
    else
    {
        delete new_session;
    }
}