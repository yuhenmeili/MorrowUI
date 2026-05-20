//
// Created by 0060328 on 25-9-23.
//

#ifndef SINGLETON_H
#define SINGLETON_H
#include <utility>
namespace morrow {
template <typename T, typename... Args>
class Singleton {
public:
    Singleton(const Singleton&) = delete;
    Singleton& operator=(const Singleton&) = delete;
    Singleton(Singleton&&) = delete;
    Singleton& operator=(Singleton&&) = delete;

    static T& getInstance(Args&&... args) {
        static T instance(std::forward<Args>(args)...);
        return instance;
    }

protected:
    Singleton() = default;
    ~Singleton() = default;
};
}
#endif //SINGLETON_H
