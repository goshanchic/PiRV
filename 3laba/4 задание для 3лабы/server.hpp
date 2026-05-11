#ifndef SERVER_HPP
#define SERVER_HPP

#include <boost/asio.hpp>
#include <memory>
#include <string>
#include <iostream>

using boost::asio::ip::tcp;

class Session : public std::enable_shared_from_this<Session> {
public:
    Session(tcp::socket socket, boost::asio::io_context& io_context);
    void start();

private:
    void do_read();
    void do_write(std::string response);

    tcp::socket socket_;
    // Strand гарантирует, что обработчики этой сессии выполняются последовательно
    boost::asio::strand<boost::asio::io_context::executor_type> strand_;
    boost::asio::streambuf buffer_;
};

class MultiThreadedServer {
public:
    MultiThreadedServer(boost::asio::io_context& io_context, short port);

private:
    void do_accept();

    tcp::acceptor acceptor_;
    boost::asio::io_context& io_context_;
};

#endif