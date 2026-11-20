"""
analyze.py - Procesa results/raw.csv y genera:
  - results/summary.csv     : mediana, desviacion, speedup, eficiencia,
                              overhead, Karp-Flatt y desbalance por config.
  - results/figures/*.pdf   : graficas para el informe.
  - results/tables/*.tex    : tablas LaTeX generadas a partir de los datos
                              (evita errores de transcripcion manual).
Proyecto Final PCIA5009 - Roger Adrian Bohorquez Moreno

Uso: python scripts/analyze.py
Requiere: Python 3 y matplotlib.
"""
import csv
import os
import statistics as st
from collections import defaultdict

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
RAW = os.path.join(ROOT, "results", "raw.csv")
FIG = os.path.join(ROOT, "results", "figures")
TAB = os.path.join(ROOT, "results", "tables")
os.makedirs(FIG, exist_ok=True)
os.makedirs(TAB, exist_ok=True)

LABEL = {
    ("omp", "static"): "OpenMP static",
    ("omp", "static-1000"): "OpenMP static,1000",
    ("omp", "dynamic-1000"): "OpenMP dynamic,1000",
    ("mpi", "block"): "MPI block",
    ("mpi", "cyclic"): "MPI cyclic",
}
CONFIGS = list(LABEL.keys())
COLORS = {
    ("omp", "static"): "#c6dbef",
    ("omp", "static-1000"): "#6baed6",
    ("omp", "dynamic-1000"): "#08519c",
    ("mpi", "block"): "#fdae6b",
    ("mpi", "cyclic"): "#d94801",
}

# ---------------------------------------------------------------- lectura
rows = list(csv.DictReader(open(RAW, newline="")))
groups = defaultdict(list)
results = {}  # (N) -> set of (count,sum,largest) para verificar correctitud
for r in rows:
    key = (r["version"], int(r["N"]), int(r["p"]), r["mode"])
    groups[key].append(r)
    results.setdefault(int(r["N"]), set()).add(
        (r["count"], r["sum"], r["largest"]))

for N, s in results.items():
    assert len(s) == 1, f"Resultados distintos para N={N}: {s}"

SIZES = sorted({k[1] for k in groups})
PS = sorted({k[2] for k in groups if k[0] != "seq"})


def med(key, field="time"):
    return st.median(float(r[field]) for r in groups[key])


def sd(key, field="time"):
    v = [float(r[field]) for r in groups[key]]
    return st.stdev(v) if len(v) > 1 else 0.0


T1 = {N: med(("seq", N, 1, "-")) for N in SIZES}

# --------------------------------------------------------------- resumen
summary = []
for N in SIZES:
    summary.append(dict(version="seq", mode="-", N=N, p=1, reps=len(groups[("seq", N, 1, "-")]),
                        t_med=T1[N], t_sd=sd(("seq", N, 1, "-")), speedup=1.0,
                        eff=1.0, overhead=0.0, karp_flatt="", imbalance=1.0,
                        work_ratio=1.0, tcomm=0.0))
    for (v, m) in CONFIGS:
        for p in PS:
            k = (v, N, p, m)
            tp = med(k)
            S = T1[N] / tp
            E = S / p
            ovh = tp - T1[N] / p          # tiempo extra frente al ideal
            kf = ((1 / S - 1 / p) / (1 - 1 / p)) if p > 1 else None
            imb = st.median(float(r["tmax"]) / float(r["tavg"]) for r in groups[k])
            # Inflacion del trabajo: suma de tiempos de computo de todas las
            # unidades dividida por T1. Si fuera 1, cada hilo/proceso
            # avanzaria igual de rapido que el programa secuencial.
            wr = p * med(k, "tavg") / T1[N]
            summary.append(dict(version=v, mode=m, N=N, p=p, reps=len(groups[k]),
                                t_med=tp, t_sd=sd(k), speedup=S, eff=E,
                                overhead=ovh,
                                karp_flatt="" if kf is None else kf,
                                imbalance=imb, work_ratio=wr,
                                tcomm=med(k, "tcomm")))

