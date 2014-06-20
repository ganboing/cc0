// k-means clustering on L0
// initial value is randomly generated
//
// dimention of points: 4
// number of points: N
// number of points each part: PSIZE
// k: K
// max number of iterations: MAX_ITER 

#include "libi0/stdio.h"
#include "libi0/stddef.h"
#include "libi0/stdint.h"

// #define DEBUG

// output centroids on finished
// #define OUTPUT_ON_FINISH
#define OUTPUT_ON_FINISH

#define PRINT_ITER

// dataset size: choose only one
#define TINY_EXP
// #define SMALL_EXP
// #define LARGE_EXP

#define MAX_ITER (6)
// #define MAX_ITER (20)


#ifdef TINY_EXP
#define N (10)
#define PSIZE (10)
#define K (2)

#else
#ifdef LARGE_EXP
#define N (204800000)
#define PSIZE (640000)
#define K (1000)

#else
#ifdef SMALL_EXP
#define N (512 * 1024)
#define PSIZE (128 * 1024)
#define K (20)

#else
#define N (64 * 1024 * 1024)
#define PSIZE (512 * 1024)
#define K (20)
#endif
#endif
#endif


// whether enable local aggregation
// Note: must defined
#define LOCAL_AGGREG


// Note: do not change this
// get input from STDIN
#define INPUT_STDIN
// Note: to use INPUT_STDIN, the N/PSIZE should be the number of VPCs


// use libi0 libraries instead
#define STDOUT (0x100000208)

#define PMEM_IN_SIZE  (N * 4 * sizeof_uint64_t)
#define PMEM_OUT_SIZE (K * 4 * sizeof_uint64_t)

#define PMEM_IN_ADDR  PPM_START
#define PMEM_OUT_ADDR (PPM_START + N * sizeof_uint64_t)

// for kmeans-from-pmem 
// and kmeans-stdin-to-pmem
#ifdef FROM_PMEM
standalone long *pmem_in;
#endif

standalone long x[N];
standalone long y[N];
standalone long z[N];
standalone long w[N];
standalone long ci[N];

standalone long cx[K];
standalone long cy[K];
standalone long cz[K];
standalone long cw[K];

#define N_LONG_PAGE 512

#define N_LOCAL_CENTROIDS (K+N_LONG_PAGE)*N/PSIZE

// for part i, centroid kk:
// (K+N_LONG_PAGE)*i + kk
standalone long lcx[N_LOCAL_CENTROIDS];
standalone long lcy[N_LOCAL_CENTROIDS];
standalone long lcz[N_LOCAL_CENTROIDS];
standalone long lcw[N_LOCAL_CENTROIDS];
standalone long lcn[N_LOCAL_CENTROIDS];

// standalone long trigger[N];
// opt
// Note: each trigger's content should not > N_LONG_PAGE Bytes
standalone long trigger[(N/PSIZE+1)*N_LONG_PAGE];
// standalone long changed[N]; // obsolete. Not used anymore.

standalone long iter;

register long reg1;
register long reg2;
register long reg3;
register long reg4;

// register usage:
// distance_sq_2

void start_iteration();

void srand(long seed)
{
    if (seed == 0) {
        seed = 1;
    }

    *((long *)0x300000000) = seed;
    return;
}

long rand()
{
    long r;

    r = *((long *)0x300000000);
    r = r * 0xfef3f6f4f3f2f1;
    *((long *)0x300000000) = r;
    return r;
}

#define distance_sq_2(x1, y1, z1, w1, x2, y2, z2, w2, d) \
reg1 = (x1) - (x2) ; \
reg2 = (y1) - (y2) ; \
reg3 = (z1) - (z2) ; \
reg4 = (w1) - (w2) ; \
*(d) = reg1*reg1 + reg2*reg2 + reg3*reg3 + reg4*reg4

#define distance_sq(x1, y1, z1, w1, x2, y2, z2, w2) \
(((x1) - (x2)) * ((x1) - (x2))) + \
(((y1) - (y2)) * ((y1) - (y2))) + \
(((z1) - (z2)) * ((z1) - (z2))) + \
(((w1) - (w2)) * ((w1) - (w2)))

