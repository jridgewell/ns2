BEGIN {
    # transSize1 = []
    # transSize2 = []
    l1 = 0
    l2 = 0
    step = 1
}

{
    event = $1
    time = int($2 / step) * step
    send_id = $3
    pkt_size = $6
    flow_id = $8


    if (send_id == "0" || send_id == "3") {
        if (event == "+") {
            # Store transmitted packet's size
            if (flow_id == "1") {
                if (time > l1) {
                    l1 = time;
                }
                transSize1[time] += pkt_size
            }
            if (flow_id == "2") {
                if (time > l2) {
                    l2 = time;
                }
                transSize2[time] += pkt_size
            }
        }
    }
}

END {
    max = (l1 < l2) ? l2 : l1
    for (i = 0; i < max; i+=step) {
        printf("%i\t%.2f\t%.2f\n", i, transSize1[i]/1024/1024*8, transSize2[i]/1024/1204*8)
    }
}
