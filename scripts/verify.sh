#!/usr/bin/env bash
# Verifica que las tres versiones producen el mismo resultado
# (cantidad, suma y mayor primo) para varios N y numeros de hilos/procesos.
cd "$(dirname "$0")/.."
fail=0
for N in 2 3 10 100 1000 99991 1000000; do
  ref=$(./bin/primes_seq.exe $N --csv | cut -d, -f6-8)
  for p in 1 2 3 4 8; do
    for s in static dynamic guided; do
      r=$(./bin/primes_omp.exe $N $p $s 1000 --csv | cut -d, -f6-8)
      [ "$r" == "$ref" ] || { echo "FALLO omp N=$N p=$p $s: $r != $ref"; fail=1; }
    done
    for d in block cyclic; do
      r=$(mpiexec -n $p ./bin/primes_mpi.exe $N $d --csv | tr -d '\r' | cut -d, -f6-8)
      [ "$r" == "$ref" ] || { echo "FALLO mpi N=$N p=$p $d: $r != $ref"; fail=1; }
    done
  done
  echo "N=$N -> (count,sum,largest) = $ref  OK"
done
[ $fail == 0 ] && echo "Todas las versiones coinciden." || exit 1
