#ifndef __SERVER_SINGLETON_HPP__
#define __SERVER_SINGLETON_HPP__

namespace Server {

template<typename T, typename X = void, int N = 0>
class Singleton {
public:
    static T* getInstance(){
        static T v;
        return &v;
    }
private:
    Singleton() = default;
    Singleton(Singleton& other) = delete;
    void operator=(const Singleton& other) = delete;
};

template<typename T, typename X = void, int N = 0>
class SingletonPtr {
public:
    static std::shared_ptr<T> getInstance() {
        static std::shared_ptr<T> v(new T);
        return v;
    }
private:
    SingletonPtr() = default;
    SingletonPtr(SingletonPtr& other) = delete;
    void operator=(const SingletonPtr& other) = delete;
};

}

#endif