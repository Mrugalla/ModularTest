#pragma once

template<typename T>
struct Vec2D {
    Vec2D() :
        data()
    {}
    const bool empty() const noexcept { return data.empty(); }
    void clear() noexcept { data.clear(); }
    void resize(const int sizeA, const int sizeB) {
        clear();
        data.resize(sizeA);
        for (auto& d : data) d.resize(sizeB);
    }
    T& operator()(const int i0, const int i1) noexcept { return data[i0][i1]; }
    const T& operator()(const int i0, const int i1) const noexcept { return data[i0][i1]; }
    std::vector<std::vector<T>> data;
};