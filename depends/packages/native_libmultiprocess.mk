package=native_libmultiprocess
<<<<<<< HEAD
$(package)_version=015e95f7ebaa47619a213a19801e7fffafc56864
||||||| parent of 8678ddae666c (depends: Update libmultiprocess library for CustomMessage functions)
$(package)_version=6aca5f389bacf2942394b8738bbe15d6c9edfb9b
=======
$(package)_version=a9e16da55e880e5a0aed5008307ea56edc33fbd1
>>>>>>> 8678ddae666c (depends: Update libmultiprocess library for CustomMessage functions)
$(package)_download_path=https://github.com/chaincodelabs/libmultiprocess/archive
$(package)_file_name=$($(package)_version).tar.gz
<<<<<<< HEAD
$(package)_sha256_hash=4b1266b121337f3f6f37e1863fba91c1a5ee9ad126bcffc6fe6b9ca47ad050a1
||||||| parent of 8678ddae666c (depends: Update libmultiprocess library for CustomMessage functions)
$(package)_sha256_hash=2efeed53542bc1d8af3291f2b6f0e5d430d86a5e04e415ce33c136f2c226a51d
=======
$(package)_sha256_hash=588afeaa8751ef56fe5bfdf1e40587809bcb617e4d2825064b185860977a4b5f
>>>>>>> 8678ddae666c (depends: Update libmultiprocess library for CustomMessage functions)
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
