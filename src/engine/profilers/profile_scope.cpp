#include <chrono>
#include "profile_scope.h"
#include "profiler.h"

ProfileScope::ProfileScope(const char* name)
    : name_(name),
    start_(std::chrono::high_resolution_clock::now())
{
}

ProfileScope::~ProfileScope() {
    auto end = std::chrono::high_resolution_clock::now();

    double ms =
        std::chrono::duration<double, std::milli>(
                end - start_
                ).count();

    Profiler::instance().addSample(name_, ms);
}
