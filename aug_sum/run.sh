export CILK_NWORKERS=1
./aug_sum 10 > aug.txt
echo ./aug_sum 10 >> aug.txt
export CILK_NWORKERS=2
./aug_sum 10 >> aug.txt
echo ./aug_sum 10 >> aug.txt
export CILK_NWORKERS=4
./aug_sum 10 >> aug.tzxt
echo ./aug_sum 10 >> aug.txt
export CILK_NWORKERS=8
./aug_sum 10 >> aug.txt
echo ./aug_sum 10 >> aug.txt
export CILK_NWORKERS=16
./aug_sum 10 >> aug.txt
echo ./aug_sum 10 >> aug.txt