void _output_centroids(long with_point_num)
{
    long i;
    long *npt;
    npt = (long *)(0x300000000 + K * 8 * 4);
    output_q(K);
    output_char(' ');
    output_char('c');
    output_char('e');
    output_char('n');
    output_char('t');
    output_char('r');
    output_char('o');
    output_char('i');
    output_char('d');
    output_char('s');
    output_char(':');
    output_char(C_n);
    for(i = 0; i < K; i = i + 1)
    {
        output_q(cx[i]);
        output_char(' ');
        output_q(cy[i]);
        output_char(' ');
        output_q(cz[i]);
        output_char(' ');
        output_q(cw[i]);
        if (with_point_num == 1) {
            output_char('/');
            output_q(npt[i]);
        }
        output_char(C_n);
    }
    output_char(C_n);

    return;
}

void output_centroids_with_point_num()
{
    _output_centroids(1);
    return;
}


void output_centroids()
{
    _output_centroids(0);
    return;
}

void kmeans_finish()
{
#ifdef OUTPUT_PMEM
    long i;

    for(i = 0; i < K; i = i + 1)
    {
        pmem_out[4*i] = cx[i];
        pmem_out[4*i+1] = cy[i];
        pmem_out[4*i+2] = cz[i];
        pmem_out[4*i+3] = cw[i];
    }
#endif

#ifdef OUTPUT_ON_FINISH
    output_centroids();
#endif
    return;
}

void on_recluster_finished(long expected_iter)
{
    long i;
    long j;
    long *tlcx;
    long *tlcy;
    long *tlcz;
    long *tlcw;
    long *tlcn;

    long n_parts;

    long *sumx;
    long *sumy;
    long *sumz;
    long *sumw;
    long *npt;

#ifdef DEBUG
    *((long *)STDOUT) = 'E';
#endif

    if (expected_iter < iter) {
        // already moved on... I am too lost behind
        abortd;
    }

    if (expected_iter > iter) {
        // not reach there where I should start work yet.
        // abort and wait
        abort;
    }
    
    for(i = 0; i < N/PSIZE * N_LONG_PAGE; i = i + N_LONG_PAGE)
    {
        if(trigger[i] != 2)
        {
            abort;
        }
    }

    // mark this watcher committed
    // trigger[0] = 3;

    sumx = (long *)(0x300000000);
    sumy = (long *)(0x300000000 + K * 8);
    sumz = (long *)(0x300000000 + K * 8 * 2);
    sumw = (long *)(0x300000000 + K * 8 * 3);
    npt = (long *)(0x300000000 + K * 8 * 4);

    for(i = 0; i < K; i = i + 1)
    {
        sumx[i] = 0;
        sumy[i] = 0;
        sumz[i] = 0;
        sumw[i] = 0;
        npt[i] = 0;
    }

    // get sum and np
    n_parts = N / PSIZE;
    for (j = 0; j < n_parts; j = j + 1) {
        tlcx = &lcx[(K+N_LONG_PAGE)*j];
        tlcy = &lcy[(K+N_LONG_PAGE)*j];
        tlcz = &lcz[(K+N_LONG_PAGE)*j];
        tlcw = &lcw[(K+N_LONG_PAGE)*j];
        tlcn = &lcn[(K+N_LONG_PAGE)*j];
	for (i = 0; i < K; i = i + 1)
	{
	    sumx[i] = sumx[i] + tlcx[i];
	    sumy[i] = sumy[i] + tlcy[i];
	    sumz[i] = sumz[i] + tlcz[i];
	    sumw[i] = sumw[i] + tlcw[i];
	    npt[i] = npt[i] + tlcn[i];
        }
    }

    for(i = 0; i < K; i = i + 1)
    {
        if (npt[i] != 0) {
            cx[i] = sumx[i] / npt[i];
            cy[i] = sumy[i] / npt[i];
            cz[i] = sumz[i] / npt[i];
            cw[i] = sumw[i] / npt[i];
        }
        else {
            // disable it
            cx[i] = 0x7FFFFFFFFFFFFFF;
            cy[i] = 0x7FFFFFFFFFFFFFF;
            cz[i] = 0x7FFFFFFFFFFFFFF;
            cw[i] = 0x7FFFFFFFFFFFFFF;
        }
    }

#ifdef PRINT_ITER
    output_char('i');
    output_char('t');
    output_char('e');
    output_char('r');
    output_char(':');
    output_q(iter);
    output_char(C_n);
#endif
 
#ifdef DEBUG
    output_centroids_with_point_num();
#endif

    // iter is a guard to avoid multiple tasks for the same
    // expected_iter commit successfully
    
    // Note: must be done before start_iteration
    iter = iter + 1;

    if (iter < MAX_ITER) {
        start_iteration();
    }
    else {
        kmeans_finish();
    }

#ifdef DEBUG
    *((long *)STDOUT) = 'T';
    output_char(C_n);
    output_q(iter);
    output_char(C_n);
#endif

    commitd;

}

