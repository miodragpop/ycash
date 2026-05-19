mingw32_CFLAGS=-pipe
# -nostdinc++: drop clang's default mingw-GCC libstdc++ include dirs so
# libc++'s `#include_next <math.h>` reaches the mingw-w64 C <math.h>
# (otherwise libstdc++'s <math.h> shadows it -> FP_NAN undeclared in boost).
# -Wno-unused-command-line-argument: default_host_CXX carries the link-only
# -stdlib=libc++; clang flags it unused on compile-only invocations. Silenced
# here so configure's -Werror probe passes and --enable-werror stays usable
# (otherwise the probe fails and AX_CHECK_COMPILE_FLAG misdetects clang as
# accepting GCC-only flags like -Wno-builtin-declaration-mismatch).
mingw32_CXXFLAGS=$(mingw32_CFLAGS) -nostdinc++ -isystem $(host_prefix)/include/c++/v1 -Wno-unused-command-line-argument

mingw32_LDFLAGS?=-fuse-ld=lld
mingw32_LDFLAGS+=-L/usr/lib/gcc/x86_64-w64-mingw32/$(shell x86_64-w64-mingw32-g++-posix -dumpversion)
# Put the staged libc++ (libcxx package) on the search path so the driver's
# own -stdlib=libc++ -lc++/-lc++abi resolve. Only a search path -- inert for
# C/non-C++ packages (no library is forced onto them).
mingw32_LDFLAGS+=-L$(host_prefix)/lib

# secp256k1.h emits __declspec(dllimport) on Windows unless SECP256K1_STATIC
# is defined; we link libsecp256k1 statically. configure.ac sets this in
# CORE_CPPFLAGS but that var is never AC_SUBST'd / used, so the define never
# reaches any TU -> LNK4217 import-vs-local spam. Define it host-wide here
# (Windows-only macro; harmless to non-secp TUs).
mingw32_CPPFLAGS=-DSECP256K1_STATIC

mingw32_release_CFLAGS=-O3
mingw32_release_CXXFLAGS=$(mingw32_release_CFLAGS)

mingw32_debug_CFLAGS=-O0
mingw32_debug_CXXFLAGS=$(mingw32_debug_CFLAGS)

mingw32_debug_CPPFLAGS=-D_GLIBCXX_DEBUG -D_GLIBCXX_DEBUG_PEDANTIC

mingw_cmake_system=Windows
