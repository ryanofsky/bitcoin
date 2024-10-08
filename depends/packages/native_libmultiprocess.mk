package=native_libmultiprocess
$(package)_version=0bc674186d2316602f5d421774fe24784a11ee8f
$(package)_download_path=https://github.com/chaincodelabs/libmultiprocess/archive
$(package)_file_name=$($(package)_version).tar.gz
$(package)_sha256_hash=1bc6285dbfe2f5ae767828a603e5d93b4bf57435b459080950533c7831bb4251
$(package)_dependencies=native_capnp

define $(package)_config_cmds
  $($(package)_cmake) .
endef

define $(package)_build_cmds
  $(MAKE)
endef

define $(package)_stage_cmds
  $(MAKE) DESTDIR=$($(package)_staging_dir) install-bin
endef
