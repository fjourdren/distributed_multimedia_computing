# chmod a+x run_performance.sh
# ./run_performance.sh 100

for filename in ../imgs/*.pgm
do
    RESULT=$(seq $1 | xargs -Iz ./CodeMaitre ../imgs/$filename | grep "chrono " | awk '{ total += $2; count++ } END { print total/count }')
    echo "$filename => $RESULTμs"
done