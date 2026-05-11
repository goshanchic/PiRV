#include <iostream>
#include <string>
#include <array>
#include <boost/asio.hpp>

using boost::asio::ip::tcp;

int main() {
    try {
        boost::asio::io_context io_context;
        tcp::resolver resolver(io_context);
        tcp::resolver::results_type endpoints = resolver.resolve("127.0.0.1", "12345");

        tcp::socket socket(io_context);
        boost::asio::connect(socket, endpoints);

        std::cout << "Введите число для вычисления факториала: ";
        std::string message;
        std::getline(std::cin, message);

        boost::system::error_code error;
        boost::asio::write(socket, boost::asio::buffer(message), error);

        std::array<char, 1024> buf;
        size_t len = socket.read_some(boost::asio::buffer(buf), error);

        if (error == boost::asio::error::eof) {
            // Соединение закрыто
        } else if (error) {
            throw boost::system::system_error(error);
        }

        std::string response(buf.data(), len);
        std::cout << "Сервер вернул: " << response;

    } catch (std::exception& e) {
        std::cerr << "Ошибка клиента: " << e.what() << std::endl;
    }

    return 0;
}