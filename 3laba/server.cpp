#include <iostream>
#include <string>
#include <memory>
#include <boost/asio.hpp>
#include <thread>

using boost::asio::ip::tcp;

class Session : public std::enable_shared_from_this<Session> {
public:
    Session(tcp::socket socket, boost::asio::thread_pool& pool)
        : socket_(std::move(socket)), pool_(pool), timer_(socket_.get_executor()) {}

    void start() {
        do_read();
    }

private:
    void do_read() {
        auto self(shared_from_this());
        socket_.async_read_some(boost::asio::buffer(data_, max_length),
            [this, self](boost::system::error_code ec, std::size_t length) {
                if (!ec) {
                    std::string msg(data_, length);
                    process_command(msg);
                }
            });
    }

    void process_command(std::string msg) {
        // Очистка строки от лишних пробелов в начале/конце
        msg.erase(0, msg.find_first_not_of(" \n\r\t"));
        msg.erase(msg.find_last_not_of(" \n\r\t") + 1);

        if (msg.find("таймер ") == 0) {
            // ОБРАБОТКА ТАЙМЕРА
            try {
                int seconds = std::stoi(msg.substr(7));
                start_timer(seconds);
            } catch (...) {
                send_response("Ошибка: неверный формат таймера. Используйте 'таймер N'\n");
            }
        } else {
            // ОБРАБОТКА ЧИСЛА (ФАКТОРИАЛ)
            auto self(shared_from_this());
            boost::asio::post(pool_, [this, self, msg]() {
                calculate_factorial(msg);
            });
        }
    }

    void start_timer(int seconds) {
        auto self(shared_from_this());
        std::cout << "Запущен таймер на " << seconds << " сек. для клиента." << std::endl;

        timer_.expires_after(std::chrono::seconds(seconds));
        timer_.async_wait([this, self, seconds](const boost::system::error_code& ec) {
            if (!ec) {
                send_response("Прошло " + std::to_string(seconds) + " секунд!\n");
            }
        });
    }

    void calculate_factorial(const std::string& msg) {
        std::string result;
        try {
            int num = std::stoi(msg);
            if (num < 0 || num > 20) {
                result = "Ошибка: введите число от 0 до 20\n";
            } else {
                unsigned long long fact = 1;
                for (int i = 1; i <= num; ++i) fact *= i;
                result = "Факториал: " + std::to_string(fact) + "\n";
            }
        } catch (...) {
            result = "Ошибка: введите число или команду 'таймер N'\n";
        }

        auto self(shared_from_this());
        boost::asio::post(socket_.get_executor(), [this, self, result]() {
            send_response(result);
        });
    }

    void send_response(const std::string& response) {
        auto self(shared_from_this());
        auto resp_ptr = std::make_shared<std::string>(response);
        boost::asio::async_write(socket_, boost::asio::buffer(*resp_ptr),
            [this, self, resp_ptr](boost::system::error_code ec, std::size_t) {
                if (!ec) {
                    // В этой реализации закрываем соединение после одного ответа
                    socket_.close();
                }
            });
    }

    tcp::socket socket_;
    boost::asio::thread_pool& pool_;
    boost::asio::steady_timer timer_;
    enum { max_length = 1024 };
    char data_[max_length];
};

class AsyncServer {
public:
    AsyncServer(boost::asio::io_context& io_context, short port, boost::asio::thread_pool& pool)
        : acceptor_(io_context, tcp::endpoint(tcp::v4(), port)), pool_(pool) {
        do_accept();
    }

private:
    void do_accept() {
        acceptor_.async_accept([this](boost::system::error_code ec, tcp::socket socket) {
            if (!ec) {
                std::make_shared<Session>(std::move(socket), pool_)->start();
            }
            do_accept();
        });
    }
    tcp::acceptor acceptor_;
    boost::asio::thread_pool& pool_;
};

int main() {
    try {
        boost::asio::io_context io_context;
        boost::asio::thread_pool pool(4);
        AsyncServer server(io_context, 12345, pool);
        std::cout << "Сервер запущен (Таймер + Факториал)..." << std::endl;
        io_context.run();
    } catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }
    return 0;
}