#pragma once

#include <cstdint>
#include <vector>

template <typename T>
struct ComponentPool {
    std::vector<T> values;

    void ensure(uint32_t idx) {
        const size_t need = static_cast<size_t>(idx) + 1;
        if (values.size() < need)
            values.resize(need);
    }

    T& at(uint32_t idx) { return values[idx]; }

    const T& at(uint32_t idx) const { return values[idx]; }

    void reset(uint32_t idx, const T& value = T{}) {
        ensure(idx);
        values[idx] = value;
    }
};
