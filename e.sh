DATE=$(date +%Y%m%d)
echo $DATE >> "pid.txt"
list=("application")
for t in "${list[@]}"; do
    ./build/iPrivJoin $t
done