#!/bin/bash

# Activate conda
eval "$(conda shell.bash hook)"
conda activate sosa-compiler || { echo "conda ctivate environment failed."; exit 1; }

# Rebuild
pushd ./build
make -j 10 || { echo "build failed."; exit 1; }
popd

COMPILER="${COMPILER:-./build/compiler_mt}"
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
                --out_dir=experiments/${dir}/${hash}

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
                --out_dir=experiments/${dir}/${hash}

            ${COMPILER} -r ${r} -c ${c} -N ${N} -I ${interconn} -d experiments/${dir}/${hash} &
            pids[${cnt}]=$!
            echo pid:${pids[${cnt}]} hash:${hash} ${model} ${N} ${interconn}
            cnt=${cnt}+1
        done
    done
done

for pid in ${pids[*]}; do
    wait $pid
done

end_time=$(date +%s)
elapsed=$(( end_time - start_time ))
echo "Completed in $elapsed seconds."
