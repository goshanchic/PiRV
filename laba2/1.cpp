#include <iostream>
#include <thread>
#include <vector>
#include <semaphore> 
#include <mutex>
#include <chrono>
#include <random>
#include <atomic>

using namespace std; 

// Ресурсы порта
counting_semaphore<10> cranes(5); // 5 погрузчиков
mutex output_mtx;                 // для синхронизации 
atomic<int> waiting_count(0);     // очередь счётчик грузовиков
atomic<int> loaded_count(0);      // Счетчик уже загруженных г
bool reserve_crane_deployed = false; // флаг активации  крана

void truck(int id) {
    // прибытие грузовика
    {
        lock_guard<mutex> lock(output_mtx);
        waiting_count++;
        cout << "[Порт] Грузовик #" << id << " прибыл. Очередь: " << waiting_count << endl;
    }

    
    //  в очереди более 5 грузовиков, запуск доп кран 
    if (waiting_count > 5 && !reserve_crane_deployed) {
        lock_guard<mutex> lock(output_mtx);
        if (!reserve_crane_deployed) {
            cranes.release(); // +1 кран
            reserve_crane_deployed = true;
            cout << ">>> ВНИМАНИЕ: Очередь велика. Запущен резервный кран!" << endl;
        }
    }

    // Ожидание и захват крана
    cranes.acquire(); // поток занимает кран 
    waiting_count--;  // выход из очереди ожидания

    // процесс погрузки
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> dis(3, 6); // 3-6 секунд время загрузки

    int duration = dis(gen);
    //  загружено менее 3 грузовиков, время сокращается 
    if (loaded_count < 3) {
        duration = 1; // мин возможное время для ускорения
        {
            lock_guard<mutex> lock(output_mtx);
            cout << "[!] АВАРИЙНЫЙ РЕЖИМ: Грузовик #" << id << " загружается ускоренно (1 сек)." << endl;
        }
    }
    else {
        {
            lock_guard<mutex> lock(output_mtx);
            cout << "Грузовик #" << id << " начал загрузку (план: " << duration << " сек)." << endl;
        }
    }

    
    this_thread::sleep_for(chrono::seconds(duration));

    {
        lock_guard<mutex> lock(output_mtx);
        loaded_count++;
        cout << "Грузовик #" << id << " загружен. Всего выехало: " << loaded_count << endl;
    }

    cranes.release(); // освобождение крана для следующего грузовика
}

int main() {
    setlocale(LC_ALL, "Russian");
    vector<thread> fleet;
    for (int i = 1; i <= 10; ++i) {
        fleet.push_back(thread(truck, i));
        // для наглядности
        this_thread::sleep_for(chrono::milliseconds(100));
    }
    for (auto& t : fleet) {
        t.join();
    }

    cout << "--- Работа порта завершена. Все 10 грузовиков обработаны. ---" << endl;

    return 0;
}