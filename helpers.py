
import numpy as np

def calculate_peak_power(r_range, c_range, N, freq, e_data, e_compute, I_0):
    sram_read = np.add.outer(r_range, 5 * c_range)
    p_data = sram_read * N * e_data * freq
    p_compute = np.multiply.outer(r_range, c_range) * N * e_compute * freq
    p_interconnect = I_0 * N * np.log2(N) * sram_read

    p_total = p_data + p_compute + p_interconnect
    return p_total, p_data, p_compute, p_interconnect

def calculate_peak_throughput(r,c,N,freq):
    return 2*r*c*N*freq/1e12