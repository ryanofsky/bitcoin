package=native_libmultiprocess
$(package)_version=66e12f1fae6458788d234e8c1a0f43d34e72640b
$(package)_download_path=https://github.com/chaincodelabs/libmultiprocess/archive
$(package)_file_name=$($(package)_version).tar.gz
$(package)_sha256_hash=96a76c360485d839384a4f3f5975c981e4a72cb38b5280c377b2b4705dbc2da2
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
