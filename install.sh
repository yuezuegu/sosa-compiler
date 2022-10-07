#!/bin/bash

# URLs for software
URL_CMAKE="https://github.com/Kitware/CMake/releases/download/v3.21.3/cmake-3.21.3-linux-x86_64.tar.gz"
URL_BOOST="https://boostorg.jfrog.io/artifactory/main/release/1.77.0/source/boost_1_77_0.tar.gz"
URL_CONDA="https://repo.anaconda.com/miniconda/Miniconda3-py39_4.10.3-Linux-x86_64.sh"

# minimum requirements
CMAKE_MIN_VER=3.11.0
GCC_MIN_VER=7.0.0

# conda-related
CONDA_ENV=sosa-compiler

# target prefix
PREFIX="$HOME/.local"
CONDA_TARGET="$HOME/miniconda"
PROFILE_FILE="$HOME/.bashrc"

# flags
should_install_cmake=true
should_install_boost=true
should_install_conda=false
should_install_condaenv=false
should_update_profile=true

function fail_msg() {
    echo "$1" >&2
    exit 1
}

# for cmd line parsing
function usage() {
    echo "$0 usage: "
    grep ")\ #" "$0"
    exit 0
}

while getopts "h-:" OPT
do
    if [[ "$OPT" = "-" ]]
    then
        OPT="${OPTARG%%=*}"
        OPTARG="${OPTARG#$OPT}"
        OPTARG="${OPTARG#=}"
    fi

    case "$OPT" in
        help | h) # help message
            usage
            ;;
        install_cmake) # Should I install cmake? [true/false] (default = true)
            should_install_cmake="$OPTARG"
            ;;
        install_boost) # Should I install boost? [true/false] (default = true)
            should_install_boost="$OPTARG"
            ;;
        install_conda) # Should I install conda? [true/false] (default = true)
            should_install_conda="$OPTARG"
            ;;
        install_condaenv) # Should I install the conda environment? [true/false] (default = true)
            should_install_condaenv="$OPTARG"
            ;;
        update_profile) # Should I update .bashrc? [true/false] (default = true)
            should_update_profile="$OPTARG"
            ;;
        ??*)
            fail_msg "Illegal long option --$OPT"
            ;;
        ?)
            fail_msg "getopt error."
            ;;
    esac
done

SCRIPT_DIR="$(pwd)"

# helper functions
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

    wget "$URL_CMAKE" -O "cmake.tar.gz" || fail_msg "wget failed."
    echo "Extracting files."
    tar -xf "cmake.tar.gz" || fail_msg "tar failed."
    echo "Copying files."
    cp -r cmake*/* "$PREFIX/" || fail_msg "cp failed."

    popd
    return 0
}

# Boost-related
function install_boost() {
    echo "Install boost from:"
    echo "$URL_BOOST"
    pushd "$TMPDIR"
        wget "$URL_BOOST" -O "boost.tar.gz" || fail_msg "wget failed."
        echo "Extracting files."
        tar -xf "boost.tar.gz" || fail_msg "tar failed."
        echo "Build and install."
        mv boost_* boost || fail_msg "mv failed."
        pushd ./boost
            # TODO change this line to build other libraries as well
            ./bootstrap.sh --prefix="$PREFIX/" --with-libraries=log,system,program_options,serialization,filesystem,thread || \
            #./bootstrap.sh --prefix="$PREFIX/" || \
                fail_msg "bootstrap.sh failed."
            ./b2 --prefix="$PREFIX/" install -j 8 || fail_msg "b2 failed."
        popd
    popd
    return 0
}

# Conda-related
function install_conda() {
    echo "Install conda from:"
    echo "$URL_CONDA"
    pushd "$TMPDIR"
        wget "$URL_CONDA" -O "conda.sh" || fail_msg "wget failed."
        echo "Install."
        bash ./conda.sh -b -f -p "$CONDA_TARGET" || fail_msg "install failed."
        source "$CONDA_TARGET/bin/activate" || fail_msg "activate failed."
        conda init || fail_msg "init failed."
    popd
    return 0
}

function install_conda_env() {
    echo "Install conda env"

    pushd "$TMPDIR"
        eval "$(conda shell.bash hook)"
        if [[ "$(conda env list)" == *${CONDA_ENV}*  ]]
        then
            echo "${CONDA_ENV} is found, removing it."
            conda env remove -n ${CONDA_ENV} || fail_msg "conda env remove failed"
        fi

        conda env create -f "$SCRIPT_DIR/conda.yml" || fail_msg "conda env create failed"
        conda activate $CONDA_ENV || fail_msg "conda env activate failed"
    popd
    return 0
}

# Profile file related
function update_profile() {
    if [[ -e "$HOME/.sosa-compiler-install-sh" ]]
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

        echo "created by install.sh of sosa-compiler." >> "$HOME/.sosa-compiler-install-sh"

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
echo "If you have g++ linked to clang, this might not be accurate; otherwise no problem."

# prepare the temporary directory
TMPDIR=$(mktemp -d)
echo "Temporary directory: $TMPDIR"
[[ -e "$TMPDIR" ]] || fail_msg "Cannot create the temporary directory!"
trap "exit 1" HUP INT PIPE QUIT TERM
trap 'echo "Removing the temporary directory."; rm -rf "$TMPDIR"' EXIT

# prepare the prefix
prepare_prefix

if [[ "$should_install_conda" = true ]]
then
    # check conda
    if check_program "conda"
    then
        echo "conda is found, update it."
        conda update -y conda || fail_msg "update conda failed"
    else
        echo "conda is not found, install conda."
        install_conda
    fi
else
    echo "Not installing conda."
fi

if [[ "$should_install_cmake" = true ]]
then
    # check cmake, we do not want to install if unless needed
    if check_program "cmake"
    then
        cmake_ver="$(cmake_version)"
        if check_minimum_version "$cmake_ver" "$CMAKE_MIN_VER"
        then
            echo "cmake $cmake_ver >= $CMAKE_MIN_VER is installed, skip installation."
        else
            echo "cmake $cmake_ver < $CMAKE_MIN_VER is installed, install cmake."
            install_cmake
        fi
    else
        echo "cmake is not found, install cmake."
        install_cmake
    fi
else
    echo "Not installing cmake."
fi

if [[ "$should_install_boost" = true ]]
then
    install_boost
else
    echo "Not installing boost."
fi

if [[ "$should_install_condaenv" = true ]]
then
    install_conda_env
else
    echo "Not installing the conda environment."
fi

if [[ "$should_update_profile" = true ]]
then
    update_profile
else
    echo "Not updating .bashrc."
fi
