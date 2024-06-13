package=native_libmultiprocess
<<<<<<< HEAD
$(package)_version=015e95f7ebaa47619a213a19801e7fffafc56864
||||||| parent of 8574f79db9e2 (depends: Update libmultiprocess library)
$(package)_version=8da797c5f1644df1bffd84d10c1ae9836dc70d60
=======
$(package)_version=8b8a4766ce0a1892b9e8a5eb73dc39821005e520
>>>>>>> 8574f79db9e2 (depends: Update libmultiprocess library)
$(package)_download_path=https://github.com/chaincodelabs/libmultiprocess/archive
$(package)_file_name=$($(package)_version).tar.gz
<<<<<<< HEAD
$(package)_sha256_hash=4b1266b121337f3f6f37e1863fba91c1a5ee9ad126bcffc6fe6b9ca47ad050a1
||||||| parent of 8574f79db9e2 (depends: Update libmultiprocess library)
$(package)_sha256_hash=030f4d393d2ac9deba98d2e1973e22fc439ffc009d5f8ae3225c90639f86beb0
=======
$(package)_sha256_hash=475c0dc2357a2ff30e9a164e4c16dc8a6597a57c9193d646fa9cbf0a55c45d78
>>>>>>> 8574f79db9e2 (depends: Update libmultiprocess library)
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
