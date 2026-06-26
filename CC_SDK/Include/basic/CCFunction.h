#ifndef CCFUNCTION_H
#define CCFUNCTION_H
#include <functional>

namespace CTL {
    template<typename T>
    class Function:public std::function<T> {
    public:
        using std::function<T>::function;
    };
}

#endif
