package=native_libmultiprocess
<<<<<<< HEAD
$(package)_version=9ba88dc2c7f4df1356c67e58e709f1609d336689
$(package)_download_path=https://github.com/chaincodelabs/libmultiprocess/archive
$(package)_file_name=$($(package)_version).tar.gz
$(package)_sha256_hash=515adff60182aef9aa26bbeee32519ca2b3ed5926db91a9236e913c61876f6db
||||||| parent of 97058b53e843 (depends: Switch libmultiprocess packages to use local git subtree)
$(package)_version=9558ceb0d47ac1f62f88d29ec55f8f3099e32b20
$(package)_download_path=https://github.com/chaincodelabs/libmultiprocess/archive
$(package)_file_name=$($(package)_version).tar.gz
$(package)_sha256_hash=9dae2d4c352fc9a12968b0a05b11faac07900135a5dc8a335e1195d00908314a
=======
$(package)_local_dir=../src/ipc/libmultiprocess
>>>>>>> 97058b53e843 (depends: Switch libmultiprocess packages to use local git subtree)
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
