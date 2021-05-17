#pragma once
#include <JuceHeader.h>
#include <any>
/*
* a pointer to an arbitrary underlying object (deprecated)
*/
struct Anything {
    Anything() : ptr(nullptr) {}
    ~Anything() {}
    template<typename T>
    static Anything make(T&& args) { Anything anyNewThing; anyNewThing.set<T>(std::forward<T>(args)); return anyNewThing; }
    template<typename T>
    void set(T&& args) { if(ptr == nullptr) ptr = new T(args); }
    template<typename T>
    void reset() noexcept { delete get<T>(); ptr = nullptr; }
    template<typename T>
    const T* get() const noexcept { return static_cast<T*>(ptr); }
    void* ptr;
};

/*
* pointers to arbitrary underlying objects
*/
struct VectorAnything {
    VectorAnything() :
        data()
    {}
    template<typename T>
    void add(const T&& args) { data.push_back(std::make_shared<T>(args)); }
    template<typename T>
    T* get(const int idx) const noexcept {
        auto newShredPtr = data[idx].get();
        return static_cast<T*>(newShredPtr);
    }
    const size_t size() const noexcept { return data.size(); }
protected:
    std::vector<std::shared_ptr<void>> data;
    /*
    * try to rewrite this with different smart pointers and/or std::any some time
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
template<class Type>
struct ThreadSafeObject {
    ThreadSafeObject(const std::shared_ptr<Type>&& ob) :
        curPtr(ob),
        updatedPtr(ob),
        spinLock()
    {
        ReleasePool::theReleasePool.add(curPtr);
    }
    std::shared_ptr<Type> loadCurrentPtr() noexcept { // Message Thread (if just changing an atomic)
        return std::atomic_load(&curPtr);
    }
    std::shared_ptr<Type> updateAndLoadCurrentPtr() noexcept {  // Audio Thread
        const juce::SpinLock::ScopedLockType lock(spinLock);
        if (spinLock.tryEnter()) {
            spinLock.enter();
            curPtr = updatedPtr;
            spinLock.exit();
        }
        return curPtr;
    }
    std::shared_ptr<Type> copy() const { // Message Thread (for adding modulators and stuff)
        auto loadedPtr = std::atomic_load(&curPtr);
        return std::make_shared<Type>(Type(loadedPtr.get()));
    }
    void replaceUpdatedPtrWith(std::shared_ptr<Type>& newPtr) { // Message Thread (for adding modulators and stuff)
        ReleasePool::theReleasePool.add(newPtr);
        std::atomic_store(&updatedPtr, newPtr);
    }
    void align() { // at the end of PrepareToPlay
        std::atomic_store(&updatedPtr, curPtr);
    }
    Type* operator->() noexcept { return curPtr.get(); }
protected:
    std::shared_ptr<Type> curPtr;
    std::shared_ptr<Type> updatedPtr;
    juce::SpinLock spinLock;
};

/* to do:
* experimental rewrite that uses juce::AbstractFifo
*   try what works better
*/