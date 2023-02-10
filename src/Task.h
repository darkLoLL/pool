#pragma once
#include <memory>
class task_unit
{
public:
    virtual ~task_unit() {};
    virtual void operator()(){};
};

template<class Func>
class task_unit_impl : public task_unit
{
public:
    task_unit_impl(Func in_func) : m_func(in_func) {}
    virtual ~task_unit_impl() {}
    virtual void operator() ()
    {
        m_func();
    }
private:
    Func m_func;
};

template<class T>
class parameter_packer
{
};

template<class T>
class parameter_packer<T&>
{
public:
    parameter_packer(T& value) : param(value) {}
    T& param;
};

template<class T>
class parameter_packer<T&&>
{
public:
    parameter_packer(T&& value) : param(std::move(value)) {}
    parameter_packer(const parameter_packer& other) : param(std::move(other.param)) {}
    T&& param;
};

template<class T>
inline T param_maker(T value)
{
    return value;
}

class Task
{
public:
    Task() : m_task_unit(nullptr){
    }

    Task(Task& other){
        m_task_unit.swap(other.m_task_unit);
    }

    Task(Task&& other) : m_task_unit(std::move(other.m_task_unit)) {}

    Task& operator = (Task& other){
        if (&other != this)
        {
            m_task_unit.swap(other.m_task_unit);
        }
        return *this;
    }

    Task& operator = (Task&& other){
        if (&other != this)
        {
            m_task_unit = std::move(other.m_task_unit);
        }
        return *this;
    }

    template<class Func, class ... Args>
    Task(Func func, Args&& ... args){
        reset_impl(func, parameter_packer< decltype(std::forward<Args>(args)) >
                (std::forward<Args>(args)) ...);
    }

    template<class Func>
    Task(Func func){
        m_task_unit.reset(new task_unit_impl<Func >(func));
    }

    template<class ReturnT, class ClassT, class ... ClassFuncArgs>
    Task(ReturnT(ClassT::* func)(ClassFuncArgs ...), ClassT * obj_ptr, ClassFuncArgs&& ... args)
    {
        reset_class_func_impl(func, obj_ptr, parameter_packer<decltype(std::forward<ClassFuncArgs>(args)) >
                (std::forward<ClassFuncArgs>(args)) ...);
    }

    void operator() (){
        if (m_task_unit)
        {
            (*m_task_unit)();
        }
    }

private:
    template<class Func, class ... Args>
    void reset_impl(Func func, Args ... args)
    {
        auto lambda_helper = [func, args ...]()->void
        {
            func((args.param)...);
        };

        m_task_unit.reset(new task_unit_impl<decltype(lambda_helper) >(lambda_helper));
    }

    template<class Func, class T, class ... Args>
    void reset_class_func_impl(Func func, T* obj_ptr, Args ... args)
    {
        auto lambda_helper = [func, obj_ptr, args ...]()->void
        {
            (obj_ptr->*func)((args.param) ...);
        };
        m_task_unit.reset(new task_unit_impl<decltype(lambda_helper) >(lambda_helper));
    }
    std::unique_ptr<task_unit> m_task_unit;
};