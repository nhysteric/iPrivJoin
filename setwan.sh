funcWan(){
    (sudo tc qdisc del dev lo root)
    n=101  # 修改为 101，因为端口范围是 8000-8100 共 101 个端口
    num=$((n-1))
    (sudo tc qdisc add root dev lo handle 1: htb default $((num+1)))
    
    i=0
    while [ $i -lt $num ]; do
        let i++
        (sudo tc class add dev lo parent 1:0 classid 1:$i htb rate 40Mbit)
    done
    (sudo tc class add dev lo parent 1:0 classid 1:$((num+1)) htb rate 1000Mbps)
    
    i=0
    while [ $i -lt $num ]; do
        let i++
        (sudo tc qdisc add dev lo parent 1:$i handle $((i+1)):0 netem delay 20ms)
    done
    
    i=0
    num=1
    port_start=8000
    port_end=8100
    while [ $port_start -le $port_end ]; do
        (sudo tc filter add dev lo protocol ip parent 1:0 prio 1 u32 match ip dport $port_start 0xffff flowid 1:$num)
        let num++
        let port_start++
    done
}

funcWan
