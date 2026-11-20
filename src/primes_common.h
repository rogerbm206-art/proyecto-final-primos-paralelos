/*
 * primes_common.h
 * Proyecto Final PCIA5009 - Computacion Paralela y Distribuida
 * Autor: Roger Adrian Bohorquez Moreno
 *
 * Funciones compartidas por las tres versiones (secuencial, OpenMP y MPI)
 * del conteo de numeros primos en el intervalo [2, N].
 *
 * Se usa division por tentativa (trial division) a proposito: el costo de
 * probar un numero n crece aproximadamente como sqrt(n), por lo que el
 * trabajo NO es uniforme a lo largo del intervalo. Esto convierte al
 * problema en un buen caso de estudio de balance de carga.
 */
#ifndef PRIMES_COMMON_H
#define PRIMES_COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
/* Reloj de pared de alta resolucion, igual para las tres versiones.
 * (omp_get_wtime en MinGW solo tiene resolucion de milisegundos). */
static inline double wall_time(void)
{
    LARGE_INTEGER f, c;
    QueryPerformanceFrequency(&f);
    QueryPerformanceCounter(&c);
    return (double)c.QuadPart / (double)f.QuadPart;
}
#else
#include <time.h>
static inline double wall_time(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}
#endif

/* Resultado agregado del conteo. Los tres campos permiten verificar la
 * correctitud entre versiones: cantidad de primos, suma de todos ellos
 * (funciona como checksum) y el mayor primo encontrado. */
typedef struct {
    long long count;              /* cantidad de primos en [2, N]   */
    unsigned long long sum;       /* suma de los primos (checksum)  */
    long long largest;            /* mayor primo <= N               */
} prime_result;

/* Devuelve 1 si n es primo, 0 en caso contrario.
 * Prueba divisores impares d con d*d <= n. */
static int is_prime(long long n)
{
    long long d;
    if (n < 2) return 0;
    if (n < 4) return 1;          /* 2 y 3 */
    if ((n & 1) == 0) return 0;   /* pares mayores que 2 */
    for (d = 3; d * d <= n; d += 2) {
        if (n % d == 0) return 0;
    }
    return 1;
}

/* Inicializa un resultado vacio. */
static void result_init(prime_result *r)
{
    r->count = 0;
    r->sum = 0ULL;
    r->largest = 0;
}

/* Agrega un primo p al resultado parcial. */
static void result_add(prime_result *r, long long p)
{
    r->count++;
    r->sum += (unsigned long long)p;
    if (p > r->largest) r->largest = p;
}

/* Lee N desde la linea de comandos; acepta notacion como 1e7. */
static long long parse_n(const char *s)
{
    double v = atof(s);
    if (v < 2.0) {
        fprintf(stderr, "N debe ser >= 2\n");
        exit(1);
    }
    return (long long)(v + 0.5);
}

#endif /* PRIMES_COMMON_H */
