# Summary

SOSA compiler repository.

## How to install prerequisites?

Execute the following from the root directory of the repository:

    bash ./install.sh

This command automatically install CMake and Boost.

## How to build?

Use CMake to build (assuming in the repository root directory):

    mkdir build-Debug  # directory for out-of-source build
    cd build-Debug
    cmake .. -DCMAKE_PREFIX_PATH="$HOME/.local"
    make

If you want a highly optimized release build, use instead:

    mkdir build-Release  # directory for out-of-source build
    cd build-Release
    cmake .. -DCMAKE_PREFIX_PATH="$HOME/.local" -DCMAKE_BUILD_TYPE=Release
    make

You can use CMake and CMake Tools extensions of vscode to facilitate development. Let CMake Tools to configure IntelliSense.

## Visual Studio Code configuration

After installation finishes, make sure that you restart vscode first by running "Remote-SSH: kill VS Code Server on Host...".

We provide `settings.json` and `launch.json` files to ease debugging and development. Please take a look at these files to have an idea what is going on under the hood.
