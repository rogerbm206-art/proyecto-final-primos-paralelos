@echo off
REM Compila las tres versiones con MSVC (ejecutar desde
REM "x64 Native Tools Command Prompt for VS").
REM /openmp:llvm es necesario porque omp_set_schedule es OpenMP 3.0.
if not exist bin mkdir bin
cl /O2 /nologo /Fe:bin\primes_seq.exe src\primes_seq.c
cl /O2 /nologo /openmp:llvm /Fe:bin\primes_omp.exe src\primes_omp.c
cl /O2 /nologo /I"%MSMPI_INC%" /Fe:bin\primes_mpi.exe src\primes_mpi.c /link /LIBPATH:"%MSMPI_LIB64%" msmpi.lib
del *.obj 2>nul