with open(os.path.join(ROOT, "results", "summary.csv"), "w", newline="") as f:
    w = csv.DictWriter(f, fieldnames=list(summary[0].keys()))
    w.writeheader()
    for s in summary:
        w.writerow({k: (f"{v:.6f}" if isinstance(v, float) else v) for k, v in s.items()})

S = {(s["version"], s["mode"], s["N"], s["p"]): s for s in summary}


def fmtN(N):
    return f"{N/1e6:g}" + r"\,M"


# ---------------------------------------------------------------- tablas
def write(name, text):
    with open(os.path.join(TAB, name), "w", encoding="utf-8") as f:
        f.write(text)


# Tabla de correctitud
lines = [r"\begin{tabular}{rrrr}", r"\toprule",
         r"$N$ & Primos & Suma de primos & Mayor primo \\", r"\midrule"]
for N in SIZES:
    c, s_, lg = next(iter(results[N]))
    lines.append(f"{N:,} & {int(c):,} & {int(s_):,} & {int(lg):,} \\\\".replace(",", "."))
lines += [r"\bottomrule", r"\end{tabular}"]
write("correctness.tex", "\n".join(lines))

# Tabla de tiempos (mediana) por N y p
for N in SIZES:
    lines = [r"\begin{tabular}{lrrrr}", r"\toprule",
             r"Configuraci\'on & " + " & ".join(f"$p={p}$" for p in PS) + r" \\",
             r"\midrule"]
    for c in CONFIGS:
        vals = " & ".join(f"{S[(c[0], c[1], N, p)]['t_med']:.3f}" for p in PS)
        lines.append(f"{LABEL[c]} & {vals} \\\\")
    lines += [r"\bottomrule", r"\end{tabular}"]
    write(f"times_{N}.tex", "\n".join(lines))

# Tabla general de tiempos (todas las N)
lines = [r"\begin{tabular}{llrrrrr}", r"\toprule",
         r"$N$ & Configuraci\'on & $T_1$ secuencial & " +
         " & ".join(f"$T_{{{p}}}$" for p in PS) + r" \\", r"\midrule"]
for N in SIZES:
    first = True
    for c in CONFIGS:
        vals = " & ".join(f"{S[(c[0], c[1], N, p)]['t_med']:.3f}" for p in PS)
        nlab = fmtN(N) if first else ""
        tlab = f"{T1[N]:.3f}" if first else ""
        lines.append(f"{nlab} & {LABEL[c]} & {tlab} & {vals} \\\\")
        first = False
    if N != SIZES[-1]:
        lines.append(r"\midrule")
lines += [r"\bottomrule", r"\end{tabular}"]
write("times_all.tex", "\n".join(lines))

# Tabla speedup / eficiencia
lines = [r"\begin{tabular}{ll" + "rr" * len(PS) + "}", r"\toprule",
         r"$N$ & Configuraci\'on & " +
         " & ".join(f"\\multicolumn{{2}}{{c}}{{$p={p}$}}" for p in PS) + r" \\",
         " & & " + " & ".join(["$S$ & $E$"] * len(PS)) + r" \\", r"\midrule"]
for N in SIZES:
    first = True
    for c in CONFIGS:
        vals = " & ".join(f"{S[(c[0], c[1], N, p)]['speedup']:.2f} & {S[(c[0], c[1], N, p)]['eff']:.2f}"
                          for p in PS)
        lines.append(f"{fmtN(N) if first else ''} & {LABEL[c]} & {vals} \\\\")
        first = False
    if N != SIZES[-1]:
        lines.append(r"\midrule")
lines += [r"\bottomrule", r"\end{tabular}"]
write("speedup_eff.tex", "\n".join(lines))

# Tabla de desbalance y overhead para el N mayor
NB = SIZES[-1]
lines = [r"\begin{tabular}{lrrrrrr}", r"\toprule",
         r"Configuraci\'on & $p$ & $t_{\max}/t_{\mathrm{prom}}$ & $p\,t_{\mathrm{prom}}/T_1$ & Overhead (s) & Karp-Flatt $e$ & Com.+espera (s) \\",
         r"\midrule"]
