package=native_libmultiprocess
$(package)_version=1b4012cadd82840b477d4289ed44c2300d6dd3f3
$(package)_download_path=https://github.com/chaincodelabs/libmultiprocess/archive
$(package)_file_name=$($(package)_version).tar.gz
$(package)_sha256_hash=52915e57d2e6982604900f9ad34a8662efaee61f1d94bcfc0df40d54fc1e0355
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
