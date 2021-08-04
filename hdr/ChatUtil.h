#pragma once

#include <iostream>
#include <atomic>
#include <thread>
#include <chrono>
#include <mutex>
#include <boost/asio.hpp>

#include "chat_message.h"

using boost::asio::ip::tcp;

class ChatWindowManager{

public:
    ChatWindowManager(boost::asio::io_context& io_context, tcp::socket& socket)
        :mIoContext(io_context), mSocket(socket){ }

    ChatWindowManager(const ChatWindowManager &rhs) = delete;
    ChatWindowManager& operator=(const ChatWindowManager &rhs) = delete;

    void Send(){

        char line[chat_message::max_body_length + 1];
        while (std::cin.getline(line, chat_message::max_body_length + 1) &&
               !mHasClientLeft.load(std::memory_order_acquire))
        {
            std::cout << "enter" << std::endl;
            std::string s;
            std::stringstream ss(s);
            ss << "\rYOU:\t" << line << " (sending...)";
            shared_print(s);
            chat_message msg;
            msg.body_length(std::strlen(line));
            std::memcpy(msg.body(), line, msg.body_length());
            msg.encode_header();
            write(std::move(msg));
        }
    }

    void Receive(){

        do_read_header();
    }

private:
    void write(chat_message&& msg)
    {
        boost::asio::post(mIoContext,
                          [this, msg = std::move(msg)]()
        {
            do_write(msg);
        });
    }

    void do_write(const chat_message& msg)
    {
        boost::asio::async_write(mSocket,
                                 boost::asio::buffer(msg.data(), msg.length()),
                                 [this](const boost::system::error_code& error, std::size_t length)
        {
            if (!error){
                receive_response();
            }else{
                mErrorMessage = "client write failed reason: " + error.message();
            }
        });
    }

    void receive_response()
    {
        static char* ack_message = "received";
        static int len = strlen(ack_message);
        std::string reply;
        reply.reserve(len + 1);
        bool ret;
        boost::asio::async_read(mSocket,
                                boost::asio::buffer(reply.data(), len),
                                [this, reply](const boost::system::error_code& error, std::size_t length)
        {
            if (error || (reply == ack_message)){
                mErrorMessage = "server didnt send acknowledgement reason: " + error.message();
            }
        });
    }

    void do_read_header()
    {
        boost::asio::async_read(mSocket,
                                boost::asio::buffer(mReadMsg.data(), chat_message::header_length),
                                [this](boost::system::error_code ec, std::size_t /*length*/)
        {
            if (!ec && mReadMsg.decode_header())
            {
                do_read_body();
            }
            else
            {
                mHasClientLeft = true;
            }
        });
    }

    void do_read_body()
    {
        boost::asio::async_read(mSocket,
                                boost::asio::buffer(mReadMsg.body(), mReadMsg.body_length()),
                                [this](boost::system::error_code ec, std::size_t /*length*/)
        {
            if (!ec)
            {
                shared_print(mReadMsg.body(), mReadMsg.body_length());
                do_write_ack();
                do_read_header();
            }
            else
            {
                mHasClientLeft = true;
            }
        });
    }

    void do_write_ack()
    {
        static char* ack_message = "received";
        static int len = strlen(ack_message);
        boost::asio::write(mSocket, boost::asio::buffer(ack_message, len));
    }

    void shared_print(const std::string& msg, int len=0){

        std::lock_guard<std::mutex> lk(mCoutGuard);
        std::cout.write(msg.data(), (len ? len : msg.length()));
        std::cout << "\n";
    }

private:

    std::mutex mCoutGuard;
    boost::asio::io_context& mIoContext;
    tcp::socket& mSocket;
    std::string mErrorMessage;
    std::atomic_bool mHasClientLeft{true};
    chat_message mReadMsg;
};
