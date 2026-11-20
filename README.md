# Conteo paralelo de números primos con OpenMP y MPI

Proyecto Final — PCIA5009 Computación Paralela y Distribuida, Universidad Sergio Arboleda.
Autor: Roger Adrian Bohorquez Moreno.

Este proyecto cuenta los números primos del intervalo [2, N] usando división por tentativa.
Como el costo de probar un número n crece con √n, el trabajo no se reparte de forma uniforme a lo
largo del intervalo, así que el problema sirve para estudiar el balance de carga. Hay tres versiones:

| Archivo | Versión | Reparto del trabajo |
|---|---|---|
| `src/primes_seq.c` | Secuencial (línea base) | — |
| `src/primes_omp.c` | OpenMP (memoria compartida) | `schedule(static)` en bloques contiguos, `static,chunk` (bloques en ronda) o `dynamic,chunk` / `guided` (bajo demanda) |
| `src/primes_mpi.c` | MPI (paso de mensajes) | `block` (bloques contiguos) o `cyclic` (impares repartidos en ronda) + `MPI_Reduce` |
| `src/primes_common.h` | Código compartido | `is_prime`, estructura del resultado y reloj de alta resolución |

Cada versión devuelve la cantidad de primos, su suma (que sirve de checksum) y el mayor primo.
Con esos tres valores se comprueba que todas las versiones dan el mismo resultado.

## Dependencias

- Windows 10/11 (también funciona en Linux con gcc + OpenMPI/MPICH).
- GCC MinGW-w64 con soporte OpenMP (probado con GCC 16.1.0, WinLibs UCRT), o MSVC.
- Microsoft MPI: el runtime (`mpiexec`) y el SDK (`mpi.h`, `msmpi.lib`), versión 10.1.
- Git Bash para los scripts `.sh`.
- Python 3 con `matplotlib` para el análisis (probado con Python 3.13 y matplotlib 3.10).

## Compilar

Con GCC (Git Bash):

```bash
./build.sh
```

Con MSVC (desde "x64 Native Tools Command Prompt for VS"):

```bat
build_msvc.bat
```

Los ejecutables quedan en `bin/`.

## Ejecutar

```bash
# Secuencial
./bin/primes_seq.exe 1e7

# OpenMP: N, hilos, planificación, chunk
./bin/primes_omp.exe 1e7 4 static
./bin/primes_omp.exe 1e7 4 dynamic 1000

# MPI: procesos, N, distribución
mpiexec -n 4 ./bin/primes_mpi.exe 1e7 block
mpiexec -n 4 ./bin/primes_mpi.exe 1e7 cyclic
```

Si se agrega `--csv` al final, el programa imprime una sola línea CSV (es lo que usan los scripts).

## Reproducir los experimentos

```bash
./scripts/verify.sh            # verifica que las tres versiones den el mismo resultado
./scripts/run_benchmarks.sh 5  # protocolo completo con 5 repeticiones -> results/raw.csv
# (los datos entregados se tomaron en dos lotes: run_benchmarks.sh, que en ese momento
#  corría static,1000 y dynamic,1000, y luego run_static_block.sh para static sin chunk)
python scripts/analyze.py      # summary.csv, figuras y tablas
```

## Estructura

```
src/                 código fuente en C
bin/                 ejecutables (se generan al compilar)
scripts/verify.sh    prueba de correctitud
scripts/run_benchmarks.sh  protocolo experimental
scripts/run_static_block.sh  segundo lote (OpenMP static en bloques contiguos)
scripts/analyze.py   cálculo de métricas, tablas y gráficas
results/raw.csv      datos crudos (una fila por ejecución)
results/summary.csv  medianas, speedup, eficiencia, overhead, Karp-Flatt, desbalance
results/figures/     gráficas (PDF)
results/tables/      tablas LaTeX generadas a partir de los datos
results/environment.txt  hardware y software usados
docs/Informe_Proyecto_Final.pdf  informe técnico
docs/Presentacion_Proyecto_Final.pdf  diapositivas para la sustentación
```

## Columnas de `results/raw.csv`

`version,N,p,mode,rep,time,count,sum,largest,tmax,tmin,tavg,tcomm`

- `time`: tiempo total de pared en segundos.
- `tmax`, `tmin`, `tavg`: tiempo de cómputo del hilo o proceso más lento, del más rápido y el promedio.
- `tcomm`: solo para MPI. Es el tiempo de comunicación más la espera en `MPI_Bcast` y `MPI_Reduce`, tomado del proceso con el valor más alto.
