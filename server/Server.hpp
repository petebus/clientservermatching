#pragma once
#include <queue>
#include <boost/asio.hpp>
#include <pqxx/connection.hxx>
#include <pqxx/transaction.hxx>
#include "json.hpp"
#include<boost/function.hpp>
#include <pqxx/nontransaction.hxx>

using boost::asio::ip::tcp;

class ClientSession
{
	static constexpr size_t MaxBufferLength = 1024;
public:
    ClientSession(class server* InServer, boost::asio::io_service& InIoService)
        : OuterServer(InServer), io_service(InIoService), Socket(InIoService)
    {
    }

    tcp::socket& GetSocket()
    {
        return Socket;
    }

    void Start();
	bool IsAuthorized() const { return bAuthorized; }
	const uint32_t& GetUserID() const { return UserID; }
	
private:
    void handle_read(const boost::system::error_code& error, size_t bytes_transferred);
    void handle_write(const boost::system::error_code& error);

	void Callback(std::string Result);
	
	/*Authorization*/
	uint32_t UserID;
	bool bAuthorized = false;

	/*Connection data*/
	class server* OuterServer;
	boost::asio::io_service& io_service;
	tcp::socket Socket;

	/*Ping connection timeout handle*/
	void handle_connection_timeout(const boost::system::error_code& error);
	boost::asio::deadline_timer* PingTimeoutHandle = nullptr;
	
	/*Buffer for reading*/
    char DataBuffer[MaxBufferLength];

	std::string Output;
	std::mutex Lock;
};

using ExecuteSignature = boost::function<void (ClientSession*, std::string)>;
using CallbackSignature = boost::function<void (std::string)>;

class server
{
public:
    server(boost::asio::io_service& io_service);

	void Start();
	void Stop();

	void handle_accept(ClientSession* new_session, const boost::system::error_code& error);
	void handle_end_connection(ClientSession* InSession);
	
	void Request(ClientSession* InSession, std::string Msg, ExecuteSignature InExecuteMethod,
		CallbackSignature SessionCallback);
	
	void Execute_AddOrder(ClientSession* InSession, std::string Msg);
	void Execute_RemoveOrder(ClientSession* InSession, std::string Msg);
	void Execute_Registration(ClientSession* InSession, std::string Msg);
	void Execute_Authorization(ClientSession* InSession, std::string Msg);
	void Execute_Balance(ClientSession* InSession, std::string Msg);
	void Execute_OrderList(ClientSession* InSession, std::string Msg);
	void Execute_Quotes(ClientSession* InSession, std::string Msg);

private:
	void Handle_OrderAdded(ClientSession* InSession);
	void Handle_MakeMatch(const pqxx::row& BuyOrder, const pqxx::row& SellOrder);
	
	/*PostgreDB connection*/
	template <typename... Args>
	constexpr pqxx::result SQL_Request(bool bCommit, const char* InFormat, Args... InArgs)
	{
		char buffer[1024];
		sprintf_s(buffer, InFormat, InArgs...);

		const std::string FullRequest = buffer;
		pqxx::work Transaction(*ConnectionObject.get());
		pqxx::result Result = Transaction.exec(FullRequest);
		if(bCommit) Transaction.commit();
		return Result;
	}
	std::shared_ptr<pqxx::connection> ConnectionObject;
	int Bid{ 0 }, Offer{ 0 };


    boost::asio::io_service& io_service;
    tcp::acceptor acceptor_;

    struct QueuedRequest
    {
	    QueuedRequest() = delete;
    	QueuedRequest(ClientSession* InSession, ExecuteSignature InExecuteMethod, CallbackSignature InSessionCallback) :
    		Session(InSession),
    		ExecuteMethod(InExecuteMethod),
    		SessionCallback(InSessionCallback) {}

    	ClientSession* Session;
    	ExecuteSignature ExecuteMethod;
		CallbackSignature SessionCallback;
    };

public:
	std::queue<QueuedRequest> RequestQueue;
	std::mutex RequestLock;
};
