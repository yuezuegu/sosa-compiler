#!/bin/bash

r=32
c=32
N=128
model="bert_tiny"
interconn="benes_vanilla"

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

# parse command line arguments
skip_rebuild=false
skip_precompile=false
use_multithreading=false

# see:
# https://stackoverflow.com/questions/402377/using-getopts-to-process-long-and-short-command-line-options/7948533
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
        skip_rebuild) # Skips rebuilding the program
            skip_rebuild=true
            ;;
        skip_precompile) # Skip precompiling
            skip_precompile=true
            ;;
        use_multithreading)
            use_multithreading=true
            ;;
        ??*)
            fail_msg "Illegal long option --$OPT"
            ;;
        ?)
            fail_msg "getopt error."
            ;;
    esac
done

# Activate conda
eval "$(conda shell.bash hook)"
conda activate sosa-compiler || { echo "conda ctivate environment failed."; exit 1; }

# Rebuild
if [[ ! "$skip_rebuild" = true ]]
then
    pushd ./build
    make -j 10 || { echo "build failed."; exit 1; }
    popd
else
    echo "Skipping rebuild."
fi

COMPILER="${COMPILER:-./build/compiler}"

if [[ "$use_multithreading" = true ]]
then
    COMPILER="${COMPILER}_mt"
else
    COMPILER="${COMPILER}_st"
fi

echo "Using COMPILER=${COMPILER}"

start_time=$(date +%s)

DIR="experiments/test"
mkdir -p "$DIR"

if [[ ! "$skip_precompile" = true ]]
then
    python precompiler/precompile.py \
        --model=${model} \
        --batch_size=1 \
        --sentence_len=100 \
        --array_size ${r} ${c} \
        --out_dir="$DIR"
else
    echo "Skipping precompiler."
fi

${COMPILER} -r ${r} -c ${c} -N ${N} -I ${interconn} -d "$DIR"

end_time=$(date +%s)
elapsed=$(( end_time - start_time ))
echo "Completed in $elapsed seconds."