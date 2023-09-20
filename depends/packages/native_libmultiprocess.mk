package=native_libmultiprocess
$(package)_version=aea56f0e2a88e1d4409285f040612a0566335e7e
$(package)_download_path=https://github.com/chaincodelabs/libmultiprocess/archive
$(package)_file_name=$($(package)_version).tar.gz
$(package)_sha256_hash=59d3fba94f96791afc2eb9cde1f2bd7e5293a168c74b87e34e1d88037c31815d
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
