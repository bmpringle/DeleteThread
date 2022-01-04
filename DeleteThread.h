#ifndef DELETETHREAD_H
#define DELETETHREAD_H

#include <thread>
#include <mutex>
#include <queue>
#include <functional>

template<class T>
void deleteThreadFunction(std::mutex* accessMutex, std::queue<std::pair<bool*, T> >* queue, std::function<void(T)> deleteFunction, bool* shouldTerminate, bool* forceDeleteAndTerminate) {
    accessMutex->lock();
    bool terminate = *shouldTerminate;
    accessMutex->unlock();

    while(!terminate) {
        bool shouldDestroy = false;

        while(!shouldDestroy && !terminate) {
            accessMutex->lock();
            bool isEmpty = queue->size() == 0;

            if(!isEmpty) {
                std::pair<bool*, T> front = queue->front();
                if(front.first == nullptr) {
                    std::runtime_error("queue is null");
                }
                if(*front.first) {
                    shouldDestroy = true;
                }
            }

            terminate = *shouldTerminate || *forceDeleteAndTerminate;

            accessMutex->unlock();

            if(!shouldDestroy) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        }

        if(shouldDestroy) {
            accessMutex->lock();
            T objectToDelete = queue->front().second;
            *queue->front().first = false;
            queue->pop();
            deleteFunction(objectToDelete);
        }

        terminate = *shouldTerminate || *forceDeleteAndTerminate;

        if(forceDeleteAndTerminate) {
            while(!queue->empty()) {
                T objectToDelete = queue->front().second;
                *queue->front().first = false;
                queue->pop();
                deleteFunction(objectToDelete);
            }
        }

        accessMutex->unlock();
    }

    /*
    safe to delete accessmutex here because if shouldTerminate is ever true and the mutex is unlocked, then the DeleteThread object is in the dtor after its last use of accessMutex, and so can be
    deleted at the end of this function.
    */
    delete accessMutex;

    delete shouldTerminate;
    
    delete forceDeleteAndTerminate;
}

template<class T>
class DeleteThread {
    public:
        DeleteThread() {

        }

        DeleteThread(std::function<void(T)> deleteFunction) :  validInstance(true), deleteFunctionStore(deleteFunction) {
            accessMutex = new std::mutex();
            deleteThread = new std::thread(deleteThreadFunction<T>, accessMutex, &destructionQueue, deleteFunction, threadShouldTerminate, forceDeleteAndTerminate);
        }

        /*DeleteThread(const DeleteThread& delThread) {
            std::cout << "DT&" << std::endl;
            if(validInstance) {
                accessMutex->lock();
                threadShouldTerminate = true;
                accessMutex->unlock();

                deleteThread->detach();

                delete deleteThread;
            }

            validInstance = delThread.validInstance;

            threadShouldTerminate = false;

            if(validInstance) {
                accessMutex = new std::mutex();

                destructionQueue = std::queue<std::pair<bool*, T> >();

                deleteFunctionStore = delThread.deleteFunctionStore;

                deleteThread = new std::thread(deleteThreadFunction<T>, accessMutex, &destructionQueue, deleteFunctionStore, &threadShouldTerminate);
            }
        }*/

        /*DeleteThread& operator=(const DeleteThread& delThread) {
            std::cout << "op=" << std::endl;
            if(validInstance) {
                accessMutex->lock();
                threadShouldTerminate = true;
                accessMutex->unlock();

                deleteThread->detach();

                delete deleteThread;
            }

            validInstance = delThread.validInstance;

            threadShouldTerminate = false;

            if(validInstance) {
                accessMutex = new std::mutex();

                destructionQueue = std::queue<std::pair<bool*, T> >();

                deleteFunctionStore = delThread.deleteFunctionStore;

                deleteThread = new std::thread(deleteThreadFunction<T>, accessMutex, &destructionQueue, deleteFunctionStore, &threadShouldTerminate);
            }

            return *this;
        }*/

        void addObjectToDelete(T obj, bool* condition) {
            if(!validInstance) {
                return;
            }

            accessMutex->lock();
            destructionQueue.push(std::pair<bool*, T>(condition, obj));
            accessMutex->unlock();
        }

        void forceJoin() {
            accessMutex->lock();
            *forceDeleteAndTerminate = true;
            accessMutex->unlock();

            deleteThread->join();

            validInstance = false; //this invalidates the instance

            delete deleteThread;
        };

        ~DeleteThread() {
            if(!validInstance) {
                return;
            }

            accessMutex->lock();
            *threadShouldTerminate = true;
            accessMutex->unlock();

            deleteThread->detach();

            delete deleteThread;
        }

        std::mutex* getMutexPointer() {
            return accessMutex;
        }

        bool isValidInstance() {
            return validInstance;
        }

    private:
        bool* forceDeleteAndTerminate = new bool(false);

        bool validInstance = false;

        bool* threadShouldTerminate = new bool(false);

        //use normal pointers instead of shared pointers because shared pointers aren't thread safe by default (I think).
        std::thread* deleteThread;
        std::mutex* accessMutex;

        std::queue<std::pair<bool*, T> > destructionQueue;

        std::function<void(T)> deleteFunctionStore;
};

#endif