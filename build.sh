#!/usr/bin/env bash
# Compila las tres versiones con GCC (MinGW-w64) y MS-MPI.
# Uso: ./build.sh
set -e
MPI_SDK="${MSMPI_SDK:-/c/Program Files (x86)/Microsoft SDKs/MPI}"
mkdir -p bin
gcc -O2 -Wall -o bin/primes_seq.exe src/primes_seq.c
gcc -O2 -Wall -fopenmp -o bin/primes_omp.exe src/primes_omp.c
gcc -O2 -Wall -I"$MPI_SDK/Include" -o bin/primes_mpi.exe src/primes_mpi.c \
    -L"$MPI_SDK/Lib/x64" -lmsmpi
echo "Compilado en bin/"
