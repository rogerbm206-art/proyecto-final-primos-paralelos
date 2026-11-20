#!/usr/bin/env bash
# Segundo lote: OpenMP schedule(static) sin chunk (bloques contiguos).
# Se agrego despues de detectar que en el primer lote "static" se habia
# ejecutado con chunk=1000. Agrega filas a results/raw.csv.
cd "$(dirname "$0")/.."
REPS=${1:-5}
OUT=results/raw.csv
for rep in $(seq 1 $REPS); do
  for N in 2500000 5000000 10000000 20000000; do
    for p in 1 2 4 8; do
      l=$(./bin/primes_omp.exe $N $p static --csv | tr -d '\r')
      IFS=, read -r v n pp m t c s lg tmax tmin tavg <<< "$l"
      echo "$v,$n,$pp,$m,$rep,$t,$c,$s,$lg,$tmax,$tmin,$tavg,0" >> $OUT
    done
  done
  echo "rep $rep listo"
done
