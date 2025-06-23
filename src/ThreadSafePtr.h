#pragma once

#include <mutex>
#include <memory>


namespace multithread_safety
{
    template <typename Mutex = std::recursive_mutex>
    struct BaseContainer
    {
        Mutex mutex;
        virtual ~BaseContainer() = default;
    };
}


template <typename Type, typename Mutex = std::recursive_mutex>
class ThreadSafePtr
{
public:
    typedef std::unique_lock<Mutex> lock_t;
    typedef multithread_safety::BaseContainer<Mutex> base_container_t;
    typedef Type value_t;

    template <typename T, typename M> friend class ThreadSafePtr;

    template<typename Child, typename Base>
    friend ThreadSafePtr<Child> dynamicPointerCast(const ThreadSafePtr<Base>& ptr);
    
    template<typename Child, typename Base>
    friend class WeakThreadSafePtr;


    ThreadSafePtr() = default;

    template<typename ConvertibleType>
    ThreadSafePtr(ThreadSafePtr<ConvertibleType, Mutex>&& other) :
        _container(std::move(other._container)),
        _object(std::move(other._object))
    {}

    template<typename ConvertibleType>
    ThreadSafePtr operator=(const ThreadSafePtr<ConvertibleType, Mutex>&& other)
    {
        _container = std::move(other._container);
        _object = std::move(other._object);
        return *this;
    }

    struct DefaultConstructor {};

    ThreadSafePtr(DefaultConstructor) :
        _container(std::make_shared<Container>()),
        _object(&std::static_pointer_cast<Container>(_container)->obj)
    {}

    ThreadSafePtr(Type* obj) :
        _container(std::make_shared<ForeignContainer>(obj)),
        _object(obj)
    {}

    ThreadSafePtr(const std::shared_ptr<Type>& obj) :
        _container(obj, new ForeignContainer(obj.get())),
        _object(obj.get())
    {}

    template<typename... Args>
    ThreadSafePtr(Args... args) :
        _container(std::make_shared<Container>(std::forward<Args>(args)...)),
        _object(&std::static_pointer_cast<Container>(_container)->obj)
    {}

    template<typename ConvertibleType>
    ThreadSafePtr(const ThreadSafePtr<ConvertibleType, Mutex>& other) :
        _container(other._container),
        _object(other._object)
    {}

    template<typename ConvertibleType>
    ThreadSafePtr operator=(const ThreadSafePtr<ConvertibleType, Mutex>& other)
    {
        _container = other._container;
        _object = other._object;
        return *this;
    }

    ThreadSafePtr(const std::shared_ptr<multithread_safety::BaseContainer<Mutex>>& container, Type* obj) :
        _container(container),
        _object(obj)
    {}

    class Proxy
    {
    public:
        Proxy(Type* obj, Mutex& mtx) : _object(obj), _lock(mtx)
        {}

        Proxy(Proxy&& o) : _object(o._object), _lock(std::move(o._lock))
        {}

        Type* operator->()
        {
            return _object;
        }

        const Type* operator->() const
        {
            return _object;
        }

        Type& operator*()
        {
            return *_object;
        }

        const Type& operator*() const
        {
            return *_object;
        }

        void unlock()
        {
            _lock.unlock();
        }

        void lock()
        {
            _lock.lock();
        }

    private:
        Proxy(const Proxy&) = delete;
        const Proxy& operator=(const Proxy&) = delete;

    private:
        Type* _object = nullptr;
        lock_t _lock;
    };

    void lock() const { _container->mutex.lock(); }
    void unlock() const { _container->mutex.unlock(); }

    Proxy getLocker() const { return Proxy(_object, _container->mutex); }

    Proxy operator->() const { return Proxy(_object, _container->mutex); }

    Type* get() const { return _object; }

    void reset(Type* obj = nullptr)
    {
        _container = std::make_shared<ForeignContainer>(obj);
        _object = obj;
    }

    explicit operator bool() const
    {
        return _object;
    }

    Mutex& getMutex() { return _container->mutex; }

private:
    struct Container : public base_container_t
    {
    public:
        template<typename... Args>
        Container(Args... args) : obj(std::forward<Args>(args)...)
        {}

        Type obj;

    private:
        Container(const Container&) = delete;
        Container& operator=(const Container&) = delete;
    };

    struct ForeignContainer : public base_container_t
    {
    public:
        ForeignContainer(Type* ptr) : obj(ptr)
        {}

        Type* obj = nullptr;

    private:
        ForeignContainer(const ForeignContainer&) = delete;
        ForeignContainer& operator=(const ForeignContainer&) = delete;
    };

    std::shared_ptr<base_container_t> _container;
    Type* _object = nullptr;
};


template <typename Type, typename Mutex = std::recursive_mutex>
class WeakThreadSafePtr
{
public:
    WeakThreadSafePtr() = default;

    WeakThreadSafePtr(const ThreadSafePtr<Type, Mutex>& ptr) :
        _container(ptr._container),
        _object(ptr._object)
    {}

    bool expired() const noexcept
    {
        return _container.expired();
    }

    ThreadSafePtr<Type, Mutex> lock()
    {
        auto ptr = _container.lock();
        if (ptr)
            return ThreadSafePtr<Type, Mutex>(std::move(ptr), _object);
        return ThreadSafePtr<Type, Mutex>();
    }

private:
    std::weak_ptr<multithread_safety::BaseContainer<Mutex>> _container;
    Type* _object = nullptr;
};


template<typename Child, typename Base, typename Mutex>
ThreadSafePtr<Child> dynamicPointerCast(const ThreadSafePtr<Base, Mutex>& ptr)
{
    static_assert(std::is_base_of<Base, Child>::value);

    Child* obj = dynamic_cast<Child*>(ptr._object);

    return ThreadSafePtr<Child>(ptr._container, obj);
}
