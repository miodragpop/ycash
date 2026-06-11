package=jemalloc
$(package)_version=5.3.0
$(package)_download_path=https://github.com/jemalloc/jemalloc/releases/download/$($(package)_version)
$(package)_file_name=$(package)-$($(package)_version).tar.bz2
$(package)_sha256_hash=2db82d1e7119df3e71b7640219b6dfe84789bc0537983c3b7ac4f7189aecfeaa

# jemalloc: a fast general-purpose allocator. Linked into the Linux build only
# (see Makefile.am), where it measurably beats glibc malloc on the coins-cache
# allocation churn during reindex. Built UNPREFIXED so its malloc/free/new
# transparently replace glibc's via ELF symbol interposition -- no code changes,
# just -ljemalloc on the final link. Static archive only.
#
# Not built for the Windows (mingw) or macOS (darwin) cross targets: jemalloc's
# transparent-override story is weak on mingw, and Apple's libmalloc is already
# strong + needs zone interposition rather than plain symbol override. Gated via
# `linux_packages += jemalloc` in packages.mk.

define $(package)_set_vars
  # --enable-static --disable-shared: archive only (we link it statically).
  # --disable-cxx: jemalloc's C++ operator new/delete overrides are not needed;
  #   glibc/libstdc++ new routes through malloc, which jemalloc already replaces,
  #   and disabling avoids a libstdc++ ABI dependency in the archive.
  # --disable-stats: drop the per-arena stats bookkeeping (small perf/size win;
  #   we are not running MALLOC_CONF=stats_print in production).
  # --disable-libdl / no profiling: keep the archive self-contained and lean.
  $(package)_config_opts=--enable-static --disable-shared
  $(package)_config_opts+=--disable-cxx --disable-stats
  $(package)_config_opts+=--with-private-namespace=ycash_je_
endef

define $(package)_config_cmds
  $($(package)_autoconf)
endef

define $(package)_build_cmds
  $(MAKE) build_lib_static
endef

define $(package)_stage_cmds
  $(MAKE) DESTDIR=$($(package)_staging_dir) install_lib_static install_include
endef
