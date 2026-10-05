#!/bin/bash
#SBATCH --job-name=gpu_RUBIK_test
#SBATCH --partition=n2gpu
#SBATCH --gres=gpu:l4:1
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=48
#SBATCH --mem=80G
#SBATCH --time=00:40:00
#SBATCH --output=/mnt/beegfs/insarlab/falcon/rubik_solvers/testRUBIK_%j.out
#SBATCH --error=/mnt/beegfs/insarlab/falcon/rubik_solvers/testRUBIK_%j.err

# SHORT test: run this first. Small depths, small limits (60 GB), 2 cubes per depth.
PROJECT_DIR="/mnt/beegfs/insarlab/falcon/rubik_solvers"
cd "$PROJECT_DIR" || exit 1
echo "Starting on node: $(hostname)   $(date)"
export OMP_NUM_THREADS=$SLURM_CPUS_PER_TASK OMP_PROC_BIND=close OMP_PLACES=cores

make -j8 || { echo "compilation failed"; exit 1; }
for p in iddfs_seq iddfs_par bidir_seq bidir_par astar_seq astar_par; do ./$p --selftest | tail -1; done
./astar_par --build-only

OUT="smoke_$SLURM_JOB_ID"; mkdir -p "$OUT"
for v in seq par; do
  ./iddfs_$v --depths 1-8  --per-depth 2 --seed 1 --timeout 30  --mem-gb 60 --csv $OUT/iddfs_$v.csv --quiet
  ./bidir_$v --depths 1-12 --per-depth 2 --seed 1 --timeout 60  --mem-gb 60 --csv $OUT/bidir_$v.csv --quiet
  ./astar_$v --depths 1-14 --per-depth 2 --seed 1 --timeout 120 --mem-gb 60 --csv $OUT/astar_$v.csv --quiet
done
echo "Finished: $(date)   results in $PROJECT_DIR/$OUT"
