#pragma once

#ifndef NDEBUG
    inline constexpr bool IS_DEBUG = true;
    inline constexpr bool IS_RELEASE = false;
    #define LOG(x) std::cout << x << "\n";
#else
    inline constexpr bool IS_DEBUG = false;
    inline constexpr bool IS_RELEASE = true;
#endif

#ifdef DEMO_BUILD
    inline constexpr bool IS_DEMO = true;
#else
    inline constexpr bool IS_DEMO = false;
#endif
