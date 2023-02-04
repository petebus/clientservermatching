#include <cstdlib>
#include <iostream>
#include <queue>
#include <boost/bind/bind.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/asio.hpp>
#include "json.hpp"
#include "Common.hpp"

using boost::asio::ip::tcp;

enum class OrderType
{
	BUY,
	SELL
};
struct OrderInfo
{
	OrderInfo() : UserName(""), Type(OrderType::BUY), Val_USD(0), Val_RUB(0){ }
	std::string UserName;
	OrderType Type;
	int64_t Val_USD;
	int64_t Val_RUB;
};
struct UserData
{
	UserData() : UserName(""), Balance_USD(0), Balance_RUB(0)
	{
		
	}
	UserData(const std::string& InUserName) : UserName(InUserName), Balance_USD(0), Balance_RUB(0)
	{
		
	}
	std::string UserName;
	int64_t Balance_USD;
	int64_t Balance_RUB;
	std::list<uint64_t> OrderList;
};

class Core
{
public:
    // "Регистрирует" нового пользователя и возвращает его ID.
    std::string RegisterNewUser(const std::string& aUserName)
    {
        size_t newUserId = mUsers.size();
        mUsers[newUserId] = aUserName;

        return std::to_string(newUserId);
    }

    // Запрос имени клиента по ID
    std::string GetUserName(const std::string& aUserId)
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
    std::string GetUserBalance(const std::string& aUserId)
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

	void AddNewOrder(const std::string& InUserID, const OrderType& InOrderType, const int64_t& InUSD, const int64_t InRUB)
    {
    	const auto& userIt = mUsers.find(std::stoi(InUserID));
    	if(userIt == mUsers.cend()) return;

    	OrderInfo NewOrder;
    	NewOrder.Type = InOrderType;
    	NewOrder.UserName = GetUserName(InUserID);
    	NewOrder.Val_USD = InUSD;
    	NewOrder.Val_RUB = InRUB;
    	Orders.insert(std::make_pair(LastOrderIdx, NewOrder));
    	userIt->second.OrderList.push_back(LastOrderIdx);
    	LastOrderIdx += 1;
    }
	void RemoveOrder(const std::string& InUserID, const uint64_t OrderID)
    {
    	const auto& userIt = mUsers.find(std::stoi(InUserID));
    	if(userIt == mUsers.cend()) return;

    	userIt->second.OrderList.remove(OrderID);
    	Orders.erase(OrderID);
    }
	const std::unordered_map<uint64_t, OrderInfo>& GetOrderList() const { return Orders; }
	
	std::vector<uint64_t> GetUserOrdersIDList(const std::string& InUserID) const
    {
    	std::vector<uint64_t> Result;
    	const auto userIt = mUsers.find(std::stoi(InUserID));
    	if (userIt == mUsers.cend())
    	{
    		return Result;
    	}
    	for(auto OrderID : userIt->second.OrderList)
    	{
    		Result.push_back(OrderID);
    	}
	    return Result; 
    }
	
	std::vector<OrderInfo> GetUserOrderList(const std::string& InUserID) const
    {
    	std::vector<OrderInfo> Result;
    	const auto userIt = mUsers.find(std::stoi(InUserID));
    	if (userIt == mUsers.cend())
    	{
    		return Result;
    	}
    	for(auto OrderID : userIt->second.OrderList)
    	{
    		Result.push_back(Orders.at(OrderID));
    	}
    	return Result;
    }
	
private:
	// <UserId, UserName>
    std::map<size_t, UserData> mUsers;
	std::unordered_map<uint64_t, OrderInfo> Orders;
	uint64_t LastOrderIdx = 0;
};

Core& GetCore()
{
    static Core core;
    return core;
}

class session
{
public:
    session(boost::asio::io_service& io_service)
        : socket_(io_service)
    {
    }

    tcp::socket& socket()
    {
        return socket_;
    }