void recluster_runner(long start, long end)
{
    long i, j, mini;
    long mind, d;
    long xt, yt, zt, wt;
    long partid;

    long *mylcx;
    long *mylcy;
    long *mylcz;
    long *mylcw;
    long *mylcn;


#ifdef DEBUG
    *((long *)STDOUT) = 'R';
#endif
    partid = start / PSIZE;

    mylcx = &lcx[(K+N_LONG_PAGE)*partid];
    mylcy = &lcy[(K+N_LONG_PAGE)*partid];
    mylcz = &lcz[(K+N_LONG_PAGE)*partid];
    mylcw = &lcw[(K+N_LONG_PAGE)*partid];
    mylcn = &lcn[(K+N_LONG_PAGE)*partid];

    for (i = 0; i < K; i = i + 1) {
        mylcx[i] = 0;
        mylcy[i] = 0;
        mylcz[i] = 0;
        mylcw[i] = 0;
        mylcn[i] = 0;
    }

    for(i = start; i < end; i = i  + 1)
    {
#ifdef INPUT_PMEM
        xt = pmem_in[4*i];
        yt = pmem_in[4*i+1];
        zt = pmem_in[4*i+2];
        wt = pmem_in[4*i+3];
#else
        xt = x[i];
        yt = y[i];
        zt = z[i];
        wt = w[i];
#endif
        j = 0;
        // d = distance_sq(xt, yt, zt, wt, cx[j], cy[j], cz[j], cw[j]);
        distance_sq_2(xt, yt, zt, wt, cx[j], cy[j], cz[j], cw[j], &d);

        mind = d;
        mini = j;

        for(j = 1; j < K; j = j + 1)
        {
            // d = distance_sq(xt, yt, zt, wt, cx[j], cy[j], cz[j], cw[j]);
            distance_sq_2(xt, yt, zt, wt, cx[j], cy[j], cz[j], cw[j], &d);

            if(d < mind)
            {
                mind = d;
                mini = j;
            }
        }
        mylcn[mini] = mylcn[mini] + 1;
        mylcx[mini] = mylcx[mini] + xt;
        mylcy[mini] = mylcy[mini] + yt;
        mylcz[mini] = mylcz[mini] + zt;
        mylcw[mini] = mylcw[mini] + wt;

        // if(ci[i] != mini)
        // {
        //     ci[i] = mini;
        //     // changed[start] = 1;
        // }
    }
    trigger[start/PSIZE*N_LONG_PAGE] = 2;
    commitd;
}

