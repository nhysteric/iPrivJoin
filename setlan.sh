funcLan() {
    (sudo tc qdisc del dev lo root)
    n=10
    num=$((n*(n-1)))
    (sudo tc qdisc add root dev lo handle 1: htb default $((num+1)))
    
    i=0
    while [ $i -lt $num ]; do
        let i++
        (sudo tc class add dev lo parent 1:0 classid 1:$i htb rate 1000Mbps)
    done
    (sudo tc class add dev lo parent 1:0 classid 1:$((num+1)) htb rate 1000Mbps)
    
    i=0
    num=1
    while [ $i -lt $n ]; do
        j=$((i+1))
        while [ $j -lt $n ]; do
            (sudo tc filter add dev lo protocol ip parent 1:0 prio 1 u32 match ip dport $((8000 + i * 100 + j)) 0xffff flowid 1:$num)
            # echo $((8000 + i * 100 + j))
            let num++
            let j++
        done
        let i++
    done
}

funcLan
