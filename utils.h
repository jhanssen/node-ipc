#ifndef UTILS_H
#define UTILS_H

#include <nan.h>
#include <assert.h>
#include <queue>
#include <errno.h>
#include <string>
#include <vector>

#define EINTRWRAP(var, op)                      \
    do {                                        \
        var = op;                               \
    } while (var == -1 && errno == EINTR);

class Condition;

class Mutex
{
public:
    Mutex()
    {
        uv_mutex_init(&mMutex);
    }
    ~Mutex()
    {
        uv_mutex_destroy(&mMutex);
    }

    void lock()
    {
        uv_mutex_lock(&mMutex);
    }
    void unlock()
    {
        uv_mutex_unlock(&mMutex);
    }

private:
    uv_mutex_t mMutex;

    friend class Condition;
};

class MutexLocker
{
public:
    MutexLocker(Mutex* m)
        : mMutex(m)
    {
        mMutex->lock();
    }

    ~MutexLocker()
    {
        mMutex->unlock();
    }

private:
    Mutex* mMutex;
};

class Condition
{
public:
    Condition()
    {
        uv_cond_init(&mCond);
    }

    ~Condition()
    {
        uv_cond_destroy(&mCond);
    }

    void wait(Mutex* mutex)
    {
        uv_cond_wait(&mCond, &mutex->mMutex);
    }

    void waitUntil(Mutex* mutex, uint64_t timeout)
    {
        uv_cond_timedwait(&mCond, &mutex->mMutex, timeout);
    }

    void signal()
    {
        uv_cond_signal(&mCond);
    }

    void broadcast()
    {
        uv_cond_broadcast(&mCond);
    }

private:
    uv_cond_t mCond;
};

template<typename T>
class Queue
{
public:
    Queue()
    {
    }

    ~Queue()
    {
    }

    void push(T&& t)
    {
        MutexLocker locker(&mMutex);
        mContainer.push(std::forward<T>(t));
    }

    T pop(bool* ok = 0)
    {
        MutexLocker locker(&mMutex);
        if (!mContainer.empty()) {
            if (ok)
                *ok = true;
            const T t = std::move(mContainer.back());
            mContainer.pop();
            return t;
        } else {
            if (ok)
                *ok = false;
            return T();
        }
    }

private:
    Mutex mMutex;
    std::queue<T> mContainer;
};

// jesus christ, v8 people. how bad can you make an API?
inline v8::Handle<v8::Value> makeValue(const std::string& str)
{
    Nan::EscapableHandleScope scope;
    const auto maybe = v8::String::NewFromUtf8(v8::Isolate::GetCurrent(), str.c_str(), v8::NewStringType::kNormal);
    v8::Local<v8::String> v8str;
    if (!maybe.ToLocal(&v8str))
        return scope.Escape(v8::Handle<v8::Value>());
    return scope.Escape(v8str);
}

template<typename Return, typename Key, typename Container>
inline Return get(const Container& container, const Key& key)
{
    auto it = container.find(key);
    if (it == container.end())
        return Return();
    return it->second;
}

inline std::vector<std::string> split(const std::string& str, char delim)
{
    std::vector<std::string> ret;
    if (str.empty())
        return ret;
    const char* cur = str.c_str();
    const char* prev = cur;
    while (*cur != '\0') {
        if (*cur == delim) {
            // from prev to cur
            ret.push_back(std::string(prev, cur - prev));
            prev = cur + 1;
        }
        ++cur;
    }
    // final one
    ret.push_back(std::string(prev, cur - prev));
    return ret;
}

// helper for std string
namespace Nan {
inline void ThrowError(const std::string& err)
{
    Nan::ThrowError(err.c_str());
}
}

#endif
