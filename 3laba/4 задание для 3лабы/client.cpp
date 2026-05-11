#include <iostream>
#include <boost/asio.hpp>
#include <string>

using boost::asio::ip::tcp;

int main() {
    try {
        boost::asio::io_context io_context;
        tcp::socket socket(io_context);
        tcp::resolver resolver(io_context);
        boost::asio::connect(socket, resolver.resolve("127.0.0.1", "12345"));

        std::cout << "Введите сообщение для сервера: ";
        std::string msg;
        std::getline(std::cin, msg);
        msg += "\n"; // Важно для async_read_until

        boost::asio::write(socket, boost::asio::buffer(msg));

        boost::asio::streambuf receive_buffer;
        boost::asio::read_until(socket, receive_buffer, '\n');
        
        std::cout << "Сервер ответил: " << boost::asio::buffer_cast<const char*>(receive_buffer.data());

    } catch (std::exception& e) {
        std::cerr << "Ошибка клиента: " << e.what() << std::endl;
    }
    return 0;
}