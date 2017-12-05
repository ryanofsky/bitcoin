package=native_libmultiprocess
$(package)_version=4c599773923afe6829139ce682d723d769e335dc
$(package)_download_path=https://github.com/chaincodelabs/libmultiprocess/archive
$(package)_file_name=$($(package)_version).tar.gz
$(package)_sha256_hash=bbfde6cf94069ec33309aeeea0f6d598629b410ae0fbf7716588b60892311b53
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
