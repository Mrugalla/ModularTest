#pragma once
#include <JuceHeader.h>

/*
* reference counted pointer to an arbitrary underlying object
*/
struct Anything {
    template<typename T>
    Anything(const std::shared_ptr<T>& d) :
        data(d)
    {}
    Anything() :
        data(nullptr)
    {}
    template<typename T>
    static std::shared_ptr<T> make(const T&& args) { return std::make_shared<T>(args); }
    template<typename T>
    void add(const T&& args) { data = make<T>(args); }
    void clear() noexcept { data.reset(); }
    template<typename T>
    const T* get() const noexcept { return static_cast<T*>(data.get()); }
    const long use_count() const noexcept { return data.use_count(); }
    std::shared_ptr<void> data;
};

/*
* releasePool for any object
*/
struct ReleasePool :
    public juce::Timer
{
    ReleasePool() :
        pool(),
        mutex()
    {}
    template<typename T>
    void add(const std::shared_ptr<T>& ptr) {
        const juce::ScopedLock lock(mutex);
        if (ptr == nullptr) return;
        pool.emplace_back(ptr);
        if (!isTimerRunning())
            startTimer(10000);
    }
    void timerCallback() override {
        const juce::ScopedLock lock(mutex);
        pool.erase(
            std::remove_if(
                pool.begin(), pool.end(), [](auto& object) { return object.use_count() <= 1; }),
            pool.end());
    }
    void dbg() {
        DBG("SIZE: " << pool.size());
        for (const auto& p : pool)
            DBG("COUNT: " << p.use_count());
    }

    static ReleasePool theReleasePool;
private:
    std::vector<Anything> pool;
    juce::CriticalSection mutex;
};

/*
* reference counted pointer to object that uses a static release pool
* to ensure thread-safety
*/
template<class Obj>
struct ThreadSafeObject {
    ThreadSafeObject(const std::shared_ptr<Obj>&& ob) :
        obj(ob)
    { ReleasePool::theReleasePool.add(obj); }
    std::shared_ptr<Obj> load() { return std::atomic_load(&obj); }
    std::shared_ptr<Obj> copy() { return std::make_shared<Obj>(Obj(load().get())); }
    void replaceWith(std::shared_ptr<Obj>& newObj) {
        ReleasePool::theReleasePool.add(newObj);
        std::atomic_store(&obj, newObj);
    }
protected:
    std::shared_ptr<Obj> obj;
};