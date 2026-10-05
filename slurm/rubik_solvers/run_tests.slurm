#!/bin/bash
#SBATCH --job-name=gpu_RUBIK
#SBATCH --partition=n2gpu
#SBATCH --gres=gpu:l4:1                 # the solvers do not use the GPU; kept because this partition is used with it
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=48              # threads for the parallel programs
#SBATCH --mem=140G                      # hard limit of Slurm (your cap is 150 GB); the programs stop themselves at 120 GB
#SBATCH --time=12:00:00                 # check the partition maximum:  sinfo -p n2gpu -o "%P %l"
#SBATCH --output=/mnt/beegfs/insarlab/falcon/rubik_solvers/resultadoRUBIK_%j.out
#SBATCH --error=/mnt/beegfs/insarlab/falcon/rubick_solvers/errorRUBIK_%j.err

# Full experiment: 3 algorithms x (sequential, parallel). Same random cubes for all (same seed).
PROJECT_DIR="/mnt/beegfs/insarlab/falcon/rubik_solvers"   # folder where you unzipped rubik_solvers.zip

cd "$PROJECT_DIR" || exit 1
echo "Starting on node: $(hostname)   $(date)"
echo "CPUs for this job: $SLURM_CPUS_PER_TASK"

export OMP_NUM_THREADS=$SLURM_CPUS_PER_TASK
export OMP_PROC_BIND=close OMP_PLACES=cores

make -j8 || { echo "compilation failed"; exit 1; }       # compile on THIS node (-march=native)
./astar_par --selftest || { echo "self-test failed"; exit 1; }
./astar_par --build-only                                   # pattern databases are built once (parallel) and cached in ./pdb_cache

SEED=2026; PER=10; TO=600; MEM=120
OUT="results_$SLURM_JOB_ID"; mkdir -p "$OUT"               # one folder per job, so reruns never mix their CSV rows

for v in seq par; do
  ./iddfs_$v --depths 1-9  --per-depth $PER --seed $SEED --timeout $TO --mem-gb $MEM --csv $OUT/iddfs_$v.csv --quiet
  ./bidir_$v --depths 1-14 --per-depth $PER --seed $SEED --timeout $TO --mem-gb $MEM --csv $OUT/bidir_$v.csv --quiet
  ./astar_$v --depths 1-19 --per-depth $PER --seed $SEED --timeout $TO --mem-gb $MEM --csv $OUT/astar_$v.csv --quiet
done
echo "Finished: $(date)   results in $PROJECT_DIR/$OUT"
