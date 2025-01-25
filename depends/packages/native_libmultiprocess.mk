package=native_libmultiprocess
<<<<<<< HEAD
$(package)_version=408990787f8dfe6f6fd0654a3b092e918b8190b6
$(package)_download_path=https://github.com/chaincodelabs/libmultiprocess/archive
$(package)_file_name=$($(package)_version).tar.gz
$(package)_sha256_hash=c9f80e0d7bf62cb10102a7da1619e3dd5dda0e20a6e94f4e9286e27c93280de5
||||||| parent of 4ab5105d0725 (depends: Switch libmultiprocess packages to use local git subtree)
$(package)_version=9ba88dc2c7f4df1356c67e58e709f1609d336689
$(package)_download_path=https://github.com/chaincodelabs/libmultiprocess/archive
$(package)_file_name=$($(package)_version).tar.gz
$(package)_sha256_hash=515adff60182aef9aa26bbeee32519ca2b3ed5926db91a9236e913c61876f6db
=======
$(package)_local_dir=../src/ipc/libmultiprocess
>>>>>>> 4ab5105d0725 (depends: Switch libmultiprocess packages to use local git subtree)
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
