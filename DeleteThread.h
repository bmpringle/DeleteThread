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
        *queue->front().first = false;
        queue->pop();
        deleteFunction(objectToDelete);

        terminate = *shouldTerminate;

        accessMutex->unlock();  
    }

    /*
    safe to delete accessmutex here because if shouldTerminate is ever true and the mutex is unlocked, then the DeleteThread object is in the dtor after its last use of accessMutex, and so can be
    deleted at the end of this function.
    */
    delete accessMutex;
}

template<class T>
class DeleteThread {
    public:
        DeleteThread() {

        }

        DeleteThread(std::function<void(T)> deleteFunction) :  validInstance(true), threadShouldTerminate(false), deleteFunctionStore(deleteFunction) {
            accessMutex = new std::mutex();
            deleteThread = new std::thread(deleteThreadFunction<T>, accessMutex, &destructionQueue, deleteFunction, &threadShouldTerminate);
        }

        DeleteThread(const DeleteThread& delThread) {
            validInstance = delThread.validInstance;

            threadShouldTerminate = false;

            if(validInstance) {
                accessMutex = new std::mutex();

                destructionQueue = delThread.destructionQueue;

                deleteFunctionStore = delThread.deleteFunctionStore;

                deleteThread = new std::thread(deleteThreadFunction<T>, accessMutex, &destructionQueue, deleteFunctionStore, &threadShouldTerminate);
            }
        }

        DeleteThread& operator=(const DeleteThread& delThread) {
            validInstance = delThread.validInstance;

            threadShouldTerminate = false;

            if(validInstance) {
                accessMutex = new std::mutex();

                destructionQueue = delThread.destructionQueue;

                deleteFunctionStore = delThread.deleteFunctionStore;

                deleteThread = new std::thread(deleteThreadFunction<T>, accessMutex, &destructionQueue, deleteFunctionStore, &threadShouldTerminate);
            }

            return *this;
        }

        void addObjectToDelete(T obj, bool* condition) {
            if(!validInstance) {
                return;
            }

            accessMutex->lock();
            destructionQueue.push(std::pair<bool*, T>(condition, obj));
            accessMutex->unlock();
        }

        ~DeleteThread() {
            if(!validInstance) {
                return;
            }

            accessMutex->lock();
            threadShouldTerminate = true;
            accessMutex->unlock();

            deleteThread->detach();

            delete deleteThread;
        }

        std::mutex* getMutexPointer() {
            return accessMutex;
        }

    private:
        bool validInstance = false;

        bool threadShouldTerminate;

        //use normal pointers instead of shared pointers because shared pointers aren't thread safe by default (I think).
        std::thread* deleteThread;
        std::mutex* accessMutex;

        std::queue<std::pair<bool*, T> > destructionQueue;

        std::function<void(T)> deleteFunctionStore;
};

#endif