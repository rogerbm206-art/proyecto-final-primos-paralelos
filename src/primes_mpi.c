/*
 * primes_mpi.c - Version paralela con MPI (paso de mensajes)
 * Proyecto Final PCIA5009 - Computacion Paralela y Distribuida
 * Autor: Roger Adrian Bohorquez Moreno
 *
 * Uso:   mpiexec -n P primes_mpi N [block|cyclic] [--csv]
 *
 * Estrategia:
 *  - Cada proceso tiene su propio espacio de memoria; no se comparte nada.
 *  - El proceso 0 lee N y lo difunde con MPI_Bcast (aqui todos lo reciben
 *    por argv, pero el Bcast deja explicita la fase de distribucion).
 *  - Distribucion del dominio:
 *      block : el proceso r procesa un bloque contiguo [lo_r, hi_r].
 *              Es simple pero desbalanceado, porque los numeros grandes
 *              (al final del intervalo) son mas costosos de probar.
 *      cyclic: los candidatos impares 3, 5, 7, ... se reparten en ronda:
 *              el proceso r toma 3 + 2r, 3 + 2r + 2P, ... (el 2 lo cuenta
 *              el proceso 0). Cada proceso recibe numeros de todo el
 *              rango, de modo que el costo total queda casi igual.
 *  - Combinacion: MPI_Reduce con MPI_SUM (cantidad y suma) y MPI_MAX
 *    (mayor primo) hacia el proceso 0.
 *  - Se mide por separado el tiempo de computo local de cada proceso y el
 *    tiempo de comunicacion (Bcast + Reduce) para estimar el overhead.
 */
#include "primes_common.h"
#include <mpi.h>

int main(int argc, char **argv)
{
    int rank, size, i, csv = 0;
    const char *dist = "block";
    long long N = 0, n, lo, hi, per, rem;
    prime_result local;
    long long g_count = 0, g_largest = 0;
    unsigned long long g_sum = 0ULL;
    double t0, t1, tc0, tc1, tcomp, tcomm0, tcomm = 0.0, ttotal;
    double tmax, tmin, tsum, commmax;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (argc < 2) {
        if (rank == 0)
            fprintf(stderr, "Uso: mpiexec -n P %s N [block|cyclic] [--csv]\n",
                    argv[0]);
        MPI_Finalize();
        return 1;
    }
    for (i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--csv") == 0) csv = 1;
        else if (strcmp(argv[i], "cyclic") == 0) dist = "cyclic";
        else if (strcmp(argv[i], "block") == 0) dist = "block";
    }

    /* Sincronizar antes de medir para que todos arranquen juntos. */
    MPI_Barrier(MPI_COMM_WORLD);
    t0 = MPI_Wtime();

    /* Fase 1: distribucion del parametro del problema. */
    tcomm0 = MPI_Wtime();
    if (rank == 0) N = parse_n(argv[1]);
    MPI_Bcast(&N, 1, MPI_LONG_LONG, 0, MPI_COMM_WORLD);
    tcomm += MPI_Wtime() - tcomm0;

    /* Fase 2: computo local, sin comunicacion. */
    result_init(&local);
    tc0 = MPI_Wtime();
    if (strcmp(dist, "block") == 0) {
        /* Reparto de M = N-1 numeros en bloques casi iguales; los primeros
         * 'rem' procesos reciben un elemento extra. */
        long long M = N - 1;
        per = M / size;
        rem = M % size;
        lo = 2 + rank * per + (rank < rem ? rank : rem);
        hi = lo + per - 1 + (rank < rem ? 1 : 0);
        for (n = lo; n <= hi; n++)
            if (is_prime(n)) result_add(&local, n);
    } else {
        /* Ciclico sobre los candidatos IMPARES. Un reparto ciclico ingenuo
         * (n = 2 + rank, paso size) con size par deja a la mitad de los
         * procesos solo con numeros pares, que se descartan en O(1): el
         * desbalance resulta peor que en bloques. Por eso el 2 lo cuenta el
         * proceso 0 y los impares 3, 5, 7, ... se reparten en ronda. */
        if (rank == 0 && N >= 2) result_add(&local, 2);
        for (n = 3 + 2LL * rank; n <= N; n += 2LL * size)
            if (is_prime(n)) result_add(&local, n);
    }
    tc1 = MPI_Wtime();
    tcomp = tc1 - tc0;

    /* Fase 3: combinacion de resultados parciales en el proceso 0.
     * El tiempo de Reduce incluye la espera por el proceso mas lento,
     * por eso refleja tanto comunicacion como desbalance. */
    tcomm0 = MPI_Wtime();
    MPI_Reduce(&local.count, &g_count, 1, MPI_LONG_LONG, MPI_SUM, 0,
               MPI_COMM_WORLD);
    MPI_Reduce(&local.sum, &g_sum, 1, MPI_UNSIGNED_LONG_LONG, MPI_SUM, 0,
               MPI_COMM_WORLD);
    MPI_Reduce(&local.largest, &g_largest, 1, MPI_LONG_LONG, MPI_MAX, 0,
               MPI_COMM_WORLD);
    tcomm += MPI_Wtime() - tcomm0;
    t1 = MPI_Wtime();
    ttotal = t1 - t0;

    /* Estadisticas de tiempos por proceso (fuera de la medicion). */
    MPI_Reduce(&tcomp, &tmax, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
    MPI_Reduce(&tcomp, &tmin, 1, MPI_DOUBLE, MPI_MIN, 0, MPI_COMM_WORLD);
    MPI_Reduce(&tcomp, &tsum, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
    MPI_Reduce(&tcomm, &commmax, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        double tavg = tsum / size;
        if (csv) {
            /* version,N,p,modo,tiempo_total,count,sum,largest,
             * tmax,tmin,tavg,tcomm_max */
            printf("mpi,%lld,%d,%s,%.6f,%lld,%llu,%lld,%.6f,%.6f,%.6f,%.6f\n",
                   N, size, dist, ttotal, g_count, g_sum, g_largest,
                   tmax, tmin, tavg, commmax);
        } else {
            printf("Version MPI (distribucion=%s)\n", dist);
            printf("N               = %lld\n", N);
            printf("Procesos        = %d\n", size);
            printf("Primos          = %lld\n", g_count);
            printf("Suma de primos  = %llu\n", g_sum);
            printf("Mayor primo     = %lld\n", g_largest);
            printf("Tiempo (s)      = %.6f\n", ttotal);
            printf("Computo max/min/prom (s) = %.6f / %.6f / %.6f\n",
                   tmax, tmin, tavg);
            printf("Comunicacion+espera max (s) = %.6f\n", commmax);
            printf("Desbalance (tmax/tavg) = %.3f\n",
                   tavg > 0 ? tmax / tavg : 0);
        }
    }
    MPI_Finalize();
    return 0;
}
