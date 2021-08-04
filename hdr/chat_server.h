#pragma once

#include <cstdlib>
#include <deque>
#include <iostream>
#include <list>
#include <memory>
#include <set>
#include <utility>
#include <boost/asio.hpp>
#include "chat_message.h"
#include "ChatUtil.h"
#include "command.h"

using boost::asio::ip::tcp;

class chat_session
{
public:
    chat_session(boost::asio::io_context& io_context, tcp::socket socket)
        : mIoContext(io_context), socket_(std::move(socket)){ }

    void start()
    {
        ChatWindowManager mngr(mIoContext, socket_);
//        std::thread t(&ChatWindowManager::Send, &mngr);
        mngr.Receive();
        mngr.Send();
//        t.join();
    }
private:
    tcp::socket socket_;
    boost::asio::io_context& mIoContext;
    chat_message read_msg_;
};

class chat_server
{
public:
    chat_server(boost::asio::io_context& io_context,
                const tcp::endpoint& endpoint)
        : mIoContext(io_context), acceptor_(io_context, endpoint)
    {
        do_accept();
    }

private:
    void do_accept()
    {
        acceptor_.async_accept(
                    [this](boost::system::error_code ec, tcp::socket socket)
        {
            std::cout << "incomming connection";
            if (!ec)
            {
                std::make_shared<chat_session>(mIoContext, std::move(socket))->start();
            }
            // Keep server in stand-by
            do_accept();
        });
    }

    boost::asio::io_context& mIoContext;
    tcp::acceptor acceptor_;
};

class RkServer : public Command
{
public:
    RkServer(std::string port):mPort(port){}

    void execute()
    {
        boost::asio::io_context io_context;
        tcp::endpoint endpoint(tcp::v4(), std::atoi(mPort.data()));
        chat_server s(io_context, endpoint);
        io_context.run();
    }

private:
    std::string mPort;
};
