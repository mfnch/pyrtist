cimport numpy as np

import numpy as np

def _pose_calc_opt(np.ndarray[np.float32_t, ndim=2] out_vertices,
                   int out_idx, np.ndarray[np.float32_t, ndim=2] vertices,
                   np.ndarray[np.float32_t, ndim=1] weights,
                   np.ndarray[np.float32_t, ndim=3] matrices,
                   indices):
    cdef int num_vertices = vertices.shape[0]
    cdef int i, j, k, midx, widx
    cdef float total, contrib, weight

    for i in range(num_vertices):
        for midx, widx in indices[i]:
            weight = weights[widx]
            for j in range(3):
                contrib = 0.0
                for k in range(4):
                    contrib += matrices[midx, j, k] * vertices[i, k]
                out_vertices[out_idx, j] += weight * contrib
        out_idx += 1
    return out_idx
