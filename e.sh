cmake --build build -j
mkdir test/lan
mkdir test/wan
tcdel lo --all
tcset lo --rate 40Mbps --delay 20ms --network 127.0.0.1 --port 8011

list=("1380-10" "4658-17" "20016-29" "24818-90" "156436-12" "265206-21")
for t in "${list[@]}"; do
    ./build/hash_test $t 
done
for t in "${list[@]}"; do
    ./build/iPrivJoin $t wan
done

tcdel lo --all
for t in "${list[@]}"; do
    ./build/iPrivJoin $t lan
done