void start_iteration()
{
    long i;
    long tmp1;
    long tmp2;

#ifdef DEBUG
    *((long *)STDOUT) = 'S';
#endif
    for(i = 0; i < N; i = i + PSIZE)
    {
        // changed[i] = 0;
        trigger[i/PSIZE*N_LONG_PAGE] = 0;

#ifdef LOCAL_AGGREG
        tmp1 = (K+N_LONG_PAGE)*i/PSIZE;
        tmp2 = tmp1 + K + N_LONG_PAGE;
#endif

        runner 
#ifdef LOCAL_AGGREG
            recluster_runner(i, i + PSIZE) 
#else
            recluster_runner_0(i, i + PSIZE) 
#endif
            using x[i, , i + PSIZE], y[i, , i + PSIZE], z[i, , i + PSIZE], w[i, , i + PSIZE], ci[i, , i + PSIZE], cx[0, , K], cy[0, , K], cz[0, , K], cw[0, , K], trigger[i/PSIZE*N_LONG_PAGE, , (i/PSIZE+1)*N_LONG_PAGE]
            /* changed[i, , i + PSIZE], */
#ifdef OUTPUT_PMEM
        , pmem_in,  pmem_out
        , pmem_in[4*i,, 4*(i + PSIZE)]
#endif
#ifdef LOCAL_AGGREG
	// , lcx[0,, N_LOCAL_CENTROIDS], lcy[0,, N_LOCAL_CENTROIDS], lcz[0,, N_LOCAL_CENTROIDS], lcw[0,, N_LOCAL_CENTROIDS], lcn[0,, N_LOCAL_CENTROIDS]
	, lcx[tmp1,, tmp2], lcy[tmp1,, tmp2], lcz[tmp1,, tmp2], lcw[tmp1,, tmp2], lcn[tmp1,, tmp2]
#endif
	;
    }
    runner 
#ifdef LOCAL_AGGREG
        on_recluster_finished(iter)
#else
        on_recluster_finished_0() 
#endif 
        using iter, cx[0, , K], cy[0, , K], cz[0, , K], cw[0, , K]
#ifdef OUTPUT_PMEM
        , pmem_in,  pmem_out
#endif
#ifdef OUTPUT_PMEM
        , pmem_in[0,,PMEM_IN_SIZE]
        , pmem_out[0,,PMEM_OUT_SIZE]
#endif

#ifdef LOCAL_AGGREG
        , lcx[0,, N_LOCAL_CENTROIDS], lcy[0,, N_LOCAL_CENTROIDS], lcz[0,, N_LOCAL_CENTROIDS], lcw[0,, N_LOCAL_CENTROIDS], lcn[0,, N_LOCAL_CENTROIDS]
#else
        , x[0, , N], y[0, , N], z[0, , N], w[0, , N], ci[0, , N] 
#endif
        watching trigger[0, , (N/PSIZE+1)*N_LONG_PAGE]
        ;
    return;
}

void on_init1_finished()
{
    long i;
    for(i = 0; i < N/PSIZE * N_LONG_PAGE; i = i + N_LONG_PAGE)
    {
        if(trigger[i] != 1)
        {
            abort;
        }
    }

    // mark this watcher committed
    trigger[0] = 3;

    for(i = 0; i < K; i = i + 1)
    {
#ifdef INPUT_PMEM
        cx[i] = pmem_in[4*i];
        cy[i] = pmem_in[4*i+1];
        cz[i] = pmem_in[4*i+2];
        cw[i] = pmem_in[4*i+3];
#else
        cx[i] = x[i];
        cy[i] = y[i];
        cz[i] = z[i];
        cw[i] = w[i];
#endif
    }

    // number of iterations
    iter = 0;

#ifdef DEBUG
    output_char('i');
    output_char('t');
    output_char('e');
    output_char('r');
    output_char(':');

    output_q(-1);
    output_char(':');
    output_centroids();
#endif

    start_iteration();

    commitd;
}

