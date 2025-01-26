package=native_libmultiprocess
$(package)_version=e89b2c6ac2cfeb03b71f90b2b44797ceee4d6500
$(package)_download_path=https://github.com/chaincodelabs/libmultiprocess/archive
$(package)_file_name=$($(package)_version).tar.gz
$(package)_sha256_hash=6520593a7aaa8fa6592f7a822c941a785f2c5bc8f59c351ec0b707d7f33b12bd
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
