
source ~/.bashrc 
eval "$(conda shell.bash hook)"
conda activate sosa-compiler 


model="bert_large"
r=32
c=32
N=128

start_time=$(date +%s)

python precompiler/precompile.py \
    --model=${model} \
    --batch_size=1 \
    --sentence_len=100 \
    --array_size ${r} ${c} \
    --out_dir=experiments/tmp

./main --r=${r} --c=${c} --N=${N}

end_time=$(date +%s)
elapsed=$(( end_time - start_time ))
echo "Completed in $elapsed seconds."