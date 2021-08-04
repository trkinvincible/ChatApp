#pragma once

#include <cstdlib>
#include <deque>
#include <iostream>
#include <thread>
#include <boost/asio.hpp>
#include "chat_message.h"
#include "ChatUtil.h"
#include "command.h"

using boost::asio::ip::tcp;

class chat_client
{
public:
  chat_client(boost::asio::io_context& io_context,
      const tcp::resolver::results_type& endpoints)
    : mIoContext(io_context),
      mSocket(io_context)
  {
    do_connect(endpoints);
  }

private:
  void close()
  {
    boost::asio::post(mIoContext, [this]() { mSocket.close(); });
  }

  void do_connect(const tcp::resolver::results_type& endpoints)
  {
      // refer https://www.boost.org/doc/libs/1_76_0/doc/html/boost_asio/example/cpp11/ssl/client.cpp
      boost::asio::async_connect(mSocket, endpoints,
                                 [this](boost::system::error_code ec, tcp::endpoint)
      {
          if (!ec)
          {
              ChatWindowManager mngr(mIoContext, mSocket);
              std::thread t(&ChatWindowManager::Send, &mngr);
              mngr.Receive();
              t.join();
              close();
          }
      });
  }

private:
  boost::asio::io_context& mIoContext;
  tcp::socket mSocket;
};

class RkClient : public Command
{
public:
    RkClient(std::string ip, std::string port)
        :mServerIP(ip), mServerPort(port){}

    void execute()
    {
        try
        {
            boost::asio::io_context io_context;
            tcp::resolver resolver(io_context);
            auto endpoints = resolver.resolve(mServerIP, mServerPort);
            chat_client c(io_context, endpoints);
            std::thread t([&io_context](){ io_context.run(); });
            t.join();
        }
        catch (std::exception& e)
        {
            std::cerr << "Exception: " << e.what() << "\n";
        }
    }
private:
    std::string& mServerIP;
    std::string& mServerPort;
};
