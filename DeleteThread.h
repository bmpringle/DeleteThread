#ifndef DELETETHREAD_H
#define DELETETHREAD_H

#include <thread>
#include <mutex>
#include <queue>
#include <functional>

template<class T>
void deleteThreadFunction(std::mutex* accessMutex, std::queue<std::pair<bool*, T> >* queue, std::function<void(T)> deleteFunction, bool* shouldTerminate) {
    accessMutex->lock();
    bool terminate = *shouldTerminate;
    accessMutex->unlock();

    while(!terminate) {
        bool shouldDestroy = false;

        while(!shouldDestroy && !terminate) {
            accessMutex->lock();
            bool isEmpty = queue->empty();

            if(!isEmpty) {
                if(*queue->front().first) {
                    shouldDestroy = true;
                }
            }
            accessMutex->unlock();

            if(!shouldDestroy) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        }

        accessMutex->lock();
        T objectToDelete = queue->front().second;
        queue->pop();
        deleteFunction(objectToDelete);

        terminate = *shouldTerminate;

        accessMutex->unlock();  
    }
}

template<class T>
class DeleteThread {
    public:
        DeleteThread(std::function<void(T)> deleteFunction) : threadShouldTerminate(false) {
            deleteThread = std::thread(deleteThreadFunction<T>, &accessMutex, &destructionQueue, deleteFunction, &threadShouldTerminate);
        }

        void addObjectToDelete(T obj, bool* condition) {
            accessMutex.lock();
            destructionQueue.push(std::pair<bool*, T>(condition, obj));
            accessMutex.unlock();
        }

        ~DeleteThread() {
            accessMutex.lock();
            threadShouldTerminate = true;
            accessMutex.unlock();

            deleteThread.detach();
        }
    private:
        bool threadShouldTerminate;
        std::thread deleteThread;
        std::mutex accessMutex;

        std::queue<std::pair<bool*, T> > destructionQueue;
};

#endif