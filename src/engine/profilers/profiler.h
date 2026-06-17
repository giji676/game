#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>

struct ProfileSample {
    double currentMs = 0.0;
    double averageMs = 0.0;
    double maxMs = 0.0;
    uint32_t calls = 0;
};

class Profiler {
public:
    static Profiler& instance();

    void addSample(const char* name, double ms);

    void beginFrame();
    void endFrame();

    const std::unordered_map<std::string, ProfileSample>& samples() const {
        return samples_;
    }

private:
    std::unordered_map<std::string, ProfileSample> samples_;
    void print();
};
