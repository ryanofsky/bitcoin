package=native_libmultiprocess
<<<<<<< HEAD
$(package)_version=477405eda34d923bd2ba6b3abc4c4d31db84c3ea
$(package)_download_path=https://github.com/chaincodelabs/libmultiprocess/archive
$(package)_file_name=$($(package)_version).tar.gz
$(package)_sha256_hash=a62db362fdd95a49c09bd7ac829990d99e937af4300c583772f46e1b3f8d7b43
||||||| parent of 6282b83442fb (depends: Switch libmultiprocess packages to use local git subtree)
$(package)_version=f09c50118f78321ab11be36326318e3b3c12d095
$(package)_download_path=https://github.com/chaincodelabs/libmultiprocess/archive
$(package)_file_name=$($(package)_version).tar.gz
$(package)_sha256_hash=9640ef6ebf2a9898ddb6cf96f2dbd2cb0bb1ce7d6a74dd73d00a3c37223690db
=======
$(package)_local_dir=../src/ipc/libmultiprocess
>>>>>>> 6282b83442fb (depends: Switch libmultiprocess packages to use local git subtree)
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
