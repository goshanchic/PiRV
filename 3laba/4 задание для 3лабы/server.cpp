#include "server.hpp"
#include <iostream>
#include <thread>

Session::Session(tcp::socket socket, boost::asio::io_context& io_context)
    : socket_(std::move(socket)), strand_(boost::asio::make_strand(io_context)) {}

void Session::start() {
    do_read();
}

void Session::do_read() {
    auto self(shared_from_this());
    // Читаем из сокета в buffer_ до символа новой строки '\n'
    boost::asio::async_read_until(socket_, buffer_, '\n',
        boost::asio::bind_executor(strand_,
            [this, self](boost::system::error_code ec, std::size_t length) {
                if (!ec) {
                    std::istream is(&buffer_);
                    std::string line;
                    std::getline(is, line);

                    std::cout << "[Thread " << std::this_thread::get_id() << "] Получено: " << line << std::endl;
                    
                    do_write("Эхо-ответ: " + line + "\n");
                }
            }));
}

void Session::do_write(std::string response) {
    auto self(shared_from_this());
    // Используем shared_ptr для строки, чтобы она не удалилась до завершения отправки
    auto msg = std::make_shared<std::string>(std::move(response));
    
    boost::asio::async_write(socket_, boost::asio::buffer(*msg),
        boost::asio::bind_executor(strand_,
            [this, self, msg](boost::system::error_code ec, std::size_t) {
                if (!ec) {
                    do_read(); // Снова ждем данные от клиента
                }
            }));
}

MultiThreadedServer::MultiThreadedServer(boost::asio::io_context& io_context, short port)
    : acceptor_(io_context, tcp::endpoint(tcp::v4(), port)), io_context_(io_context) {
    do_accept();
}

void MultiThreadedServer::do_accept() {
    acceptor_.async_accept([this](boost::system::error_code ec, tcp::socket socket) {
        if (!ec) {
            std::make_shared<Session>(std::move(socket), io_context_)->start();
        }
        do_accept();
    });
}