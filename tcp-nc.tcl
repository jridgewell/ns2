# This script is created by NSG2 beta1
# <http://wushoupong.googlepages.com/nsg>

#===================================
#     Simulation parameters setup
#===================================
proc default_options {} {
	global opts opt_wants_arg

	set raw_opt_info {
		duration 60
		output out.tr
		outnam out.nam
		p 0.0
		agent NC
        nc_r 1
        nc_field_size_ 256
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

#===================================
#        Links Definition        
#===================================
#Createlinks between nodes
$ns duplex-link $source $router 10Mb 10ms DropTail
$ns duplex-link $router $sink 10Mb 10ms DropTail
$ns queue-limit $router $sink 100

#Give node position (for NAM)
$ns duplex-link-op $source $router orient right
$ns duplex-link-op $router $sink orient right

#Monitor the queue for link (router-sink) (for NAM)
$ns duplex-link-op $router $sink queuePos 0.5


#===================================
#        Agents Definition        
#===================================
set tcp0 [new Agent/TCP/$opts(agent)]
# set tcp0 [new Agent/TCP/Vegas]
$tcp0 set class_ 0
$ns attach-agent $source $tcp0
if {$opts(agent) == {NC}} {
	$tcp0 set nc_r_ $opts(nc_r)
	$tcp0 set nc_field_size_ $opts(nc_field_size_)
}
# $tcp0 set v_alpha_ 28
# $tcp0 set v_beta_ 30
# $tcp0 set v_gamma_ 2

if {$opts(agent) == {NC}} {
    set sink0 [new Agent/TCPSink/NC]
} else {
    set sink0 [new Agent/TCPSink]
}
$ns attach-agent $sink $sink0
$ns connect $tcp0 $sink0

set em [new ErrorModel]
$em unit pkt
$em set rate_ $opts(p)
$em drop-target [new Agent/Null]
$ns lossmodel $em $router $sink

#===================================
#        Applications Definition        
#===================================
#Setup a CBR Application over TCP connection
# set app0 [new Application/Traffic/CBR]
# $app0 set type_ CBR
# $app0 set rate_ 3Mb
# $app0 set random_ false
set app0 [new Application/FTP]
$app0 attach-agent $tcp0
$app0 set packet_size_ 1000
$ns at 1.0 "$app0 send 67108864"
# $ns at 1.0 "$app0 start"
# $ns at 11.0 "$app0 stop"


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
# $ns at $opts(duration) "finish"
# $ns at $opts(duration) "puts \"done\" ; $ns halt"
$ns run
