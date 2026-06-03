# The Windows cross toolchain is llvm-mingw (see packages/native_llvm_mingw.mk):
# a self-consistent UCRT bundle of Clang, LLD, mingw-w64, libc++, libc++abi,
# libunwind, and winpthreads. Using it instead of the host's system mingw-w64
# means the whole tree shares one CRT (UCRT) and one libc++ ABI.
mingw32_native_toolchain=native_llvm_mingw
mingw32_native_binutils=native_llvm_mingw

# native_llvm_mingw symlinks its target-prefixed tools into $(build_prefix)/bin
# (the native toolchain bin directory), so we reference them by name without a
# path. That directory is on PATH during package builds and is also the prefix the
# generated config.site prepends to each tool name; an absolute path here would be
# doubled by that prefix and break configure. The clang/clang++ wrappers select the
# mingw target, the bundled UCRT sysroot, libc++, and LLD automatically, so no
# -target/--sysroot/-stdlib/-fuse-ld flags are needed.
mingw32_CC=x86_64-w64-mingw32-clang
mingw32_CXX=x86_64-w64-mingw32-clang++
mingw32_AR=x86_64-w64-mingw32-ar
mingw32_RANLIB=x86_64-w64-mingw32-ranlib
mingw32_NM=x86_64-w64-mingw32-nm
mingw32_STRIP=x86_64-w64-mingw32-strip

mingw32_CFLAGS=-pipe
mingw32_CXXFLAGS=$(mingw32_CFLAGS)

# _WIN32_WINNT is a build-time header-version selector, not a runtime OS floor.
# Set to 0x0602 (Win8), raised from upstream's 0x0601 (Win7) to match Boost 1.91.
# Boost.Thread's win32 backend (thread_primitives.cpp) includes Boost.Atomic,
# whose wait_ops_windows.hpp calls boost::winapi::WaitOnAddress unconditionally;
# that winapi wrapper is gated behind BOOST_WINAPI_VERSION_WIN8, so it is
# undeclared at 0x0601 and the Boost build fails. (Upstream avoids this by
# holding Boost at 1.88, which still had a Win7 runtime-dispatch fallback; we
# keep 1.91 and require Win8 instead.) Do NOT go to 0x0a00 (Win10): llvm-mingw
# then derives a Win11 NTDDI_VERSION that breaks the libevent iphlpapi.h build.
# Win8 is the minimum that satisfies Boost 1.91 without tripping that.
mingw32_CPPFLAGS=-D_WIN32_WINNT=0x0602

mingw32_release_CFLAGS=-O3
mingw32_release_CXXFLAGS=$(mingw32_release_CFLAGS)

mingw32_debug_CFLAGS=-O0
mingw32_debug_CXXFLAGS=$(mingw32_debug_CFLAGS)

mingw32_debug_CPPFLAGS=-D_GLIBCXX_DEBUG -D_GLIBCXX_DEBUG_PEDANTIC

mingw_cmake_system=Windows
