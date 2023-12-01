import sys
import numpy as np
import subprocess as sp
from pathlib import Path
from scipy.io import mmread, mmwrite

def gen_params(colcnts, rowscales, cvals, dvals):
    params = []
    for n in colcnts:
        for scale in rowscales:
            for cond in cvals:
                for dmax in dvals:
                    params.append((scale*n, n, cond, dmax))
    return params

if __name__ == "__main__":

    target = Path("svd_matrix_cases")

    if target.exists():
        assert target.is_dir()
        for child in target.iterdir():
            child.unlink()
    else:
        target.mkdir()

    # colcnts = 2**np.arange(8,12)
    # rowscales = [1,2,4]
    # cvals = [2, 100]
    # dvals = [2, 4, 20, 88]

    colcnts = 2**np.arange(8,9)
    rowscales = [1,2,4]
    cvals = [2, 100]
    dvals = [2, 4, 20, 88]
    params = gen_params(colcnts, rowscales, cvals, dvals)

    for m, n, cond, dmax in params:
        label = str(target.joinpath(f"case_m{m}_n{n}_cond{cond}_dmax{dmax}"))
        cmd = f"./gen_svd -m {m} -n {n} -c {cond} -d {dmax} -o {label}"
        print(cmd)
        proc = sp.Popen(cmd.split(), stdout=sp.PIPE)
        proc.wait()