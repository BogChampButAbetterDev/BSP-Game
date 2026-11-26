#pragma once

#ifndef NDEBUG
    inline constexpr bool IS_DEBUG = true;
    inline constexpr bool IS_RELEASE = false;
#else
    inline constexpr bool IS_DEBUG = false;
    inline constexpr bool IS_RELEASE = true;
#endif

#ifdef DEMO_BUILD
    inline constexpr bool IS_DEMO = true;
#else
    inline constexpr bool IS_DEMO = false;
#endif
