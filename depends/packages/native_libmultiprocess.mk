package=native_libmultiprocess
$(package)_version=9558ceb0d47ac1f62f88d29ec55f8f3099e32b20
$(package)_download_path=https://github.com/chaincodelabs/libmultiprocess/archive
$(package)_file_name=$($(package)_version).tar.gz
$(package)_sha256_hash=9dae2d4c352fc9a12968b0a05b11faac07900135a5dc8a335e1195d00908314a
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
