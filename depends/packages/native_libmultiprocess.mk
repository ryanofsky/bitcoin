<<<<<<< HEAD
package=native_libmultiprocess
$(package)_version=78f2f75d4722c42988988efd2b0074d3ec64099e
$(package)_download_path=https://github.com/chaincodelabs/libmultiprocess/archive
$(package)_file_name=$($(package)_version).tar.gz
$(package)_sha256_hash=424e77ae3387b54517e7b2ea46d789d1ff7e29282aa6b9df83a627ac0be20816
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
||||||| merged common ancestors
=======
package=native_libmultiprocess
$(package)_version=c0e3a5045596872cb63110cba0ef17abae56fd07
$(package)_download_path=https://github.com/chaincodelabs/libmultiprocess/archive
$(package)_file_name=$($(package)_version).tar.gz
$(package)_sha256_hash=0c0aa4d4a573d1a68ea34bf4f233c44a062e21de9130fa2778fe857c7e28ada5
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
>>>>>>> libmultiprocess depends build
