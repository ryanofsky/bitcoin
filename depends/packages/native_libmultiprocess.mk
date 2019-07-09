package=native_libmultiprocess
$(package)_version=f2ea4b91b4f16d5a634cf0e8abacd5fe15c5a4e8
$(package)_download_path=https://github.com/chaincodelabs/libmultiprocess/archive
$(package)_file_name=$($(package)_version).tar.gz
$(package)_sha256_hash=ca4ef6e7521cc1132e758f5b2332ef5daf27edfb6ac8d7366fc5df0740b139a0
$(package)_dependencies=native_boost native_capnp

define $(package)_config_cmds
  cmake -DCMAKE_INSTALL_PREFIX=$($($(package)_type)_prefix)
endef

define $(package)_build_cmds
  $(MAKE)
endef

define $(package)_stage_cmds
  $(MAKE) DESTDIR=$($(package)_staging_dir) install
endef
