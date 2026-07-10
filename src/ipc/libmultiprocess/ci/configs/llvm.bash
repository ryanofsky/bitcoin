CI_DESC="CI job using LLVM-based libraries and tools (clang, libc++, clang-tidy, iwyu) and testing Ninja"
CI_DIR=build-llvm
NIX_ARGS=(--arg enableLibcxx true)
export CXX=clang++
<<<<<<< HEAD
export CXXFLAGS="-Werror -Wall -Wextra -Wextra-semi -Wpedantic -Wundef -Wthread-safety -Wno-unused-parameter -Wno-c++23-lambda-attributes"
||||||| parent of 5fee9dcffdf (sync with libmultiprocess master)
export CXXFLAGS="-Werror -Wall -Wextra -Wpedantic -Wthread-safety -Wno-unused-parameter"
=======
export CXXFLAGS="-Werror -Wall -Wextra -Wextra-semi -Wpedantic -Wthread-safety -Wno-unused-parameter -Wno-c++23-lambda-attributes"
>>>>>>> 5fee9dcffdf (sync with libmultiprocess master)
CMAKE_ARGS=(
  -G Ninja
  -DMP_ENABLE_CLANG_TIDY=ON
  -DMP_ENABLE_IWYU=ON
)
BUILD_ARGS=(-k 0)
