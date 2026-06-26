#ifndef CCArray_H
#define CCArray_H
#pragma once

#include "array"

namespace CTL {
    template<typename T,std::size_t Nm>
    class Array:public std::array<T,Nm>{
    public:
        using std::array<T,Nm>::array;

    };
}



#endif
