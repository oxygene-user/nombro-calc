#pragma once

extern "C" { extern unsigned int g_cpu_caps; extern int g_cpu_cores; }

namespace cpu
{
    bool INLINE caps(u32 mask) { return 0 != (g_cpu_caps & mask); }
}