    void start()
    {
        socket_.async_read_some(boost::asio::buffer(data_, max_length),
            boost::bind(&session::handle_read, this,
                boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred));
    }

    void handle_read(const boost::system::error_code& error,
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
                NewReply.reply_body = GetCore().RegisterNewUser(j["Message"]);
            }
            else if (reqType == Requests::OrderRemove)
            {
            	NewReply.user = GetCore().GetUserName(j["UserId"]);
                NewReply.reply_body = "Order not found \n";

            	std::string Msg = j["Message"];
				int64_t OrderIdx = std::atoi(Msg.c_str());
            	const auto OrderList = GetCore().GetUserOrdersIDList(j["UserId"]);
            	if(OrderIdx >= 0 && OrderList.size() > OrderIdx)
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
					
            	if(CommandList.size() < 3 || CommandList[0] != "BUY" && CommandList[0] != "SELL")
            	{
            		NewReply.reply_body = "Error: mismatching template";
            	}
            	else
            	{
            		const int64_t Amount_USD = std::atoi(CommandList[1].c_str());				
            		const int64_t Amount_RUB = std::atoi(CommandList[2].c_str());
            		
            		GetCore().AddNewOrder(j["UserId"], CommandList[0] == "BUY" ? OrderType::BUY : OrderType::SELL, Amount_USD, Amount_RUB);
            		NewReply.user = GetCore().GetUserName(j["UserId"]);
            		NewReply.reply_body = "Order successfully added";

            		/*Try to match order*/
            	}
            }
            else if (reqType == Requests::Balance)
            {
            	NewReply.user = GetCore().GetUserName(j["UserId"]);
            	NewReply.reply_body = "Balance is: " + GetCore().GetUserBalance(j["UserId"]);
            }
            else if (reqType == Requests::OrderList)
            {
            	NewReply.user = GetCore().GetUserName(j["UserId"]);
            	NewReply.reply_body = "OrderList:\n";

            	const auto OrderList = GetCore().GetUserOrderList(j["UserId"]);
            	for(uint64_t OrderIt = 0; OrderIt < OrderList.size(); OrderIt++)
            	{
            		NewReply.reply_body += std::to_string(OrderIt) + " ";

            		const OrderInfo& Order = OrderList[OrderIt];
            		NewReply.reply_body += Order.Type == OrderType::BUY ? "BUY " : "SELL ";
            		NewReply.reply_body += std::to_string(Order.Val_USD) + " USD ";
            		NewReply.reply_body += std::to_string(Order.Val_RUB) + " RUB ";
            		NewReply.reply_body += "\n";
            	}
            }

        	replies_queue.push(NewReply);
        	if(replies_queue.size() == 1)
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

    void handle_write(const boost::system::error_code& error)
    {
        if (!error)
        {
        	replies_queue.pop();
        	if(replies_queue.size() > 0)
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

private:
	tcp::socket socket_;
	enum { max_length = 1024 };
	char data_[max_length];

	struct Reply
	{
		std::string user;
        std::string reply_body;
	};

	std::queue<Reply> replies_queue;
};

class server
{
public:
    server(boost::asio::io_service& io_service)
        : io_service_(io_service),
        acceptor_(io_service, tcp::endpoint(tcp::v4(), port))
    {
        std::cout << "Server started! Listen " << port << " port" << std::endl;

        session* new_session = new session(io_service_);
        acceptor_.async_accept(new_session->socket(),
            boost::bind(&server::handle_accept, this, new_session,
                boost::asio::placeholders::error));
    }

    void handle_accept(session* new_session,
        const boost::system::error_code& error)
    {
        if (!error)
        {
            new_session->start();
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

private:
    boost::asio::io_service& io_service_;
    tcp::acceptor acceptor_;
};

int main()
{
    try
    {
        boost::asio::io_service io_service;
        static Core core;

        server s(io_service);

        io_service.run();
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}