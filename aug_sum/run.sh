export CILK_NWORKERS=1
./aug_sum 0 1 > aug1.txt
./aug_sum 1 1 >> aug1.txt

export CILK_NWORKERS=24
./aug_sum 31 1 >> aug1.txt

export CILK_NWORKERS=1
./aug_sum 32 1 >> aug1.txt
./aug_sum 32 100 >> aug1.txt
./aug_sum 32 10000 >> aug1.txt

echo "stage 5"
export CILK_NWORKERS=24
for i in 3 8 31 41 4 5 61 6 7
do
./aug_sum $i 1 >> aug1.txt
done