#!/bin/bash

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
conda activate sosa-compiler || fail_msg "conda activate environment failed."

# Rebuild
if [[ ! "$skip_rebuild" = true ]]
then
    pushd ./build
    make -j 10 || fail_msg "build failed."
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

dir=$(date "+%Y-%m-%d-%H-%M-%S")
mkdir -p experiments/${dir}

r=32
c=32

cnt=0
for model in bert_small bert_base bert_large
do
    for N in 32 64 128 256 512
    do
        for interconn in crossbar banyan_exp_0 banyan_exp_1 banyan_exp_2 benes_vanilla benes_copy 
        do
            hash=$(echo $(date +%N) | md5sum | cut -c1-12)

            mkdir experiments/${dir}/${hash}

            python precompiler/precompile.py \
                --model=${model} \
                --batch_size=1 \
                --sentence_len=100 \
                --array_size ${r} ${c} \
                --out_dir=experiments/${dir}/${hash} || fail_msg "precompile failed."

            ${COMPILER} -r ${r} -c ${c} -N ${N} -I ${interconn} -d experiments/${dir}/${hash} &
            pids[${cnt}]=$!
            echo pid:${pids[${cnt}]} hash:${hash} ${model} ${N} ${sentence_len} ${interconn} 
            cnt=${cnt}+1
        done
    done
done

for model in inception resnet50 densenet121
do
    for N in 32 64 128 256 512
    do
        for interconn in crossbar banyan_exp_0 banyan_exp_1 banyan_exp_2 benes_vanilla benes_copy
        do

            hash=$(echo $(date +%N) | md5sum | cut -c1-8)
            
            mkdir experiments/${dir}/${hash}

            python precompiler/precompile.py \
                --model=${model} \
                --batch_size=1 \
                --array_size ${r} ${c} \
                --out_dir=experiments/${dir}/${hash} || fail_msg "precompile failed."

            ${COMPILER} -r ${r} -c ${c} -N ${N} -I ${interconn} -d experiments/${dir}/${hash} &
            pids[${cnt}]=$!
            echo pid:${pids[${cnt}]} hash:${hash} ${model} ${N} ${interconn}
            cnt=${cnt}+1
        done
    done
done

for pid in ${pids[*]}
do
    wait $pid
done

end_time=$(date +%s)
elapsed=$(( end_time - start_time ))
echo "Completed in $elapsed seconds."
