
import numpy as np

#"banyan_exp_0", "banyan_exp_1", "banyan_exp_2", "banyan_exp_3", "benes_copy", "crossbar"
def calculate_peak_power(r_range, c_range, N, freq, e_data, e_compute, I_0, ict_type="banyan_exp_1"):
    sram_read = np.add.outer(r_range, 5 * c_range)
    p_data = sram_read * N * e_data * freq
    p_compute = np.multiply.outer(r_range, c_range) * N * e_compute * freq

    if ict_type == "banyan_exp_0":
        p_interconnect = I_0 * N * np.log2(N) * sram_read
    elif ict_type == "banyan_exp_1":
        p_interconnect = I_0 * 2 * N * np.log2(N) * sram_read
    elif ict_type == "banyan_exp_2":
        p_interconnect = I_0 * 4 * N * np.log2(N) * sram_read
    elif ict_type == "banyan_exp_3":
        p_interconnect = I_0 * 8 * N * np.log2(N) * sram_read
    elif ict_type == "benes_copy":
        p_interconnect = I_0 * 4 * N * np.log2(N) * sram_read
    elif ict_type == "crossbar":
        p_interconnect = I_0 * N * N * sram_read
    else:
        raise NotImplementedError

    p_total = p_data + p_compute + p_interconnect
    return p_total, p_data, p_compute, p_interconnect

def calculate_peak_throughput(r,c,N,freq):
    return 2*r*c*N*freq/1e12