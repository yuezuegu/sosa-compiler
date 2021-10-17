#!/bin/bash

# Activate conda
eval "$(conda shell.bash hook)"
conda activate sosa-compiler

# Rebuild
pushd ./build
make
popd

COMPILER="${COMPILER:-./build/compiler}"
echo "Using COMPILER=${COMPILER}"

start_time=$(date +%s)

DIR="experiments/test"
mkdir -p "$DIR"

r=32
c=32
N=32
model="bert_tiny"
interconn="benes_copy"

python precompiler/precompile.py \
    --model=${model} \
    --batch_size=1 \
    --sentence_len=100 \
    --array_size ${r} ${c} \
    --out_dir="$DIR"

${COMPILER} -r ${r} -c ${c} -N ${N} -I ${interconn} -d "$DIR"

end_time=$(date +%s)
elapsed=$(( end_time - start_time ))
echo "Completed in $elapsed seconds."
