/*
 * primes_omp.c - Version paralela con OpenMP (memoria compartida)
 * Proyecto Final PCIA5009 - Computacion Paralela y Distribuida
 * Autor: Roger Adrian Bohorquez Moreno
 *
 * Uso:   primes_omp N hilos [static|dynamic|guided] [chunk] [--csv]
 *
 * Estrategia:
 *  - El intervalo [2, N] se reparte entre hilos con un "omp for".
 *  - La politica de planificacion (schedule) es configurable para estudiar
 *    el balance de carga: con "static" cada hilo recibe un bloque contiguo
 *    y los ultimos bloques (numeros grandes) cuestan mas; con "dynamic" los
 *    hilos toman trozos de tamano "chunk" bajo demanda.
 *  - Cada hilo acumula un resultado parcial privado (sin sincronizacion
 *    dentro del bucle) y al final lo combina en una seccion critica.
 *    Se usa critical en lugar de reduction(max:...) para mantener
 *    compatibilidad con OpenMP 2.0 de MSVC.
 *  - Se mide el tiempo de computo de cada hilo para cuantificar el
 *    desbalance (max/min/promedio).
 */
#include "primes_common.h"
#include <omp.h>

#define MAX_THREADS 256

int main(int argc, char **argv)
{
    long long N;
    int threads, chunk = 0, csv = 0, i, nt = 0;
    const char *sched = "static";
    prime_result total;
    double t0, t1, t_thr[MAX_THREADS];
    double tmax = 0.0, tmin = 1e30, tavg = 0.0;

    if (argc < 3) {
        fprintf(stderr,
                "Uso: %s N hilos [static|dynamic|guided] [chunk] [--csv]\n",
                argv[0]);
        return 1;
    }
    N = parse_n(argv[1]);
    threads = atoi(argv[2]);
    if (threads < 1 || threads > MAX_THREADS) {
        fprintf(stderr, "hilos debe estar entre 1 y %d\n", MAX_THREADS);
        return 1;
    }
    for (i = 3; i < argc; i++) {
        if (strcmp(argv[i], "--csv") == 0) csv = 1;
        else if (strcmp(argv[i], "static") == 0 ||
                 strcmp(argv[i], "dynamic") == 0 ||
                 strcmp(argv[i], "guided") == 0) sched = argv[i];
        else chunk = atoi(argv[i]);
    }

    /* La politica se fija en tiempo de ejecucion con schedule(runtime). */
    {
        omp_sched_t kind = omp_sched_static;
        if (strcmp(sched, "dynamic") == 0) kind = omp_sched_dynamic;
        else if (strcmp(sched, "guided") == 0) kind = omp_sched_guided;
        omp_set_schedule(kind, chunk); /* chunk<=0 -> valor por defecto */
    }
    omp_set_num_threads(threads);
    for (i = 0; i < MAX_THREADS; i++) t_thr[i] = 0.0;
    result_init(&total);

    t0 = wall_time();   /* incluye creacion del equipo de hilos */
#pragma omp parallel
    {
        prime_result local;          /* resultado privado del hilo */
        int id = omp_get_thread_num();
        int n;                       /* int con signo: OpenMP 2.0 */
        int last = (int)N;
        double ts = wall_time();

        result_init(&local);
#pragma omp single
        nt = omp_get_num_threads();

        /* nowait: cada hilo registra su tiempo apenas termina su parte,
         * asi se observa el desbalance antes de la barrera final. */
#pragma omp for schedule(runtime) nowait
        for (n = 2; n <= last; n++) {
            if (is_prime(n)) result_add(&local, n);
        }
        t_thr[id] = wall_time() - ts;

        /* Combinacion de resultados parciales: una entrada por hilo. */
#pragma omp critical
        {
            total.count += local.count;
            total.sum += local.sum;
            if (local.largest > total.largest) total.largest = local.largest;
        }
    }
    t1 = wall_time();

    for (i = 0; i < nt; i++) {
        if (t_thr[i] > tmax) tmax = t_thr[i];
        if (t_thr[i] < tmin) tmin = t_thr[i];
        tavg += t_thr[i];
    }
    tavg /= nt;

    if (csv) {
        /* version,N,p,modo,tiempo_total,count,sum,largest,tmax,tmin,tavg
         * modo = "static" (bloques contiguos) o "<schedule>-<chunk>". */
        char mode[64];
        if (chunk > 0) snprintf(mode, sizeof mode, "%s-%d", sched, chunk);
        else snprintf(mode, sizeof mode, "%s", sched);
        printf("omp,%lld,%d,%s,%.6f,%lld,%llu,%lld,%.6f,%.6f,%.6f\n",
               N, nt, mode, t1 - t0, total.count, total.sum, total.largest,
               tmax, tmin, tavg);
    } else {
        printf("Version OpenMP (schedule=%s, chunk=%d)\n", sched, chunk);
        printf("N               = %lld\n", N);
        printf("Hilos           = %d\n", nt);
        printf("Primos          = %lld\n", total.count);
        printf("Suma de primos  = %llu\n", total.sum);
        printf("Mayor primo     = %lld\n", total.largest);
        printf("Tiempo (s)      = %.6f\n", t1 - t0);
        for (i = 0; i < nt; i++)
            printf("  hilo %2d: %.6f s\n", i, t_thr[i]);
        printf("Desbalance (tmax/tavg) = %.3f\n", tavg > 0 ? tmax / tavg : 0);
    }
    return 0;
}
