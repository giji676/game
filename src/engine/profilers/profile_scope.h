#pragma once

#include <chrono>

class ProfileScope {
public:
    ProfileScope(const char* name);
    ~ProfileScope();

private:
    const char* name_;
    std::chrono::high_resolution_clock::time_point start_;
};

#define PROFILE_SCOPE(name) \
    ProfileScope profileScope##__LINE__(name)
