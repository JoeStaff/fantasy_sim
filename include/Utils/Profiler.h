#pragma once

#include "Core/Types.h"
#include <string>
#include <unordered_map>
#include <chrono>
#include <vector>

namespace Utils {

// Simple profiler for performance measurement
class Profiler {
public:
    static Profiler& GetInstance();
    
    // Start profiling a section
    void StartSection(const std::string& name);
    
    // End profiling a section
    void EndSection(const std::string& name);
    
    // Get section time in milliseconds
    f32 GetSectionTime(const std::string& name) const;
    
    // Get all section times
    std::unordered_map<std::string, f32> GetAllSectionTimes() const;
    
    // Reset all timings
    void Reset();
    
    // Print profiling report
    void PrintReport() const;
    
private:
    Profiler() = default;
    ~Profiler() = default;
    Profiler(const Profiler&) = delete;
    Profiler& operator=(const Profiler&) = delete;
    
    struct SectionData {
        std::chrono::high_resolution_clock::time_point start_time;
        f32 total_time_ms = 0.0f;
        u32 call_count = 0;
    };
    
    std::unordered_map<std::string, SectionData> sections_;
};

// RAII profiler scope
class ProfilerScope {
public:
    ProfilerScope(const std::string& name);
    ~ProfilerScope();
    
private:
    std::string name_;
};

#ifdef ENABLE_PROFILING
#define PROFILE_SCOPE(name) ProfilerScope _prof_scope(name)
#define PROFILE_START(name) Profiler::GetInstance().StartSection(name)
#define PROFILE_END(name) Profiler::GetInstance().EndSection(name)
#else
#define PROFILE_SCOPE(name) ((void)0)
#define PROFILE_START(name) ((void)0)
#define PROFILE_END(name) ((void)0)
#endif

} // namespace Utils



