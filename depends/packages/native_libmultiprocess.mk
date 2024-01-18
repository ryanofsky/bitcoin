package=native_libmultiprocess
$(package)_version=7d1fee06d65cefd4a59c51f1f6ea4754d0efbd99
$(package)_download_path=https://github.com/chaincodelabs/libmultiprocess/archive
$(package)_file_name=$($(package)_version).tar.gz
$(package)_sha256_hash=25db77d2ce7d668b75c5bc142ea22ec981d57224b563aec6e0f309684f9cfbb4
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
