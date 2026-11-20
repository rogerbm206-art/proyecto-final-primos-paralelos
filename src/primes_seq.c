/*
 * primes_seq.c - Version secuencial (linea base)
 * Proyecto Final PCIA5009 - Computacion Paralela y Distribuida
 * Autor: Roger Adrian Bohorquez Moreno
 *
 * Uso:   primes_seq N [--csv]
 * Salida normal: cantidad de primos, suma, mayor primo y tiempo.
 * Con --csv imprime una sola linea:
 *   version,N,p,modo,tiempo_total,count,sum,largest
 */
#include "primes_common.h"

int main(int argc, char **argv)
{
    long long N, n;
    int csv = 0;
    prime_result r;
    double t0, t1;

    if (argc < 2) {
        fprintf(stderr, "Uso: %s N [--csv]\n", argv[0]);
        return 1;
    }
    N = parse_n(argv[1]);
    if (argc > 2 && strcmp(argv[2], "--csv") == 0) csv = 1;

    result_init(&r);

    t0 = wall_time();
    /* Recorrido lineal del intervalo [2, N]. */
    for (n = 2; n <= N; n++) {
        if (is_prime(n)) result_add(&r, n);
    }
    t1 = wall_time();

    if (csv) {
        printf("seq,%lld,1,-,%.6f,%lld,%llu,%lld\n",
               N, t1 - t0, r.count, r.sum, r.largest);
    } else {
        printf("Version secuencial\n");
        printf("N               = %lld\n", N);
        printf("Primos          = %lld\n", r.count);
        printf("Suma de primos  = %llu\n", r.sum);
        printf("Mayor primo     = %lld\n", r.largest);
        printf("Tiempo (s)      = %.6f\n", t1 - t0);
    }
    return 0;
}
