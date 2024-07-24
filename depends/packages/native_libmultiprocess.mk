package=native_libmultiprocess
<<<<<<< HEAD
$(package)_version=c1b4ab4eb897d3af09bc9b3cc30e2e6fff87f3e2
||||||| parent of 357a6bc97b19 (depends: Update libmultiprocess library for CustomMessage functions and cmake headers target)
$(package)_version=6aca5f389bacf2942394b8738bbe15d6c9edfb9b
=======
$(package)_version=a9e16da55e880e5a0aed5008307ea56edc33fbd1
>>>>>>> 357a6bc97b19 (depends: Update libmultiprocess library for CustomMessage functions and cmake headers target)
$(package)_download_path=https://github.com/chaincodelabs/libmultiprocess/archive
$(package)_file_name=$($(package)_version).tar.gz
<<<<<<< HEAD
$(package)_sha256_hash=6edf5ad239ca9963c78f7878486fb41411efc9927c6073928a7d6edf947cac4a
||||||| parent of 357a6bc97b19 (depends: Update libmultiprocess library for CustomMessage functions and cmake headers target)
$(package)_sha256_hash=2efeed53542bc1d8af3291f2b6f0e5d430d86a5e04e415ce33c136f2c226a51d
=======
$(package)_sha256_hash=588afeaa8751ef56fe5bfdf1e40587809bcb617e4d2825064b185860977a4b5f
>>>>>>> 357a6bc97b19 (depends: Update libmultiprocess library for CustomMessage functions and cmake headers target)
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
