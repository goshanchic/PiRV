#include "server.hpp"
#include <vector>
#include <thread>

int main() {
    try {
        boost::asio::io_context io_context;
        MultiThreadedServer server(io_context, 12345);

        std::cout << "Многопоточный сервер запущен на порту 12345..." << std::endl;

        std::vector<std::thread> threads;
        // Запускаем 4 потока для обработки событий
        for (int i = 0; i < 4; ++i) {
            threads.emplace_back([&io_context]() {
                io_context.run();
            });
        }

        for (auto& t : threads) {
            t.join();
        }
    } catch (std::exception& e) {
        std::cerr << "Ошибка: " << e.what() << std::endl;
    }
    return 0;
}