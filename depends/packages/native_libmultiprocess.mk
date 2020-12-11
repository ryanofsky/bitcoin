package=native_libmultiprocess
$(package)_version=4dcd80741e867785100c761d1d8aea4dbdf89d15
$(package)_download_path=https://github.com/chaincodelabs/libmultiprocess/archive
$(package)_file_name=$($(package)_version).tar.gz
$(package)_sha256_hash=9fe68761c491f80da509dd6f5d9703b2221e9d3d998b37eb680c62b82c0d0ff8
$(package)_dependencies=native_capnp

define $(package)_config_cmds
  $($(package)_cmake)
endef

define $(package)_build_cmds
  $(MAKE)
endef

define $(package)_stage_cmds
  $(MAKE) DESTDIR=$($(package)_staging_dir) install
endef
