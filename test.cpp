#include <map>
#include <string>
#include <chrono>
#include <iostream>

#include "DeleteThread.h"

std::map<int, std::string> intToStringMap = std::map<int, std::string>();

//this is needed becuase i'm going across threads to delete something in the templated function. if your delete function is thread safe, this isn't needed
std::mutex mapMutex;

void deleteFunc(int keyToDelete) {
    mapMutex.lock();
    if(intToStringMap.count(keyToDelete) > 0) {
        intToStringMap.erase(keyToDelete);
    }else {
        throw std::runtime_error(std::to_string(keyToDelete) + " isn't a key in the map, and as such cannot be deleted!");
    }
    mapMutex.unlock();
    
    std::cout << "object " << keyToDelete << " has been deleted from the map!" << std::endl;
}

int main() {
    DeleteThread<int> dThread1 = DeleteThread<int>(deleteFunc);

    DeleteThread<int> dThread = dThread1;

    intToStringMap[1] = "hi!";
    intToStringMap[2] = "this is another hi!";
    intToStringMap[69] = "funny number";

    bool countdown1 = false;
    bool countdown2 = false;
    bool countdown69 = false;

    dThread.addObjectToDelete(1, &countdown1);
    dThread.addObjectToDelete(2, &countdown2);
    dThread.addObjectToDelete(69, &countdown69);

    std::mutex* accessMutex = dThread.getMutexPointer();

    bool shouldLoop = true;

    auto startTimer = std::chrono::high_resolution_clock::now();

    int lastCountdownForCD1 = 5;
    int lastCountdownForCD2 = 10;
    int lastCountdownForCD69 = 15;

    std::cout << "map: {" << std::endl;
    for(std::pair<int, std::string> pair : intToStringMap) {
        std::cout << "key: " << pair.first << ", object: " << pair.second << std::endl;
    }
    std::cout <<  "}" << std::endl;

    while(shouldLoop) {
        int duration = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - startTimer).count();

        if(duration > 5000) {
            accessMutex->lock();
            countdown1 = true;
            accessMutex->unlock();
        }else {
            if(5 - (duration/1000) < lastCountdownForCD1) {
                std::cout << lastCountdownForCD1 << " until object 1 is deleted from the map!" << std::endl;
                --lastCountdownForCD1;
            }
            
        }

        if(duration > 10000) {
            accessMutex->lock();
            countdown2 = true;
            accessMutex->unlock();
        }else {
            if(10 - (duration/1000) < lastCountdownForCD2) {
                std::cout << lastCountdownForCD2 << " until object 2 is deleted from the map!" << std::endl;
                --lastCountdownForCD2;
            }
        }

        if(duration > 15000) {
            accessMutex->lock();
            countdown69 = true;
            accessMutex->unlock();
        }else {
            if(15 - (duration/1000) < lastCountdownForCD69) {
                std::cout << lastCountdownForCD69 << " until object 69 is deleted from the map!" << std::endl;
                --lastCountdownForCD69;
            }
        }

        accessMutex->lock();
        shouldLoop = !(countdown1 && countdown2 && countdown69);
        accessMutex->unlock();
    }

    //give it a little more time than its timeout to realize it should delete the last object
    std::this_thread::sleep_for(std::chrono::milliseconds(15));

    std::cout << "map: {" << std::endl;
    for(std::pair<int, std::string> pair : intToStringMap) {
        std::cout << "key: " << pair.first << ", object: " << pair.second << std::endl;
    }
    std::cout <<  "}" << std::endl;

    return 0;
}