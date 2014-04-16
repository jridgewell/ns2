/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Copyright (C) Xerox Corporation 1997. All rights reserved.
 *  
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Linking this file statically or dynamically with other modules is making
 * a combined work based on this file.  Thus, the terms and conditions of
 * the GNU General Public License cover the whole combination.
 *
 * In addition, as a special exception, the copyright holders of this file
 * give you permission to combine this file with free software programs or
 * libraries that are released under the GNU LGPL and with code included in
 * the standard release of ns-2 under the Apache 2.0 license or under
 * otherwise-compatible licenses with advertising requirements (or modified
 * versions of such code, with unchanged license).  You may copy and
 * distribute such a system following the terms of the GNU GPL for this
 * file and the licenses of the other code concerned, provided that you
 * include the source code of that other code when and as the GNU GPL
 * requires distribution of source code.
 *
 * Note that people who make modified versions of this file are not
 * obligated to grant this special exception for their modified versions;
 * it is their choice whether to do so.  The GNU General Public License
 * gives permission to release a modified version without this exception;
 * this exception also makes it possible to release a modified version
 * which carries forward this exception.
 */

#ifndef lint
static const char rcsid[] =
    "@(#) $Header: /cvsroot/nsnam/ns-2/apps/udp.cc,v 1.21 2005/08/26 05:05:28 tomh Exp $ (Xerox)";
#endif

#include <algorithm>
#include <math.h>
#include <limits>
#include "udp.h"
#include "rtp.h"
#include "random.h"
#include "address.h"
#include "ip.h"


static class UdpAgentClass : public TclClass {
public:
	UdpAgentClass() : TclClass("Agent/UDP") {}
	TclObject* create(int, const char*const*) {
		return (new UdpAgent());
	}
} class_udp_agent;

UdpAgent::UdpAgent() : Agent(PT_TCP), seqno_(-1)
{
	bind("packetSize_", &size_);
}

UdpAgent::UdpAgent(packet_t type) : Agent(type)
{
	bind("packetSize_", &size_);
}

// put in timestamp and sequence number, even though UDP doesn't usually 
// have one.
void UdpAgent::sendmsg(int nbytes, AppData* data, const char* flags)
{
	Packet *p;
	int n;

	assert (size_ > 0);

	n = nbytes / size_;

	if (nbytes == -1) {
		printf("Error:  sendmsg() for UDP should not be -1\n");
		return;
	}	

	// If they are sending data, then it must fit within a single packet.
	if (data && nbytes > size_) {
		printf("Error: data greater than maximum UDP packet size\n");
		return;
	}

	double local_time = Scheduler::instance().clock();
	while (n-- > 0) {
		p = allocpkt();
		hdr_cmn::access(p)->size() = size_;
		hdr_rtp* rh = hdr_rtp::access(p);
		rh->flags() = 0;
		rh->seqno() = ++seqno_;
		hdr_cmn::access(p)->timestamp() = 
		    (u_int32_t)(SAMPLERATE*local_time);
		// add "beginning of talkspurt" labels (tcl/ex/test-rcvr.tcl)
		if (flags && (0 ==strcmp(flags, "NEW_BURST")))
			rh->flags() |= RTP_M;
		p->setdata(data);
        send(p);
	}
	n = nbytes % size_;
	if (n > 0) {
		p = allocpkt();
		hdr_cmn::access(p)->size() = n;
		hdr_rtp* rh = hdr_rtp::access(p);
		rh->flags() = 0;
		rh->seqno() = ++seqno_;
		hdr_cmn::access(p)->timestamp() = 
		    (u_int32_t)(SAMPLERATE*local_time);
		// add "beginning of talkspurt" labels (tcl/ex/test-rcvr.tcl)
		if (flags && (0 == strcmp(flags, "NEW_BURST")))
			rh->flags() |= RTP_M;
		p->setdata(data);
        send(p);
	}
	idle();
}
void UdpAgent::recv(Packet* pkt, Handler*)
{
	if (app_ ) {
		// If an application is attached, pass the data to the app
		hdr_cmn* h = hdr_cmn::access(pkt);
		app_->process_data(h->size(), pkt->userdata());
	} else if (pkt->userdata() && pkt->userdata()->type() == PACKET_DATA) {
		// otherwise if it's just PacketData, pass it to Tcl
		//
		// Note that a Tcl procedure Agent/Udp recv {from data}
		// needs to be defined.  For example,
		//
		// Agent/Udp instproc recv {from data} {puts data}

		PacketData* data = (PacketData*)pkt->userdata();

		hdr_ip* iph = hdr_ip::access(pkt);
                Tcl& tcl = Tcl::instance();
		tcl.evalf("%s process_data %d {%s}", name(),
		          iph->src_.addr_ >> Address::instance().NodeShift_[1],
			  data->data());
	}
	Packet::free(pkt);
}