void init_runner(long start, long end)
{
    long i;
    long t;

// #ifdef INPUT_PMEM
// /*
//     for (i = start; i < end; i = i + 1)
//     {
//         x[i] = pmem_in[4*i];
//         y[i] = pmem_in[4*i+1];
//         z[i] = pmem_in[4*i+2];
//         w[i] = pmem_in[4*i+3];
// #ifdef DEBUG_PRINT_IN
//         output_char('P');
//         output_q(i);
//         output_char('@');
//         output_q_hex((long)&pmem_in[4*i]);
//         output_char(':');
//         output_q(x[i]);
//         output_char('#');
//         output_q(pmem_in[4*i]);
//         output_char(' ');
//         output_q(y[i]);
//         output_char(' ');
//         output_q(z[i]);
//         output_char(' ');
//         output_q(w[i]);
//         output_char(C_n);
// #endif
//     }
// */
// #else
    // default: generate randomly
    // srand(*(long*)(0x200000008));
    srand(1);

#ifdef INPUT_STDIN
    for(i = 0; i < start; i = i + 1)
    {
	    t = input_q();
	    t = input_q();
	    t = input_q();
	    t = input_q();
    }
#endif

    for(i = start; i < end; i = i + 1)
    {
#ifdef INPUT_STDIN
        x[i] = input_q();
#ifdef DEBUG
        output_q(x[i]);
        output_char(' ');
#endif
        y[i] = input_q();
#ifdef DEBUG
        output_q(y[i]);
        output_char(' ');
#endif
        z[i] = input_q();
#ifdef DEBUG
        output_q(z[i]);
        output_char(' ');
#endif
        w[i] = input_q();
#ifdef DEBUG
        output_q(w[i]);
        output_char(C_n);
#endif
#else // INPUT_STDIN
        x[i] = (long)rand() / (long)0x100000000;
        y[i] = (long)rand() / (long)0x100000000;
        z[i] = (long)rand() / (long)0x100000000;
        w[i] = (long)rand() / (long)0x100000000;
#endif // INPUT_STDIN
    }
// #endif
    trigger[start/PSIZE*N_LONG_PAGE] = 1;
    commit;
}

void init_runner_stdin(long start, long end)
{
    long i;
    long t;

    for(i = 0; i < start; i = i + 1)
    {
	    t = input_q();
	    t = input_q();
	    t = input_q();
	    t = input_q();
    }

    for(i = start; i < end; i = i + 1)
    {
        x[i] = input_q();
#ifdef DEBUG
        output_q(x[i]);
        output_char(' ');
#endif
        y[i] = input_q();
#ifdef DEBUG
        output_q(y[i]);
        output_char(' ');
#endif
        z[i] = input_q();
#ifdef DEBUG
        output_q(z[i]);
        output_char(' ');
#endif
        w[i] = input_q();
#ifdef DEBUG
        output_q(w[i]);
        output_char(C_n);
#endif
    }

    for(i = start; i < end; i = i + 1) {
        trigger[i/PSIZE*N_LONG_PAGE] = 1;
    }

    commit;
}


void init0()
{
    long i;
#ifdef INPUT_PMEM
    pmem_in = (long*)PMEM_IN_ADDR;
    pmem_out = (long*)PMEM_OUT_ADDR;
#endif
    for(i = 0; i < N; i = i + PSIZE)
    {
        trigger[i/PSIZE*N_LONG_PAGE] = 0;
        runner init_runner(i, i + PSIZE) 
            using x[i, , i + PSIZE], y[i, , i + PSIZE], z[i, , i + PSIZE], w[i, , i + PSIZE], trigger[i/PSIZE*N_LONG_PAGE, , (i/PSIZE+1)*N_LONG_PAGE]
#ifdef INPUT_PMEM
            , pmem_in
#endif
	;
    }

    runner on_init1_finished() 
        using cx[0, , K], cy[0, , K], cz[0, , K], cw[0, , K], x[0, , K], y[0, , K], z[0, , K], w[0, , K], /* changed[0, , N], */ trigger[0, , N], iter
#ifdef INPUT_PMEM
        , pmem_in, pmem_out
        , pmem_in[0,,PMEM_IN_SIZE]
        , pmem_out[0,,PMEM_OUT_SIZE]
#endif
        watching trigger[0, , (N/PSIZE+1)*N_LONG_PAGE];

    commit;
}

