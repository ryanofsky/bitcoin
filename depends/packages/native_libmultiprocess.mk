package=native_libmultiprocess
$(package)_version=9ba88dc2c7f4df1356c67e58e709f1609d336689
$(package)_download_path=https://github.com/chaincodelabs/libmultiprocess/archive
$(package)_file_name=$($(package)_version).tar.gz
$(package)_sha256_hash=515adff60182aef9aa26bbeee32519ca2b3ed5926db91a9236e913c61876f6db
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