for c in CONFIGS:
    for p in PS:
        s = S[(c[0], c[1], NB, p)]
        kf = "--" if s["karp_flatt"] == "" else f"{s['karp_flatt']:.3f}"
        tc = f"{s['tcomm']:.3f}" if c[0] == "mpi" else "--"
        lines.append(f"{LABEL[c] if p == PS[0] else ''} & {p} & {s['imbalance']:.3f} & {s['work_ratio']:.2f} & "
                     f"{s['overhead']:.3f} & {kf} & {tc} \\\\")
    if c != CONFIGS[-1]:
        lines.append(r"\midrule")
lines += [r"\bottomrule", r"\end{tabular}"]
write("imbalance.tex", "\n".join(lines))

# Variabilidad: coeficiente de variacion maximo por version
cvs = defaultdict(list)
for k, g in groups.items():
    v = [float(r["time"]) for r in g]
    if len(v) > 1:
        cvs[(k[0], k[3])].append(st.stdev(v) / st.mean(v) * 100)
lines = [r"\begin{tabular}{lrr}", r"\toprule",
         r"Versi\'on & CV promedio (\%) & CV m\'aximo (\%) \\", r"\midrule"]
for k in [("seq", "-")] + CONFIGS:
    name = "Secuencial" if k[0] == "seq" else LABEL[k]
    lines.append(f"{name} & {st.mean(cvs[k]):.2f} & {max(cvs[k]):.2f} \\\\")
lines += [r"\bottomrule", r"\end{tabular}"]
write("variability.tex", "\n".join(lines))

# --------------------------------------------------------------- graficas
plt.rcParams.update({"font.size": 10, "axes.grid": True, "grid.alpha": 0.3,
                     "savefig.bbox": "tight"})

# 1) Tiempo vs p para cada N (todas las configuraciones)
fig, axes = plt.subplots(1, len(SIZES), figsize=(13, 3.4), sharey=False)
for ax, N in zip(axes, SIZES):
    for c in CONFIGS:
        ax.plot(PS, [S[(c[0], c[1], N, p)]["t_med"] for p in PS], "o-",
                color=COLORS[c], label=LABEL[c])
    ax.axhline(T1[N], color="gray", ls="--", lw=1, label="Secuencial")
    ax.set_title(f"N = {N/1e6:g} millones")
    ax.set_xlabel("p (hilos / procesos)")
    ax.set_xticks(PS)
axes[0].set_ylabel("Tiempo (s), mediana")
axes[0].legend(fontsize=8)
fig.savefig(os.path.join(FIG, "time_vs_p.pdf"))
plt.close(fig)

# 2) Speedup y 3) eficiencia vs p para N mayor y menor
for metric, fname, ylabel in [("speedup", "speedup_vs_p.pdf", "Speedup S(p)"),
                              ("eff", "efficiency_vs_p.pdf", "Eficiencia E(p)")]:
    fig, axes = plt.subplots(1, len(SIZES), figsize=(13, 3.4), sharey=True)
    for ax, N in zip(axes, SIZES):
        for c in CONFIGS:
            ax.plot(PS, [S[(c[0], c[1], N, p)][metric] for p in PS], "o-",
                    color=COLORS[c], label=LABEL[c])
        if metric == "speedup":
            ax.plot(PS, PS, "k:", lw=1, label="Ideal")
        else:
            ax.axhline(1.0, color="k", ls=":", lw=1, label="Ideal")
        ax.axvline(4, color="gray", lw=0.8, alpha=0.6)
        ax.set_title(f"N = {N/1e6:g} millones")
        ax.set_xlabel("p (hilos / procesos)")
        ax.set_xticks(PS)
    axes[0].set_ylabel(ylabel)
    axes[0].legend(fontsize=8)
    fig.savefig(os.path.join(FIG, fname))
    plt.close(fig)