int UdpAgent::command(int argc, const char*const* argv)
{
	if (argc == 4) {
		if (strcmp(argv[1], "send") == 0) {
			PacketData* data = new PacketData(1 + strlen(argv[3]));
			strcpy((char*)data->data(), argv[3]);
			sendmsg(atoi(argv[2]), data);
			return (TCL_OK);
		}
	} else if (argc == 5) {
		if (strcmp(argv[1], "sendmsg") == 0) {
			PacketData* data = new PacketData(1 + strlen(argv[3]));
			strcpy((char*)data->data(), argv[3]);
			sendmsg(atoi(argv[2]), data, argv[4]);
			return (TCL_OK);
		}
	}
	return (Agent::command(argc, argv));
}

void RetransmitTimer::expire(Event *e) {
	a_->timeout();
}

static class CTcpAgentClass : public TclClass {
public:
    CTcpAgentClass() : TclClass("Agent/TCP/CTCP") {}
    TclObject* create(int, const char*const*) {
        return (new CTcpAgent());
    }
} class_ctcp_agent;

CTcpAgent::CTcpAgent() : UdpAgent(), rtx_timer_(this), time_lastack_(0),
    min_rtto_(0.2), rtt_(0.2), rtt_min_(std::numeric_limits<double>::max()), numblks_(10), blksize_(10),
    seqno_nxt_(0), seqno_una_(1),
    slow_start_(true), ss_threshold_(20), tokens_(1),
    currblk_(0), currdof_(0),
    p_(0.0), u_(.015), y_(2.0),
    total_blocks_(0), last_block_size_(0)
{
    block_numbers_ = new std::vector<int>();
    send_timestamps_ = new std::vector<double>();
    coding_window_ = new std::vector< std::vector<Packet*> *>();
}

int CTcpAgent::B(int seqno) {
    return block_numbers_->at(seqno);
}

double CTcpAgent::T(int seqno) {
    return send_timestamps_->at(seqno);
}

void CTcpAgent::timeout() {
    //TODO: bind initial tokens
    tokens_ = 1;
    seqno_una_ = seqno_nxt_ + 1;
    slow_start_ = true;
    send_packets();
}

void CTcpAgent::sendmsg(int nbytes, AppData* data, const char* flags) {
    assert (size_ > 0);
    Packet *p;
    int n;
    n = nbytes / size_;
    if (nbytes == -1) {
        printf("Error:  sendmsg() for UDP should not be -1\n");
        return;
    }	
    // If they are sending data, then it must fit within a single packet.
    if (data && nbytes > size_) {
        printf("Error: data greater than maximum UDP packet size\n");
        return;
    }

    double local_time = Scheduler::instance().clock();
    if (nbytes % size_ > 0) { n++; }
    while (n-- > 0) {
        p = allocpkt();
        hdr_cmn::access(p)->size() = size_;
        hdr_rtp* rh = hdr_rtp::access(p);
        rh->flags() = 0;
        rh->seqno() = ++seqno_;
        hdr_cmn::access(p)->timestamp() = 
            (u_int32_t)(SAMPLERATE*local_time);
        // add "beginning of talkspurt" labels (tcl/ex/test-rcvr.tcl)
        if (flags && (0 ==strcmp(flags, "NEW_BURST")))
            rh->flags() |= RTP_M;
        p->setdata(data);
        send(p);
    }
    idle();
}


void CTcpAgent::send(Packet* p) {
    hdr_tcp* header = hdr_tcp::access(p);
    int seqno = hdr_rtp::access(p)->seqno();
    header->seqno() = seqno;
    int blkno = seqno / blksize_;
    int index = seqno % blksize_;

    blkno -= currblk_;

    if (total_blocks_ <= blkno) {
        coding_window_->push_back(new std::vector<Packet*>());
        total_blocks_++;
        last_block_size_ = 0;
    }
    if (last_block_size_ <= index) {
        coding_window_->at(blkno)->resize(index + 1);
        last_block_size_++;
    }
    coding_window_->at(blkno)->at(index) = p;
    send_packets();
}

