package=native_libmultiprocess
<<<<<<< HEAD
$(package)_version=4dcd80741e867785100c761d1d8aea4dbdf89d15
||||||| merged common ancestors
$(package)_version=5741d750a04e644a03336090d8979c6d033e32c0
=======
$(package)_version=4c599773923afe6829139ce682d723d769e335dc
>>>>>>> Multiprocess bitcoin
$(package)_download_path=https://github.com/chaincodelabs/libmultiprocess/archive
$(package)_file_name=$($(package)_version).tar.gz
<<<<<<< HEAD
$(package)_sha256_hash=9fe68761c491f80da509dd6f5d9703b2221e9d3d998b37eb680c62b82c0d0ff8
||||||| merged common ancestors
$(package)_sha256_hash=ac848db49a6ed53e423c62d54bd87f1f08cbb0326254a8667e10bbfe5bf032a4
=======
$(package)_sha256_hash=bbfde6cf94069ec33309aeeea0f6d598629b410ae0fbf7716588b60892311b53
>>>>>>> Multiprocess bitcoin
$(package)_dependencies=native_capnp

define $(package)_config_cmds
  $($(package)_cmake)
endef

define $(package)_build_cmds
  $(MAKE)
endef

define $(package)_stage_cmds
  $(MAKE) DESTDIR=$($(package)_staging_dir) install
endef
