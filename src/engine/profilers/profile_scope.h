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

#define PROFILE_CONCAT_IMPL(x, y) x##y
#define PROFILE_CONCAT(x, y) PROFILE_CONCAT_IMPL(x, y)

#define PROFILE_SCOPE(name) \
    ProfileScope PROFILE_CONCAT(profileScope, __LINE__)(name)
