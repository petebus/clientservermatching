#include <cstdlib>
#include <iostream>
#include <queue>
#include <boost/bind/bind.hpp>
#include <boost/asio.hpp>
#include "json.hpp"
#include "Common.hpp"

using boost::asio::ip::tcp;

enum OrderType
{
	BUY,
	SELL
};
struct OrderInfo
{
	std::string UserName;
	OrderType Type;
	int64_t Val_USD;
	int64_t Val_RUB;
};
struct UserData
{
	UserData(const std::string& InUserName) : UserName(InUserName), Balance_USD(0), Balance_RUB(0)
	{
		
	}
	std::string UserName;
	int64_t Balance_USD;
	int64_t Balance_RUB;
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

	void AddNewOrder(const std::string& InUserID, const OrderType& InOrderType, const int64_t& InUSD, const int64_t InRUB)
    {
    	const auto userIt = mUsers.find(std::stoi(InUserID));
    	if(userIt == mUsers.cend()) return;

    	OrderInfo NewOrder;
    	NewOrder.Type = InOrderType;
    	NewOrder.UserName = GetUserName(InUserID);
    	NewOrder.Val_USD = InUSD;
    	NewOrder.Val_RUB = InRUB;
    	Orders.push_back(NewOrder);
    }
	
private:
	// <UserId, UserName>
    std::map<size_t, UserData> mUsers;
	std::list<OrderInfo> Orders;
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

    // Обработка полученного сообщения.
    void handle_read(const boost::system::error_code& error,
        size_t bytes_transferred)
    {
        if (!error)
        {
            // Парсим json, который пришёл нам в сообщении.
        	data_[bytes_transferred] = '\0';

            auto j = nlohmann::json::parse(data_);
            auto reqType = j["ReqType"];
        	
        	Reply NewReply;
            NewReply.reply_body = "Error! Unknown request type";
            if (reqType == Requests::Registration)
            {
                // Это реквест на регистрацию пользователя.
                // Добавляем нового пользователя и возвращаем его ID.
                NewReply.reply_body = GetCore().RegisterNewUser(j["Message"]);
            }
            else if (reqType == Requests::Hello)
            {
                // Это реквест на приветствие.
                // Находим имя пользователя по ID и приветствуем его по имени.
            	NewReply.user = GetCore().GetUserName(j["UserId"]);
                NewReply.reply_body = "Hello, " + NewReply.user + "!\n";
            }
            else if (reqType == Requests::OrderAdd)
            {
	            
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