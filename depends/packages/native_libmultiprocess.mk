package=native_libmultiprocess
<<<<<<< HEAD
$(package)_version=011fc53aeaf2b8bfceb8e738aa9bf512a240496e
$(package)_download_path=https://github.com/chaincodelabs/libmultiprocess/archive
$(package)_file_name=$($(package)_version).tar.gz
$(package)_sha256_hash=43737480f59fbad16db33dd08790fd245bb6660491a9a41f060406c26d1a23d4
||||||| parent of 43fb933969cf (depends: Switch libmultiprocess packages to use local git subtree)
$(package)_version=26b9f3dda42110a8ffa5e81b0ea78ba1e30ae659
$(package)_download_path=https://github.com/chaincodelabs/libmultiprocess/archive
$(package)_file_name=$($(package)_version).tar.gz
$(package)_sha256_hash=08e9715841cb4049c3b7c49f592dfaa59a4f7863395c632fa484435e701cf304
=======
$(package)_local_dir=../src/ipc/libmultiprocess
>>>>>>> 43fb933969cf (depends: Switch libmultiprocess packages to use local git subtree)
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
