BEGIN {
    recvdSize = 0
    transSize = 0
    startTime = 400
    stopTime = 0
}

{
    event = $1
    time = $2
    send_id = $3
    rec_id = $4
    pkt_size = $6
    flow_id = $8

    # Store start time
    if (send_id == "0") {
        if (time < startTime) {
            startTime = time
        }

        if (event == "+") {
            # Store transmitted packet's size
            transSize += pkt_size
        }
    }

    # Update total received packets' size and store packets arrival time
    if (event == "r" && rec_id == "2") {
        if (time > stopTime) {
            stopTime = time
        }
        # Store received packet's size
        if (flow_id == "1") {
            recvdSize += pkt_size
        }
    }
}

END {
    printf("%i\t%i\t%.2f\t%.2f\n", transSize, recvdSize, startTime, stopTime)
}
