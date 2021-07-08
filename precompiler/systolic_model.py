import numpy as np



def fold_data(array_size, input_size, weight_size):    
    assert input_size[1]==weight_size[0], "Matrix dimensions must match!"
    
    weight_chunks = []
    input_chunks = []
    
    for i in range( int(np.ceil(weight_size[1]/array_size[1]))-1 ):
        weight_modulo = [(weight_size[0]-1) % array_size[0] + 1, (weight_size[1]-1) % array_size[1] + 1]
        for j in range( int(np.ceil(weight_size[0]/array_size[0]))-1 ):
            weight_chunks.append([np.int64(array_size[0]), np.int64(array_size[1])])
            input_chunks.append([np.int64(input_size[0]), np.int64(array_size[0])])
        
        weight_chunks.append([np.int64(weight_modulo[0]), np.int64(array_size[1])])    
        input_chunks.append([np.int64(input_size[0]), np.int64(weight_modulo[0])])

    weight_modulo = [(weight_size[0]-1) % array_size[0] + 1, (weight_size[1]-1) % array_size[1] + 1]
    for j in range( int(np.ceil(weight_size[0]/array_size[0]))-1 ):
        weight_chunks.append([np.int64(array_size[0]), np.int64(weight_modulo[1])])
        input_chunks.append([np.int64(input_size[0]), np.int64(array_size[0])])
        
    weight_chunks.append([np.int64(weight_modulo[0]), np.int64(weight_modulo[1])])    
    input_chunks.append([np.int64(input_size[0]), np.int64(weight_modulo[0])])
    
    return input_chunks, weight_chunks

def distribute_batch(array_size, input_chunks, weight_chunks):    
    no_chunks = len(input_chunks)

    input_chunks_dist = []
    weight_chunks_dist = []

    min_chunk_row = array_size[1]

    for i in range(no_chunks):
        no_input_row = input_chunks[i][0]
        no_fold = int( np.floor(no_input_row / min_chunk_row) - 1)
        #fold_size = int(np.ceil(no_input_row/no_fold))
        fold_size = min_chunk_row

        row_cnt = 0
        for j in range(no_fold):
            input_chunks_dist.append([fold_size, input_chunks[i][1]])
            weight_chunks_dist.append([weight_chunks[i][0], weight_chunks[i][1]])

            row_cnt = row_cnt + fold_size

        input_chunks_dist.append([no_input_row-row_cnt, input_chunks[i][1]])
        weight_chunks_dist.append([weight_chunks[i][0], weight_chunks[i][1]])

    return input_chunks_dist, weight_chunks_dist

def map_chunks(no_array, input_chunks, weight_chunks):

    chunks = []
    for (i,j) in weight_chunks.keys():
        for (k,l) in input_chunks.keys():
            if(i==l):
                chunks.append([input_chunks[(k,l)], weight_chunks[(i,j)]])

    chunk_map = []
    for i in range(no_array):
        chunk_map.append([])

    for c in range(len(chunks)):
        chunk_map[c%no_array].append(chunks[c])

    return chunk_map

def systolic_model(array_size, input_chunk, weight_chunk, last_t_exec=0, local_broadcast=1, local_fanin=1):
    no_sys_row, no_sys_col = array_size
    no_filt_row, no_filt_col = weight_chunk

    no_pe_all = no_sys_row * no_sys_col
    no_pe_active = no_filt_row * no_filt_col

    assert input_chunk[1] == weight_chunk[0], "Matrix dimensions must match!:{}-{}".format(input_chunk[1], weight_chunk[0])
    assert no_filt_row <= no_sys_row, "Input size cannot be bigger than array size!:{}-{}".format(no_filt_row, no_sys_row)
    assert no_filt_col <= no_sys_col, "Input size cannot be bigger than array size!:{}-{}".format(no_filt_col, no_sys_col)

    t_exec = 0
    t_weight_stall = np.maximum(no_filt_row - last_t_exec,0)

    t_exec += t_weight_stall

    horizontal_latency_stall = int(np.ceil(no_filt_col / local_broadcast)) - 1
    t_exec += horizontal_latency_stall

    t_exec += input_chunk[0]

    vertical_latency_stall = int(np.ceil(no_filt_row / local_fanin)) - 1
    t_exec += vertical_latency_stall

    cycle_pe_avail = t_exec * no_pe_all
    cycle_pe_weight_stall = t_weight_stall * no_pe_active

    cycle_pe_horizontal_latency = horizontal_latency_stall * no_filt_col * no_filt_row
    cycle_pe_vertical_latency = vertical_latency_stall * no_filt_row * no_filt_col

    cycle_pe_narrow_data = t_exec * (no_sys_col - no_filt_col) * no_sys_row
    cycle_pe_shallow_data = t_exec * (no_sys_row - no_filt_row) * no_sys_col

    cycle_pe_narrow_data -= t_exec * (no_sys_col - no_filt_col) * (no_sys_row - no_filt_row) / 2
    cycle_pe_shallow_data -= t_exec * (no_sys_col - no_filt_col) * (no_sys_row - no_filt_row) / 2

    cycle_pe_active = input_chunk[0] * no_pe_active

    assert cycle_pe_avail == cycle_pe_weight_stall + cycle_pe_horizontal_latency + cycle_pe_vertical_latency + cycle_pe_narrow_data + cycle_pe_shallow_data + cycle_pe_active, "Something does not add up! {}".format([t_exec, cycle_pe_avail, cycle_pe_weight_stall, cycle_pe_horizontal_latency, cycle_pe_vertical_latency, cycle_pe_narrow_data, cycle_pe_shallow_data, cycle_pe_active])

    cycle_stats = [cycle_pe_avail, cycle_pe_weight_stall, cycle_pe_horizontal_latency, cycle_pe_vertical_latency, cycle_pe_narrow_data, cycle_pe_shallow_data, cycle_pe_active]
    
    return t_exec, cycle_stats

if __name__ == "__main__":         
    array_size = [40,32]
    input_size = [150,64]
    weight_size = [80,64]

    local_broadcast = 1
    local_fanin = 1

    no_array = 1
    assert np.sqrt(no_array) == np.ceil(np.sqrt(no_array)), 'no_array should be a multiple of 2'

    x_tile_dims, w_tile_dims = split_mat(input_size,weight_size,array_size)

    
    
    
    
    