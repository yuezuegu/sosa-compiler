# Summary

SOSA compiler repository.

## How to install prerequisites?

Execute the following from the root directory of the repository:

    bash ./install.sh

This command automatically install CMake and Boost.

## How to build?

Use CMake to build (assuming in the repository root directory):

    mkdir build  # directory for out-of-source build
    cd build
    cmake .. -DCMAKE_PREFIX_PATH="$HOME/.local"
    make

If you want a highly optimized release build, use instead:

    cmake .. -DCMAKE_PREFIX_PATH="$HOME/.local" -DCMAKE_BUILD_TYPE=Release

You can use CMake and CMake Tools extensions of vscode to facilitate development. Let CMake Tools to configure IntelliSense.
