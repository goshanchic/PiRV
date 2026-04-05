#include <iostream>
#include <thread>
#include <vector>
#include <queue>
#include <semaphore>
#include <mutex>
#include <chrono>
#include <atomic>
#include <string>

using namespace std;

struct VideoFrame {
    int cameraId;
    int frameNumber;
    bool isImportant; 

    bool operator<(const VideoFrame& other) const {
        if (isImportant != other.isImportant) {
            return !isImportant; 
        }
        return frameNumber > other.frameNumber; // При равном приоритете - по порядку
    }
};
// Глобальные ресурсы
priority_queue<VideoFrame> buffer; // Очередь задач с приоритето
mutex buffer_mtx;                 
mutex output_mtx;                  
counting_semaphore<6> cameras_sem(6); // потоки камер
atomic<int> active_accelerators(3);   // кол-во рабочих ускорителей 
atomic<bool> running(true);

void accelerator_worker(int id) {
    while (running || !buffer.empty()) {
        VideoFrame frame;
        bool found = false;

        {
            lock_guard<mutex> lock(buffer_mtx);
            if (!buffer.empty()) {
                frame = buffer.top();
                buffer.pop();
                found = true;
            }
        }

        if (found) {
            {
                lock_guard<mutex> lock(output_mtx);
                cout << "[Ускоритель " << id << "] Обработка: Камера " << frame.cameraId
                    << ", Кадр " << frame.frameNumber
                    << (frame.isImportant ? " (ПРИОРИТЕТ)" : " (Обычный)") << endl;
            }
            // имитация времени обработки
            this_thread::sleep_for(chrono::milliseconds(frame.isImportant ? 200 : 500));
        }
        else {
            this_thread::sleep_for(chrono::milliseconds(100));
        }
    }
}


void camera_stream(int cameraId) {
    for (int i = 1; i <= 5; ++i) {
        cameras_sem.acquire(); 

        VideoFrame frame;
        frame.cameraId = cameraId;
        frame.frameNumber = i;
        frame.isImportant = (i % 2 == 0); // четные кадры 

        {
            lock_guard<mutex> lock(buffer_mtx);
            buffer.push(frame); // доба в приоритет очередь
        }

        {
            lock_guard<mutex> lock(output_mtx);
            cout << "Камера " << cameraId << " отправила кадр " << i << endl;
        }

        cameras_sem.release();
        this_thread::sleep_for(chrono::milliseconds(300));
    }
}

// Глобал мониторинг
void global_monitoring() {
    this_thread::sleep_for(chrono::seconds(2));
    {
        lock_guard<mutex> lock(output_mtx);
        cout << "\n!!! МОНИТОРИНГ: Обнаружен отказ Ускорителя №3 !!!" << endl;
        active_accelerators--; // Перераспределение нагрузки за счет уменьшения потоков 
        cout << "!!! Задачи перераспределены на оставшиеся " << active_accelerators << " ускорителя.\n" << endl;
    }
}

int main() {
    setlocale(LC_ALL, "Russian");

    // запуск ускорителей ( 3)
    vector<thread> accels;
    for (int i = 1; i <= 3; ++i) {
        accels.push_back(thread(accelerator_worker, i));
    }

    // запуск потоков камер (6 потоков) 
    vector<thread> cams;
    for (int i = 1; i <= 6; ++i) {
        cams.push_back(thread(camera_stream, i));
    }

    // запуск мониторинга
    thread monitor(global_monitoring);

    //завершения работы камер
    for (auto& t : cams) t.join();

    this_thread::sleep_for(chrono::seconds(3)); // время на доработку хвостов очереди
    running = false;

    for (auto& t : accels) t.join();
    monitor.join();

    cout << "Все видеоданные обработаны." << endl;
    return 0;
}