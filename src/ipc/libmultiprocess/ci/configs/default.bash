CI_DESC="CI job using default libraries and tools, and running IWYU"
CI_DIR=build-default
<<<<<<< HEAD
export CXXFLAGS="-Werror -Wall -Wextra -Wpedantic -Wundef -Wunused-const-variable -Wno-unused-parameter -Wextra-semi -Wmissing-noreturn -Wtrailing-whitespace"
||||||| parent of 5fee9dcffdf (sync with libmultiprocess master)
export CXXFLAGS="-Werror -Wall -Wextra -Wpedantic -Wno-unused-parameter"
=======
export CXXFLAGS="-Werror -Wall -Wextra -Wpedantic -Wno-unused-parameter -Wextra-semi -Wmissing-noreturn"
>>>>>>> 5fee9dcffdf (sync with libmultiprocess master)
CMAKE_ARGS=(-DMP_ENABLE_IWYU=ON)
BUILD_ARGS=(-k)
