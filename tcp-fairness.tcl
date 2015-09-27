#===================================
#     Simulation parameters setup
#===================================
proc default_options {} {
    global opts opt_wants_arg

    set raw_opt_info {
        duration 120
        output out.tr
        outnam out.nam
        p 0.0
        agent Reno
        agent2 Reno
        nc_r 1
        nc_field_size_ 256
        size 16777216
        queue 20
    }

    while {$raw_opt_info != ""} {
        if {![regexp "^\[^\n\]*\n" $raw_opt_info line]} {
            break
        }
        regsub "^\[^\n\]*\n" $raw_opt_info {} raw_opt_info
        set line [string trim $line]
        if {[regexp "^\[ \t\]*#" $line]} {
            continue
        }
        if {$line == ""} {
            continue
        } elseif [regexp {^([^ ]+)[ ]+([^ ]+)$} $line dummy key value] {
            set opts($key) $value
            set opt_wants_arg($key) 1
        } else {
            set opt_wants_arg($key) 0
            # die "unknown stuff in raw_opt_info\n"
        }
    }
}


proc process_args {} {
    global argc argv opts opt_wants_arg

    default_options
    for {set i 0} {$i < $argc} {incr i} {
        set key [lindex $argv $i]
        regsub {^-} $key {} key
        if {![info exists opt_wants_arg($key)]} {
            puts stderr "unknown option $key";
        }
        if {$opt_wants_arg($key)} {
            incr i
            set opts($key) [lindex $argv $i]
        } else {
            set opts($key) [expr !opts($key)]
        }
    }
}
process_args

#===================================
#        Initialization
#===================================
#Create a ns simulator
set ns [new Simulator]

#Open the NS trace file
set tracefile [open $opts(output) w]
$ns trace-all $tracefile

#Open the NAM trace file
set namfile [open $opts(outnam) w]
$ns namtrace-all $namfile

#===================================
#        Nodes Definition
#===================================
#Create 2 nodes
set source [$ns node]
set router [$ns node]
set sink [$ns node]
set other [$ns node]

#===================================
#        Links Definition
#===================================
#Createlinks between nodes
$ns duplex-link $source $router 1Mb 100ms DropTail
$ns duplex-link $router $sink 1Mb 100ms SFQ
$ns duplex-link $other $router 1Mb 100ms DropTail

#Give node position (for NAM)
$ns duplex-link-op $source $router orient right-up
$ns duplex-link-op $router $sink orient right
$ns duplex-link-op $other $router orient right-down

#Monitor the queue for link (router-sink) (for NAM)
$ns queue-limit $router $sink $opts(queue)
$ns duplex-link-op $router $sink queuePos 0.5

#===================================
#        Agents Definition
#===================================
set tcp0 [new Agent/TCP/$opts(agent)]
$tcp0 set class_ 1
$tcp0 set fid_ 1
$tcp0 set window_ 1000
$ns attach-agent $source $tcp0

# Sink
if {$opts(agent) == {CTCP}} {
    set sink0 [new Agent/TCPSink/CTCP]
} else {
    set sink0 [new Agent/TCPSink]
}
$ns attach-agent $sink $sink0
$ns connect $tcp0 $sink0

# Fairness with NCTCP
set tcp1 [new Agent/TCP/$opts(agent2)]
$tcp1 set class_ 2
$tcp1 set fid_ 2
$tcp1 set window_ 1000
$ns attach-agent $other $tcp1


if {$opts(agent2) == {CTCP}} {
    set sink1 [new Agent/TCPSink/CTCP]
} else {
    set sink1 [new Agent/TCPSink]
}
$ns attach-agent $sink $sink1
$ns connect $tcp1 $sink1


#===================================
#        Applications Definition
#===================================
set app0 [new Application/FTP]
$app0 attach-agent $tcp0
$app0 set packet_size_ 1000
$ns at 1.0 "$app0 send $opts(size)"

set app1 [new Application/FTP]
$app1 attach-agent $tcp1
$app1 set packet_size_ 1000
$ns at 10.0 "$app1 send 8388608"


$ns color 1 Blue
$ns color 2 Red
#===================================
#        Termination
#===================================
#Define a 'finish' procedure
proc finish {} {
    global ns tracefile namfile
    $ns flush-trace
    close $tracefile
    close $namfile
    # exec nam out.nam &
    exit 0
}

# $ns at $opts(duration) "$ns nam-end-wireless $opts(duration)"
$ns at $opts(duration) "finish"
# $ns at $opts(duration) "puts \"done\" ; $ns halt"
$ns run