# 4) Desbalance tmax/tavg vs p (N mayor)
fig, ax = plt.subplots(figsize=(6, 3.4))
width = 0.16
for i, c in enumerate(CONFIGS):
    xs = [j + (i - (len(CONFIGS) - 1) / 2) * width for j in range(len(PS))]
    ax.bar(xs, [S[(c[0], c[1], NB, p)]["imbalance"] for p in PS], width,
           color=COLORS[c], label=LABEL[c])
ax.set_xticks(range(len(PS)))
ax.set_xticklabels([str(p) for p in PS])
ax.axhline(1.0, color="k", lw=0.8)
ax.set_ylim(0.95, None)
ax.set_xlabel("p (hilos / procesos)")
ax.set_ylabel("t_max / t_prom")
ax.set_title(f"Desbalance de carga, N = {NB/1e6:g} millones")
ax.legend(fontsize=8)
fig.savefig(os.path.join(FIG, "imbalance.pdf"))
plt.close(fig)

# 5) Tiempo por hilo/proceso en block vs cyclic (N mayor, p=4): se obtiene
#    de tmax/tmin/tavg (mediana) para ilustrar el rango.
fig, ax = plt.subplots(figsize=(6, 3.4))
for i, c in enumerate(CONFIGS):
    k = (c[0], NB, 4, c[1])
    tmin = st.median(float(r["tmin"]) for r in groups[k])
    tmax = st.median(float(r["tmax"]) for r in groups[k])
    tavg = st.median(float(r["tavg"]) for r in groups[k])
    ax.bar(i, tmax - tmin, bottom=tmin, color=COLORS[c], width=0.5)
    ax.plot([i - 0.25, i + 0.25], [tavg, tavg], "k-", lw=1.5)
ax.set_xticks(range(len(CONFIGS)))
ax.set_xticklabels([LABEL[c].replace(" ", "\n", 1) for c in CONFIGS], fontsize=8)
ax.set_ylabel("Tiempo de cómputo por unidad (s)")
ax.set_title(f"Rango min-max de tiempos por hilo/proceso, p=4, N={NB/1e6:g} M")
fig.savefig(os.path.join(FIG, "per_worker_range.pdf"))
plt.close(fig)

# 6) Escalabilidad con el tamano: speedup con p=4 y p=8 vs N
fig, ax = plt.subplots(figsize=(6, 3.4))
for c in CONFIGS:
    for p, ls in [(4, "-"), (8, "--")]:
        ax.plot([N / 1e6 for N in SIZES], [S[(c[0], c[1], N, p)]["speedup"] for N in SIZES],
                "o" + ls, color=COLORS[c], label=f"{LABEL[c]}, p={p}")
ax.set_xscale("log", base=2)
ax.set_xticks([N / 1e6 for N in SIZES])
ax.set_xticklabels([f"{N/1e6:g}" for N in SIZES])
ax.set_xlabel("N (millones)")
ax.set_ylabel("Speedup")
ax.set_title("Speedup en función del tamaño del problema")
ax.legend(fontsize=7, ncol=2)
fig.savefig(os.path.join(FIG, "speedup_vs_n.pdf"))
plt.close(fig)

# 7) Tiempo secuencial vs N (crecimiento del costo)
fig, ax = plt.subplots(figsize=(6, 3.2))
ax.plot([N / 1e6 for N in SIZES], [T1[N] for N in SIZES], "ko-")
ax.set_xlabel("N (millones)")
ax.set_ylabel("T1 (s)")
ax.set_title("Tiempo de la versión secuencial")
fig.savefig(os.path.join(FIG, "seq_time.pdf"))
plt.close(fig)

print("Resumen en results/summary.csv; figuras en results/figures; tablas en results/tables")
for N in SIZES:
    print(f"N={N}: T1={T1[N]:.3f}  " + "  ".join(
        f"{LABEL[c]} p=4 S={S[(c[0], c[1], N, 4)]['speedup']:.2f} p=8 S={S[(c[0], c[1], N, 8)]['speedup']:.2f}"
        for c in CONFIGS))
