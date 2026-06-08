package=libcxx
$(package)_version=$(if $(native_clang_version_$(host_arch)_$(host_os)),$(native_clang_version_$(host_arch)_$(host_os)),$(if $(native_clang_version_$(host_os)),$(native_clang_version_$(host_os)),$(native_clang_default_version)))

ifneq ($(canonical_host),$(build))
ifneq ($(host_os),mingw32)
# Clang is provided pre-compiled for a bunch of targets; fetch the one we need
# and stage its copies of the static libraries.
$(package)_download_path=$(native_clang_download_path)
$(package)_download_file_aarch64_linux=LLVM-$($(package)_version)-Linux-ARM64.tar.xz
$(package)_file_name_aarch64_linux=LLVM-$($(package)_version)-Linux-ARM64.tar.xz
$(package)_sha256_hash_aarch64_linux=118ca2d3ad9da34367e05735317854e7977db45dc4c02a32af58da64c23b8789
ifeq ($(LEGACY_GLIBC),1)
# Match native_clang's LEGACY_GLIBC pin (Clang 18.1.8 ubuntu-18.04 tarball,
# different naming than the default Clang 22 LLVM-*-Linux-X64). This is only
# reached on a Linux->Linux CROSS build with LEGACY_GLIBC; the common native
# 18.04 build (build==host) takes the bottom else-branch and stages libc++ from
# native_clang directly, never fetching here.
$(package)_download_file_linux=clang+llvm-$($(package)_version)-x86_64-linux-gnu-ubuntu-18.04.tar.xz
$(package)_file_name_linux=clang-llvm-$($(package)_version)-x86_64-linux-gnu-ubuntu-18.04.tar.xz
$(package)_sha256_hash_linux=54ec30358afcc9fb8aa74307db3046f5187f9fb89fb37064cdde906e062ebf36
else
$(package)_download_file_linux=LLVM-$($(package)_version)-Linux-X64.tar.xz
$(package)_file_name_linux=LLVM-$($(package)_version)-Linux-X64.tar.xz
$(package)_sha256_hash_linux=edb0522b41e261819c06ea437d249f9b8acfa413d3805bc9920eec6fb76ff830
endif

# Starting from LLVM 14.0.0, some Clang binary tarballs store libc++ in a
# target-specific subdirectory.
define $(package)_stage_cmds
  mkdir -p $($(package)_staging_prefix_dir)/lib && \
  (test ! -f lib/*/libc++.a    || cp lib/*/libc++.a    $($(package)_staging_prefix_dir)/lib) && \
  (test ! -f lib/*/libc++abi.a || cp lib/*/libc++abi.a $($(package)_staging_prefix_dir)/lib) && \
  (test ! -f lib/libc++.a      || cp lib/libc++.a      $($(package)_staging_prefix_dir)/lib) && \
  (test ! -f lib/libc++abi.a   || cp lib/libc++abi.a   $($(package)_staging_prefix_dir)/lib)
endef

else
# For Windows cross-compilation, libc++ (with libc++abi and libunwind) is provided
# by the llvm-mingw toolchain (see packages/native_llvm_mingw.mk), so nothing is
# staged here. This empty package only exists to satisfy the libcxx dependency that
# boost, zeromq, bdb, and googletest declare.
define $(package)_fetch_cmds
endef

define $(package)_extract_cmds
endef

define $(package)_stage_cmds
  mkdir -p $($(package)_staging_prefix_dir)/lib
endef
endif

else
# For native compilation, use the static libraries from native_clang.
# We explicitly stage them so that subsequent dependencies don't link to the
# shared libraries distributed with Clang.
define $(package)_fetch_cmds
endef

define $(package)_extract_cmds
endef

define $(package)_stage_cmds
  mkdir -p $($(package)_staging_prefix_dir)/lib && \
  (test ! -f $(build_prefix)/lib/*/libc++.a    || cp $(build_prefix)/lib/*/libc++.a    $($(package)_staging_prefix_dir)/lib) && \
  (test ! -f $(build_prefix)/lib/*/libc++abi.a || cp $(build_prefix)/lib/*/libc++abi.a $($(package)_staging_prefix_dir)/lib) && \
  (test ! -f $(build_prefix)/lib/libc++.a      || cp $(build_prefix)/lib/libc++.a      $($(package)_staging_prefix_dir)/lib) && \
  (test ! -f $(build_prefix)/lib/libc++abi.a   || cp $(build_prefix)/lib/libc++abi.a   $($(package)_staging_prefix_dir)/lib)
endef

endif
