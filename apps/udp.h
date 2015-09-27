/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Copyright (c) Xerox Corporation 1997. All rights reserved.
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
 *  
 * @(#) $Header: /cvsroot/nsnam/ns-2/apps/udp.h,v 1.15 2005/08/26 05:05:28 tomh Exp $ (Xerox)
 */

#ifndef ns_udp_h
#define ns_udp_h

#include "agent.h"
#include "trafgen.h"
#include "packet.h"
#include "tcp.h"

//"rtp timestamp" needs the samplerate
#define SAMPLERATE 8000
#define RTP_M 0x0080 // marker for significant events
#define ZERO 1.0E-20

typedef struct consecutive_ones {
    int start;
    int length;
    bool operator==(const consecutive_ones& rhs) const {
        return this->start == rhs.start && this->length == rhs.length;
    }
} consecutive_ones;

class UdpAgent : public Agent {
public:
	UdpAgent();
	UdpAgent(packet_t);
	virtual void sendmsg(int nbytes, const char *flags = 0)
	{
		sendmsg(nbytes, NULL, flags);
	}
	virtual void sendmsg(int nbytes, AppData* data, const char *flags = 0);
	virtual void send(Packet* p) { target_->recv(p); }
	virtual void recv(Packet* pkt, Handler*);
	virtual int command(int argc, const char*const* argv);
protected:
	int seqno_;
};


class CTcpAgent;
class RetransmitTimer : public TimerHandler {
public:
	RetransmitTimer(CTcpAgent *a) : TimerHandler() { a_ = a; }
protected:
	virtual void expire(Event *e);
	CTcpAgent *a_;
};

class CTcpAgent : public UdpAgent {
public:
	CTcpAgent();
	CTcpAgent(packet_t);
    ~CTcpAgent();
	virtual void recv(Packet* pkt, Handler*);
	virtual void timeout();
    virtual void set_rtx_timer();

	virtual void sendmsg(int nbytes, AppData* data, const char *flags = 0);
    virtual void send_packets();
    virtual void send_packet(int seqno, int blkno);
    virtual double T(int seqno);
    virtual int B(int seqno);
    void finish();
protected:
    virtual void delay_bind_init_all();
    virtual int delay_bind_dispatch(const char *varName, const char *localName, TclObject *tracer);

	RetransmitTimer rtx_timer_;

    int curseq_;
    double time_lastack_;
    double min_rtto_;
    double rtt_;
    double rtt_min_;
    int numblks_;
    int blksize_;
    int seqno_nxt_;
    int seqno_una_;

    bool slow_start_;
    int ss_threshold_;
    double tokens_;
    int used_tokens_;
    int currblk_;
    int *currdof_;
    double p_;
    double u_;
    double y_;

    int total_blocks_;
    int last_block_size_;

    std::vector<double>* send_timestamps_;
    std::vector<int>* block_numbers_;

    std::vector< std::vector<consecutive_ones>* > *consecutive_ones_;

    std::vector< std::vector<Packet*> *> *coding_window_;
};

inline bool double_equal(double val, int expected) {
    return (val <= expected + ZERO && val >= expected - ZERO);
}

inline bool double_equal(double val, int expected, double precision) {
    return (val <= expected + precision && val >= expected - precision);
}

#endif
