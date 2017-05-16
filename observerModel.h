#pragma once
template <class T>
class Observer
{
public:
    virtual void Update(T&) = 0;
};
template <class T>
class Subject
{
public:
    virtual void Attach(Observer<T>*) = 0;
    virtual void Detach(Observer<T>*) = 0;
    virtual void Notify() = 0;
};
