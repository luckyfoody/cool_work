#ifndef GLIST_H
#define GLIST_H

#include <iostream>

// COOL 实验中常用的通用列表模版
template <class T>
class GList {
public:
    T *data;
    GList<T> *next;

    GList(T *d, GList<T> *n) : data(d), next(n) {}
};

// 如果 cgen.h 里用到了具体的 List 类型，可以根据报错在这里补齐
// 例如：typedef GList<CgenNode> CgenNodeList;

#endif
