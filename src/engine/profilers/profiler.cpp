#include <algorithm>
#include <iostream>
#include <iomanip>
#include <iostream>
#include <vector>
#include "profiler.h"

Profiler& Profiler::instance() {
    static Profiler p;
    return p;
}

void Profiler::addSample(const char* name, double ms) {
    auto& sample = samples_[name];

    sample.currentMs += ms;
    sample.calls++;
}

void Profiler::beginFrame() {
    for (auto& [_, sample] : samples_) {
        sample.currentMs = 0.0;
        sample.calls = 0;
    }
}

void Profiler::endFrame() {
    constexpr double alpha = 0.01;

    for (auto& [_, sample] : samples_) {
        sample.maxMs = std::max(sample.maxMs, sample.currentMs);

        if (sample.averageMs == 0.0)
            sample.averageMs = sample.currentMs;
        else
            sample.averageMs =
                sample.averageMs * (1.0 - alpha) +
                sample.currentMs * alpha;
    }
    // print();
}

void Profiler::print() {
    std::vector<const std::pair<const std::string, ProfileSample>*> sorted;
    sorted.reserve(samples_.size());

    for (const auto& entry : samples_)
        sorted.push_back(&entry);

    std::sort(sorted.begin(), sorted.end(),
            [](const auto* a, const auto* b) {
            return a->second.averageMs > b->second.averageMs;
            });

    std::cout << "\x1B[2J\x1B[H";
    std::cout
        << "+----------------------------------+----------+----------+----------+--------+\n"
        << "| Scope                            | Current  | Average  | Max      | Calls  |\n"
        << "+----------------------------------+----------+----------+----------+--------+\n";

    for (const auto* entry : sorted) {
        const auto& [name, sample] = *entry;
        std::cout
            << "| "
            << std::left << std::setw(32) << name
            << " | "
            << std::right << std::setw(8) << std::fixed
            << std::setprecision(3) << sample.currentMs
            << " | "
            << std::setw(8) << sample.averageMs
            << " | "
            << std::setw(8) << sample.maxMs
            << " | "
            << std::setw(6) << sample.calls
            << " |\n";
    }

    std::cout
        << "+-----------------------------------+----------+----------+----------+--------+\n";
    std::cout << std::flush;
}
