package=native_libmultiprocess
$(package)_version=408990787f8dfe6f6fd0654a3b092e918b8190b6
$(package)_download_path=https://github.com/chaincodelabs/libmultiprocess/archive
$(package)_file_name=$($(package)_version).tar.gz
$(package)_sha256_hash=c9f80e0d7bf62cb10102a7da1619e3dd5dda0e20a6e94f4e9286e27c93280de5
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
