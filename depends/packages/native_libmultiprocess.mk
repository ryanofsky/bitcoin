package=native_libmultiprocess
$(package)_version=f09c50118f78321ab11be36326318e3b3c12d095
$(package)_download_path=https://github.com/chaincodelabs/libmultiprocess/archive
$(package)_file_name=$($(package)_version).tar.gz
$(package)_sha256_hash=9640ef6ebf2a9898ddb6cf96f2dbd2cb0bb1ce7d6a74dd73d00a3c37223690db
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
