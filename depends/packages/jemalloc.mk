package=jemalloc
$(package)_version=5.3.0
$(package)_download_path=https://github.com/jemalloc/jemalloc/releases/download/$($(package)_version)
$(package)_file_name=$(package)-$($(package)_version).tar.bz2
$(package)_sha256_hash=2db82d1e7119df3e71b7640219b6dfe84789bc0537983c3b7ac4f7189aecfeaa

# jemalloc: a fast general-purpose allocator. Linked into the Linux and Windows
# (mingw) builds (see Makefile.am), where it beats the platform allocator on the
# coins-cache allocation churn during reindex. Static archive only.
#
# Built UNPREFIXED (no --with-jemalloc-prefix => public malloc/free, and the
# JEMALLOC_IS_MALLOC path) so its malloc/free transparently become the program's
# allocator and operator new (libstdc++/libc++ implement new on top of malloc)
# benefits without code changes. On ELF this works via symbol interposition; on
# PE/COFF jemalloc DEFAULTS the prefix to "je_" (so it would NOT replace malloc),
# so for the mingw target we force --with-jemalloc-prefix="" to re-enable the
# JEMALLOC_IS_MALLOC behaviour, then rely on --whole-archive at the final link
# (jemalloc itself sets link_whole_archive=1 for static mingw).
#
# Not built for the macOS (darwin) cross target: Apple's libmalloc is already
# strong and needs zone interposition rather than plain symbol override, and the
# arm64 depends port is freshly validated -- not worth perturbing. Gated via
# linux_packages / mingw32_packages in packages.mk.

define $(package)_set_vars
  # --enable-static --disable-shared: archive only (we link it statically).
  # --disable-cxx: jemalloc's own operator new/delete overrides are not needed;
  #   libstdc++/libc++ new routes through malloc, which jemalloc replaces, and
  #   disabling avoids a C++ ABI dependency in the archive.
  # --disable-stats: drop the per-arena stats bookkeeping (small perf/size win).
  $(package)_config_opts=--enable-static --disable-shared
  $(package)_config_opts+=--disable-cxx --disable-stats
  $(package)_config_opts+=--with-private-namespace=ycash_je_
  # On PE/COFF jemalloc defaults to the je_ prefix (no malloc replacement); force
  # the empty prefix so it becomes the program allocator like on Linux.
  $(package)_config_opts_mingw32+=--with-jemalloc-prefix=
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
