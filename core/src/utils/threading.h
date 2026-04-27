#pragma once

#include <thread>

namespace utils {

#if defined(_WIN32)
#define NOMINMAX
#include <windows.h>
#include <processthreadsapi.h>
#include <string>

inline std::wstring utils_utf8ToWide(const char* utf8) {
    if (!utf8) {
        return std::wstring();
    }
    int size = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, nullptr, 0);
    if (size <= 0) {
        return std::wstring();
    }
    std::wstring result(size, L"\0");
    MultiByteToWideChar(CP_UTF8, 0, utf8, -1, &result[0], size);
    if (!result.empty() && result.back() == L'\0') {
        result.pop_back();
    }
    return result;
}

inline void setCurrentThreadName(const char* name) {
    auto wname = utils_utf8ToWide(name);
    if (!wname.empty()) {
        SetThreadDescription(GetCurrentThread(), wname.c_str());
    }
}

inline void setThreadName(std::thread& thread, const char* name) {
    auto wname = utils_utf8ToWide(name);
    if (!wname.empty()) {
        SetThreadDescription(static_cast<HANDLE>(thread.native_handle()), wname.c_str());
    }
}

#elif defined(__APPLE__)
#include <pthread.h>

inline void setCurrentThreadName(const char* name) {
    pthread_setname_np(name);
}

inline void setThreadName(std::thread& /*thread*/, const char* /*name*/) {
    // macOS does not support setting another thread's name via pthread_setname_np.
}

#else
#include <pthread.h>

inline void setCurrentThreadName(const char* name) {
    pthread_setname_np(pthread_self(), name);
}

inline void setThreadName(std::thread& thread, const char* name) {
    pthread_setname_np(thread.native_handle(), name);
}

#endif

} // namespace utils
