#!/bin/bash

# URLs for software
URL_CMAKE="https://github.com/Kitware/CMake/releases/download/v3.21.3/cmake-3.21.3-linux-x86_64.tar.gz"
URL_BOOST="https://boostorg.jfrog.io/artifactory/main/release/1.77.0/source/boost_1_77_0.tar.gz"

# minimum requirements
CMAKE_MIN_VER=3.11.0
GCC_MIN_VER=8.0.0

# target prefix
PREFIX="$HOME/.local"
PROFILE_FILE="$HOME/.bashrc"

# helper functions
function fail_msg() {
    echo "$1" >&2
    exit 1
}

function check_program() {
    command -v "$1" &> /dev/null
}

function check_minimum_version() {
    installed=$(echo $1 | grep -Eo "([0-9]{1,}\.)+[0-9]{1,}")
    target="$2"
    # if in the ascending order, than $installed > $target
    printf "%s\n%s\n" "$target" "$installed" | sort --check=quiet --version-sort 
}

function fail_minimum_version() {
    program="$1"
    installed="$2"
    target="$3"
    if ! check_minimum_version "$installed" "$target"
    then
        fail_msg "$program $installed installed, version $target or better is required."
    else
        echo "$program $installed installed, satisfies version $target or better."
    fi
}

# silent pushd
function pushd() {
    command pushd "$@" > /dev/null
}

# silent popd
function popd() {
    command popd "$@" > /dev/null
}

function prepare_prefix() {
    if [[ ! -d "$PREFIX" ]]
    then
        echo "Creating the prefix directory: $PREFIX"
        mkdir -p "$PREFIX"
    fi
    echo "Prefix directory: $PREFIX"
}

# CMake-related
function cmake_version() {
    cmake -version | grep -Eo "([0-9]{1,}\.)+[0-9]{1,}"
}

function install_cmake() {
    echo "Installing cmake from:"
    echo "$URL_CMAKE"
    pushd "$TMPDIR"

    wget "$URL_CMAKE" -O "cmake.tar.gz"
    echo "Extracting files."
    tar -xf "cmake.tar.gz"
    echo "Copying files."
    cp -r cmake*/* "$PREFIX/"

    popd
    return 0
}

# Boost-related
function install_boost() {
    echo "Install boost from:"
    echo "$URL_BOOST"
    pushd "$TMPDIR"

        wget "$URL_BOOST" -O "boost.tar.gz"
        echo "Extracting files."
        tar -xf "boost.tar.gz"
        echo "Build and install."
        mv boost_* boost
        pushd ./boost
            # TODO change this line to build other libraries as well
            ./bootstrap.sh --prefix="$PREFIX/" --with-libraries=log,system,program_options
            ./b2 --prefix="$PREFIX/" install -j 8
        popd
    popd
    return 0
}

# Profile file related
function update_profile() {
    if [[ -e "$HOME/.profile_contains_prefix" ]]
    then
        echo "No need to update $PROFILE_FILE, it is already patched."
    else
        echo "Updating: $PROFILE_FILE"
        echo "Create a backup."
        cp -f "$PROFILE_FILE" "$PROFILE_FILE.$(date +%F).bak"
        echo "Update it."

        echo "# Add user $PREFIX/lib to LD_CONFIG_PATH" >> "$PROFILE_FILE"
        echo "export LD_LIBRARY_PATH=\"$PREFIX/lib:\$LD_LIBRARY_PATH\"" >> "$PROFILE_FILE"
        echo "# Add user $PREFIX/bin to PATH" >> "$PROFILE_FILE"
        echo "export PATH=\"$PREFIX/bin:\$PATH\"" >> "$PROFILE_FILE"

        echo "Done updating: $PROFILE_FILE"
    fi
}

# check the prerequistes
for program in grep wget g++ tar mktemp
do
    if ! check_program "$program"
    then
        fail_msg "$program is not found!"
    else
        echo "$program is installed."
    fi
done

# -dumpfullversion -dumpversion works well for older versions.
# TODO support checking for clang.
fail_minimum_version "g++" "$(g++ -dumpfullversion -dumpversion)" "$GCC_MIN_VER"
echo "If you have g++ linked to clang, this might not be accurate, otherwise no problem."

# prepare the temporary directory
TMPDIR=$(mktemp -d)
echo "Temporary directory: $TMPDIR"
[[ -e "$TMPDIR" ]] || fail_msg "Cannot create the temporary directory!"
trap "exit 1" HUP INT PIPE QUIT TERM
trap 'echo "Removing the temporary directory."; rm -rf "$TMPDIR"' EXIT

# prepare the prefix
prepare_prefix

# check cmake, we do not want to install if unless needed
if check_program "cmake"
then
    cmake_ver="$(cmake_version)"
    if check_minimum_version "$cmake_ver" "$CMAKE_MIN_VER"
    then
        echo "cmake $cmake_ver >= $CMAKE_MIN_VER is installed, skip installation."
    else
        echo "cmake $cmake_ver < $CMAKE_MIN_VER is installed, install cmake $CMAKE_MIN_VER."
        install_cmake
    fi
else
    echo "cmake is not found, install cmake $CMAKE_MIN_VER."
    install_cmake
fi

install_boost
update_profile
