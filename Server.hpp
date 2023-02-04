#include <cstdlib>
#include <iostream>
#include <queue>
#include <boost/bind/bind.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/asio.hpp>
#include "json.hpp"
#include "Common.hpp"
#include <boost/date_time/posix_time/posix_time.hpp>

using boost::asio::ip::tcp;

enum class OrderType
{
    BUY,
    SELL
};

struct OrderInfo
{
    OrderInfo() : UserID(""), Type(OrderType::BUY), Val_USD(0), Val_RUB(0) { }
    std::string UserID;
    OrderType Type;
    int64_t Val_USD;
    int64_t Val_RUB;
    boost::posix_time::ptime Timestamp;
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
    std::string RegisterNewUser(const std::string& aUserName);

    std::string GetUsername(const std::string& aUserId);
    std::string GetUserBalance(const std::string& aUserId);
    void GetUserBalanceValues(const std::string& aUserId, int64_t& OutUSD, int64_t& OutRUB);
    void UpdateUserBalance(const std::string& aUserId, const int64_t& InUSD, const int64_t& InRUB);
    void AddNewOrder(const std::string& InUserID, const OrderType& InOrderType, const int64_t& InUSD, const int64_t InRUB);
    void RemoveOrder(const std::string& InUserID, const uint64_t OrderID);
    void UpdateOrderPrice(const uint64_t OrderID, const int64_t& InUSD);

    const std::unordered_map<uint64_t, OrderInfo>& GetOrderList() const { return Orders; }

    std::vector<uint64_t> GetUserOrdersIDList(const std::string& InUserID) const;
    std::vector<OrderInfo> GetUserOrderList(const std::string& InUserID) const;

private:
    // <UserId, UserName>
    std::map<size_t, UserData> mUsers;
    std::unordered_map<uint64_t, OrderInfo> Orders;
    uint64_t LastOrderIdx = 0;
};

Core& GetCore();

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

    void start();
    void handle_read(const boost::system::error_code& error, size_t bytes_transferred);
    void handle_write(const boost::system::error_code& error);
    void OnOrderAdded(const std::string UserID);

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
    server(boost::asio::io_service& io_service);

    void handle_accept(session* new_session, const boost::system::error_code& error);

private:
    boost::asio::io_service& io_service_;
    tcp::acceptor acceptor_;
};