#!/usr/bin/env bash
# Ejecuta el protocolo experimental completo y guarda los datos crudos en
# results/raw.csv (una fila por ejecucion).
#   - Tamanos N: 2.5e6, 5e6, 1e7, 2e7
#   - Recursos p: 1, 2, 4, 8 (hilos OpenMP / procesos MPI)
#   - OpenMP: static (bloques contiguos), static,1000 y dynamic,1000
#   - MPI: distribucion block y cyclic
#   - REPS repeticiones por configuracion (por defecto 5); en cada
#     repeticion se recorren todas las configuraciones para repartir
#     efectos termicos / de turbo entre ellas.
# Uso: ./scripts/run_benchmarks.sh [REPS]
cd "$(dirname "$0")/.."
REPS=${1:-5}
SIZES="2500000 5000000 10000000 20000000"
PS="1 2 4 8"
OUT=results/raw.csv
echo "version,N,p,mode,rep,time,count,sum,largest,tmax,tmin,tavg,tcomm" > $OUT
for rep in $(seq 1 $REPS); do
  for N in $SIZES; do
    l=$(./bin/primes_seq.exe $N --csv | tr -d '\r')
    IFS=, read -r v n p m t c s lg <<< "$l"
    echo "$v,$n,$p,$m,$rep,$t,$c,$s,$lg,$t,$t,$t,0" >> $OUT
    for p in $PS; do
      # static sin chunk = bloques contiguos; static 1000 = bloques de 1000
      # repartidos en ronda; dynamic 1000 = bloques de 1000 bajo demanda.
      for cfg in "static" "static 1000" "dynamic 1000"; do
        l=$(./bin/primes_omp.exe $N $p $cfg --csv | tr -d '\r')
        IFS=, read -r v n pp m t c s lg tmax tmin tavg <<< "$l"
        echo "$v,$n,$pp,$m,$rep,$t,$c,$s,$lg,$tmax,$tmin,$tavg,0" >> $OUT
      done
      for d in block cyclic; do
        l=$(mpiexec -n $p ./bin/primes_mpi.exe $N $d --csv | tr -d '\r')
        IFS=, read -r v n pp m t c s lg tmax tmin tavg tcomm <<< "$l"
        echo "$v,$n,$pp,$m,$rep,$t,$c,$s,$lg,$tmax,$tmin,$tavg,$tcomm" >> $OUT
      done
    done
    echo "rep $rep N=$N listo"
  done
done
echo "Datos crudos en $OUT"
