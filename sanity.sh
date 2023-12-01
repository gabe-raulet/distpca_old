#!/bin/bash

rm -f *.mtx *.diag

echo "./gen_svd -m 256 -n 128 -u -1 -c 100 -d 2 -o sanity"
./gen_svd -m 256 -n 128 -u -1 -c 100 -d 2 -o sanity

echo "mpirun -np 8 ./dist_svd A_sanity.mtx S_sanity.diag U_sanity.mtx Vt_sanity.mtx 10"
mpirun -np 8 ./dist_svd A_sanity.mtx S_sanity.diag U_sanity.mtx Vt_sanity.mtx 10
