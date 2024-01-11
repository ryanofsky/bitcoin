package=native_libmultiprocess
<<<<<<< HEAD
$(package)_version=8da797c5f1644df1bffd84d10c1ae9836dc70d60
||||||| parent of 56ef459b5734 (Update libmultiprocess library)
$(package)_version=414542f81e0997354b45b8ade13ca144a3e35ff1
=======
$(package)_version=2cbbd09d8b9972a8f5c68b510c0dae9a9f7a22da
>>>>>>> 56ef459b5734 (Update libmultiprocess library)
$(package)_download_path=https://github.com/chaincodelabs/libmultiprocess/archive
$(package)_file_name=$($(package)_version).tar.gz
<<<<<<< HEAD
$(package)_sha256_hash=030f4d393d2ac9deba98d2e1973e22fc439ffc009d5f8ae3225c90639f86beb0
||||||| parent of 56ef459b5734 (Update libmultiprocess library)
$(package)_sha256_hash=8542dbaf8c4fce8fd7af6929f5dc9b34dffa51c43e9ee360e93ee0f34b180bc2
=======
$(package)_sha256_hash=a86416908cc4d27a41c58d846ca854730b1c56bd7a58d823348969e300a10014
>>>>>>> 56ef459b5734 (Update libmultiprocess library)
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
