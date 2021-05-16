/*
* if you came here because of my youtube videos saying that this is thread-safe
* pls beware that i was proven wrong already.
* almost everything about this is pretty cool, but atomically loading the modulation-
* matrix in processBlock is not lock-free, which is bad obviously.
* i'll have to reimplement this with a different thread-safety-method soon.
* until then feel free to have a look at everything but don't take it
* too seriously yet.
*/

#pragma once
#include <JuceHeader.h>
/*
* a pointer to an arbitrary underlying object
*/
struct Anything {
    template<typename T>
    static Anything make(T&& args) { Anything anyNewThing; anyNewThing.set<T>(std::forward<T>(args)); return anyNewThing; }
    template<typename T>
    void set(T&& args) { ptr = new T(args); }
    template<typename T>
    void reset() noexcept { delete get<T>(); }
    template<typename T>
    const T* get() const noexcept { return static_cast<T*>(ptr); }
    void* ptr;

    /* to do:
    * check out std::any, might not need this
    */
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
    std::vector<std::shared_ptr<void>> pool;
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

/* to do:
* rewrite all this so that it uses juce::AbstractFifo and see if it works better
*/