void CTcpAgent::send_packets() {
    double current_time = Scheduler::instance().clock();
    int tokens = tokens_ - (seqno_nxt_ - seqno_una_) - 1;
    int i;

    if (tokens > 0) {
        int on_fly[numblks_];
        for (i = 0; i < numblks_; i++) {
            on_fly[i] = 0;
        }
        for (int seqno = seqno_una_; seqno < seqno_nxt_; seqno++) {
            if (B(seqno) >= currblk_ && current_time < T(seqno) + (1.5 * rtt_)) { // TODO: Bind 1.5
                on_fly[B(seqno) - currblk_]++;
            }
        }
        on_fly[0] += currdof_;

        while (tokens > 0) {
            tokens--;
            for (int blkno = 0; blkno < numblks_; blkno++) {
                if (blkno + currblk_ >= total_blocks_) {
                    break;
                }
                if ((1 - p_) * on_fly[blkno] < blksize_) {
                    send_packet(seqno_nxt_, blkno + currblk_);
                    on_fly[blkno]++;
                    seqno_nxt_++;
                    break;
                }
            }
        }

        if (rtx_timer_.status() != TIMER_PENDING) {
            /* No timer pending.  Schedule one. */
            set_rtx_timer();
        }
    }
}

void CTcpAgent::send_packet(int seqno, int blkno) {
    srand(1);
    for (int i = 0; i < seqno; i++) {
        rand();
    }
    int seed = rand();
    srand(seed);

    Packet *p = allocpkt();
    hdr_cmn::access(p)->size() = size_;
    hdr_tcp* header = hdr_tcp::access(p);
    header->seqno() = seqno;
    header->blkno() = blkno;
    header->blksize() = blksize_;
    header->seed() = seed;
    header->num_packets() = (int)coding_window_->at(blkno - currblk_)->size();

    //TODO: create linear combination of data
    // std::vector <Packet*> *block = coding_window_->at(blkno);
    // for (int i = 0; i < blksize_; i++) {
    // }

    send_timestamps_->push_back(Scheduler::instance().clock());
    block_numbers_->push_back(blkno);

    UdpAgent::send(p);
}

void CTcpAgent::set_rtx_timer() {
    double rtt = (rtt_ > min_rtto_) ? rtt_ : min_rtto_;
    rtx_timer_.resched(y_ * rtt);
}

void CTcpAgent::recv(Packet* pkt, Handler* h) {
    hdr_tcp* header = hdr_tcp::access(pkt);
    int seqno = header->seqno();
    int blk = header->ack_currblk();
    int dof = header->ack_currdof();

    // Update Parameters Algorithm
    time_lastack_ = Scheduler::instance().clock();
    rtt_ = time_lastack_ - T(seqno);
    rtt_min_ = std::min(rtt_, rtt_min_);

    if (blk > currblk_) {
        while (currblk_++ < blk) {
            for (int i = 0; i < (int)coding_window_->front()->size(); i++) {
                Packet::free(coding_window_->front()->at(i));
            }
            coding_window_->front()->clear();
            delete coding_window_->front();
            coding_window_->erase(coding_window_->begin());
        }
        currblk_ = blk;
        currdof_ = dof;
    }
    currdof_ = std::max(currdof_, dof);
    if (seqno >= seqno_una_) {
        int losses = seqno - seqno_una_;
        p_ = p_ * pow(1.0 - u_, losses + 1) + (1 - pow(1.0 - u_, losses));
    }

    // Congestion Control Algorithm
    if (slow_start_) {
        tokens_++;
        if (tokens_ > ss_threshold_) {
            slow_start_ = false;
        }
    } else {
        if (seqno > seqno_una_) {
            tokens_ *= rtt_min_ / rtt_;
        } else {
            tokens_ += 1.0 / tokens_;
        }
    }

    set_rtx_timer();

    seqno_una_ = seqno + 1;

    Packet::free(pkt);
    // UdpAgent::recv(pkt, h);

    if (currblk_ >= total_blocks_ - 1 && currdof_ >= last_block_size_ - 1) {
        rtx_timer_.force_cancel();
        finish();
    }

    // Send packets for after updating tokens
    send_packets();
}

void CTcpAgent::finish() {
	Tcl::instance().evalf("%s done", this->name());
	Tcl::instance().evalf("finish");
}
