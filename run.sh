
source ~/.bashrc 
eval "$(conda shell.bash hook)"
conda activate sosa-compiler 

start_time=$(date +%s)

make clean
make final

dir=$(date "+%Y-%m-%d-%H-%M-%S")
mkdir experiments/${dir}

r=32
c=32

cnt=0
for model in bert_small bert_base bert_large
do
    for N in 32 64 128 256 512
    do
        for sentence_len in 20 60 100
        do
            for interconn in crossbar banyan_exp_1 benes_vanilla benes_copy 
            do
                hash=$(echo $(date +%s) | md5sum | cut -c1-8)

                mkdir experiments/${dir}/${hash}

                python precompiler/precompile.py \
                    --model=${model} \
                    --batch_size=1 \
                    --sentence_len=${sentence_len} \
                    --array_size ${r} ${c} \
                    --out_dir=experiments/${dir}/${hash}

                ./main -r ${r} -c ${c} -N ${N} -I ${interconn} -d experiments/${dir}/${hash} &
                pids[${cnt}]=$!
                echo pid:${pids[${cnt}]} hash:${hash} ${model} ${N} ${sentence_len} ${interconn} 
                cnt=${cnt}+1
            done
        done
    done
done

for model in inception resnet50 densenet121
do
    for N in 32 64 128 256 512
    do
        for interconn in crossbar banyan_exp_1 benes_vanilla benes_copy 
        do

            hash=$(echo $(date +%s) | md5sum | cut -c1-8)
            
            mkdir experiments/${dir}/${hash}

            python precompiler/precompile.py \
                --model=${model} \
                --batch_size=1 \
                --array_size ${r} ${c} \
                --out_dir=experiments/${dir}/${hash}

            ./main -r ${r} -c ${c} -N ${N} -I ${interconn} -d experiments/${dir}/${hash} &
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