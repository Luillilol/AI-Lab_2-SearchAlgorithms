#!/bin/bash
#SBATCH --job-name=RUBIK_par_rest
#SBATCH --nodes=1
#SBATCH --partition=extra
#SBATCH --nodelist=node4
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=48              # same number of threads as run_tests.slurm (node2)
#SBATCH --mem=140G                      # Slurm limit; the programs stop themselves at 120 GB
#SBATCH --exclusive                     # nobody else on the node while we measure times (delete this line if the cluster refuses it)
#SBATCH --exclude=node2                 # node2 is busy with run_tests.slurm
#SBATCH --time=12:00:00                 # check the partition maximum:  sinfo -N -o "%N %P %l"
#SBATCH --output=/mnt/beegfs/insarlab/falcon/rubik_solvers/resultadoRUBIK_par_rest_%j.out
#SBATCH --error=/mnt/beegfs/insarlab/falcon/rubik_solvers/resultadoRUBIK_par_rest_%j.err

# IDDFS parallel + BIDIRECTIONAL parallel. Same cubes / limits as run_tests.slurm  ->  keep SEED, PER, TO, MEM identical to it.
SRC="/mnt/beegfs/insarlab/falcon/rubik_solvers" # folder with the sources and pdb_cache (shared disk)
SEED=2026
PER=10
TO=600
MEM=120

export OMP_NUM_THREADS=$SLURM_CPUS_PER_TASK
export OMP_PROC_BIND=close OMP_PLACES=cores

OUT="$SRC/results_par_$SLURM_JOB_ID"
mkdir -p "$OUT"
WORK="/tmp/rubik_$SLURM_JOB_ID" # private build folder on THIS node (never touches the binaries of node2's job)
trap 'rm -rf "$WORK"' EXIT
mkdir -p "$WORK"
cp -r "$SRC/Makefile" "$SRC/common" "$SRC"/*.cpp "$WORK/" || exit 1
cd "$WORK" || exit 1

{
  echo "node: $(hostname)   date: $(date)   threads: $OMP_NUM_THREADS"
  lscpu | grep -E "Model name|Socket|NUMA node\(s\)"
  free -h | head -2
} | tee "$OUT/node_info.txt"

[ -d "$SRC/pdb_cache" ] || {
  echo "pdb_cache not found in $SRC: run run_tests.slurm (or ./astar_par --build-only) first"
  exit 1
}
make -j8 || {
  echo "compilation failed"
  exit 1
} # compiled on THIS node (-march=native)
./iddfs_par --selftest || {
  echo "self-test failed"
  exit 1
}
./bidir_par --selftest || {
  echo "self-test failed"
  exit 1
}

# speed check of this node: tiny sequential runs on cubes that node2 also solves (same seed, depth, index)
./astar_seq --depths 10-12 --per-depth 3 --seed $SEED --timeout $TO --mem-gb $MEM --pdb-dir "$SRC/pdb_cache" --csv "$OUT/calib_astar_seq_$(hostname).csv" --quiet

./iddfs_par --depths 1-9 --per-depth $PER --seed $SEED --timeout $TO --mem-gb $MEM --csv "$OUT/iddfs_par.csv" --quiet
./bidir_par --depths 1-14 --per-depth $PER --seed $SEED --timeout $TO --mem-gb $MEM --csv "$OUT/bidir_par.csv" --quiet
echo "Finished: $(date)   results in $OUT"
