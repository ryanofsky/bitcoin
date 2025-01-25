package=native_libmultiprocess
<<<<<<< HEAD
$(package)_version=26b9f3dda42110a8ffa5e81b0ea78ba1e30ae659
$(package)_download_path=https://github.com/chaincodelabs/libmultiprocess/archive
$(package)_file_name=$($(package)_version).tar.gz
$(package)_sha256_hash=08e9715841cb4049c3b7c49f592dfaa59a4f7863395c632fa484435e701cf304
||||||| parent of 4d564c2220b9 (depends: Switch libmultiprocess packages to use local git subtree)
$(package)_version=408990787f8dfe6f6fd0654a3b092e918b8190b6
$(package)_download_path=https://github.com/chaincodelabs/libmultiprocess/archive
$(package)_file_name=$($(package)_version).tar.gz
$(package)_sha256_hash=c9f80e0d7bf62cb10102a7da1619e3dd5dda0e20a6e94f4e9286e27c93280de5
=======
$(package)_local_dir=../src/ipc/libmultiprocess
>>>>>>> 4d564c2220b9 (depends: Switch libmultiprocess packages to use local git subtree)
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
