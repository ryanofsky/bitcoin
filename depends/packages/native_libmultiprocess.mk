package=native_libmultiprocess
<<<<<<< HEAD
$(package)_version=f09c50118f78321ab11be36326318e3b3c12d095
$(package)_download_path=https://github.com/chaincodelabs/libmultiprocess/archive
$(package)_file_name=$($(package)_version).tar.gz
$(package)_sha256_hash=9640ef6ebf2a9898ddb6cf96f2dbd2cb0bb1ce7d6a74dd73d00a3c37223690db
||||||| parent of 19eef973cb4a (depends: Switch libmultiprocess packages to use local git subtree)
$(package)_version=07c917f7ca910d66abc6d3873162fc9061704074
$(package)_download_path=https://github.com/chaincodelabs/libmultiprocess/archive
$(package)_file_name=$($(package)_version).tar.gz
$(package)_sha256_hash=ac9db311e3b22aac3c7b7b7b3f6b7fee5cf3043ebb3c3bf412049e8b17166de8
=======
$(package)_local_dir=../src/ipc/libmultiprocess
>>>>>>> 19eef973cb4a (depends: Switch libmultiprocess packages to use local git subtree)
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
