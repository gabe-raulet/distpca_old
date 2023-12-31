#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <time.h>
#include <math.h>
#include <mpi.h>
#include "distpca.h"
#include "utils.h"
#include "mmio.h"

int p = 10;
char *Afname, *PCfname, *expvarfname;

int usage(char *argv[]);
double* mmread_centered(char const *fname, int *n, int *d);

int main(int argc, char *argv[])
{
    int myrank, nprocs;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

    if (argc != 5)
    {
        if (!myrank) fprintf(stderr, "Usage: %s <A.mtx> <PC.mtx> <expvar.txt> <p>\n", argv[0]);
        MPI_Finalize();
        return 1;
    }

    if (nprocs == 1 || (nprocs&(nprocs-1)))
    {
        if (!myrank) fprintf(stderr, "[error::main][nprocs=%d] nprocs must be a power of 2 greater than 1\n", nprocs);
        MPI_Finalize();
        return 1;
    }


    Afname = argv[1];
    PCfname = argv[2];
    expvarfname = argv[3];
    p = atoi(argv[4]);

    int n, d;
    double *A, *Aloc;

    mpi_timer_t timer;
    double maxtime, proctime;

    mpi_timer_init(&timer, MPI_COMM_WORLD);
    mpi_timer_start(&timer);

    if (!myrank)
    {
        A = mmread_centered(Afname, &n, &d);
    }

    MPI_Bcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&d, 1, MPI_INT, 0, MPI_COMM_WORLD);

    int s = d / nprocs;


    if (!myrank)
    {
        if (!(1 <= d && d <= n) || (n&(n-1)) || (d&(d-1)))
        {
            if (!myrank) fprintf(stderr, "[error::main][n=%d,d=%d] must have 1 <= d <= n with d and n both being powers of 2\n", n, d);
            MPI_Finalize();
            return 1;
        }

        if (p > d || d % nprocs != 0 || p > s)
        {
            if (!myrank) fprintf(stderr, "[error::main][p=%d,d=%d,nprocs=%d] must have p <= d, d %% nprocs == 0, and p <= d/nprocs\n", p, d, nprocs);
            MPI_Finalize();
            return 1;
        }
    }

    mpi_timer_stop(&timer);
    mpi_timer_query(&timer, &maxtime, &proctime);

    if (!myrank) fprintf(stderr, "[read_input::main::maxtime=%.5f(s)::proctime=%.5f(s)] finished\n", maxtime, proctime);

    mpi_timer_start(&timer);

    double *Up, *Sp, *Vtp;

    if (!myrank)
    {
        Up = dalloc(n*p, 0);
        Sp = dalloc(p, 0);
        Vtp = dalloc(p*d, 0);
    }
    else
    {
        Up = Sp = Vtp = NULL;
    }

    Aloc = dalloc(n*s, 0);
    MPI_Scatter(A, n*s, MPI_DOUBLE, Aloc, n*s, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    mpi_timer_stop(&timer);
    mpi_timer_query(&timer, &maxtime, &proctime);

    if (!myrank) fprintf(stderr, "[distribute_Amat::main::maxtime=%.5f(s)::proctime=%.5f(s)] finished\n", maxtime, proctime);

    mpi_timer_start(&timer);

    if (svd_dist(Aloc, Up, Sp, Vtp, n, d, p, 0, MPI_COMM_WORLD) != 0)
    {
        MPI_Finalize();
        return 1;
    }

    mpi_timer_stop(&timer);
    mpi_timer_query(&timer, &maxtime, &proctime);

    if (!myrank) fprintf(stderr, "[svd_dist::main::maxtime=%.5f(s)::proctime=%.5f(s)] finished\n", maxtime, proctime);

    mpi_timer_start(&timer);

    if (!myrank)
    {
        for (int i = 0; i < p; ++i)
        {
            Sp[i] = (Sp[i]*Sp[i])/(n-1.0);
        }

        FILE *f = fopen(expvarfname, "w");
        for (int i = 0; i < p; ++i)
            fprintf(f, "%.18e\n", Sp[i]);
        fclose(f);

        mmwrite(PCfname, Vtp, p, d);
    }

    mpi_timer_stop(&timer);
    mpi_timer_query(&timer, &maxtime, &proctime);

    if (!myrank) fprintf(stderr, "[write_files::main::maxtime=%.5f(s)::proctime=%.5f(s)] finished\n", maxtime, proctime);

    if (!myrank)
    {
        free(A);
        free(Up);
        free(Sp);
        free(Vtp);
    }
    free(Aloc);

    return 0;
}

double* mmread_centered(char const *fname, int *n, int *d)
{
    MM_typecode matcode;
    int N, D, i, j;
    double *A;
    FILE *f;

    f = fopen(fname, "r");
    mm_read_banner(f, &matcode);
    assert(mm_is_dense(matcode));
    mm_read_mtx_array_size(f, &N, &D);
    A = dalloc(N*D, 0);
    assert(A != NULL);

    for (j = 0; j < D; ++j)
    {
        double *col = &A[j*N];
        double mean = 0.;

        for (i = 0; i < N; ++i)
        {
            fscanf(f, "%lg", &col[i]);
            mean += col[i];
        }

        mean /= N;

        for (i = 0; i < N; ++i)
            col[i] -= mean;
    }

    fclose(f);

    *n = N;
    *d = D;

    return A;
}
