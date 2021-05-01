#pragma once
#include <JuceHeader.h>

struct ReleasePool :
    public juce::Timer
{
    ReleasePool() :
        pool(),
        mutex()
    {}

    template <typename T>
    void add(std::shared_ptr<T>& object) {
        if (!object) return;
        const juce::ScopedLock lock(mutex);
        pool.emplace_back(object);
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
private:
    std::vector<std::shared_ptr<void>> pool;
    juce::CriticalSection mutex;
};

template<class Obj>
struct ThreadSafeObject {
    ThreadSafeObject(ReleasePool& p, const std::shared_ptr<Obj>&& ob) :
        obj(ob),
        pool(p)
    { pool.add(obj); }
    std::shared_ptr<Obj> load() { return std::atomic_load(&obj); }
    std::shared_ptr<Obj> copy() { return std::make_shared<Obj>(Obj(load().get())); }
    void replaceWith(std::shared_ptr<Obj>& newObj) {
        pool.add(newObj);
        std::atomic_store(&obj, newObj);
    }
protected:
    std::shared_ptr<Obj> obj;
    ReleasePool& pool;
};