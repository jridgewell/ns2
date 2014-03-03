/*
 * tcp-nc.cc
 * Copyright (C) 2014 Justin Ridgewell
 * $Id: tcp-nc.cc,v 1.37 2005/08/25 18:58:12 johnh Exp $
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 *
 *
 * The copyright of this module includes the following
 * linking-with-specific-other-licenses addition:
 *
 * In addition, as a special exception, the copyright holders of
 * this module give you permission to combine (via static or
 * dynamic linking) this module with free software programs or
 * libraries that are released under the GNU LGPL and with code
 * included in the standard release of ns-2 under the Apache 2.0
 * license or under otherwise-compatible licenses with advertising
 * requirements (or modified versions of such code, with unchanged
 * license).  You may copy and distribute such a system following the
 * terms of the GNU GPL for this module and the licenses of the
 * other code concerned, provided that you include the source code of
 * that other code when and as the GNU GPL requires distribution of
 * source code.
 *
 * Note that people who make modified versions of this module
 * are not obligated to grant this special exception for their
 * modified versions; it is their choice whether to do so.  The GNU
 * General Public License gives permission to release a modified
 * version without this exception; this exception also makes it
 * possible to release a modified version which carries forward this
 * exception.
 *
 */

#ifndef lint
static const char rcsid[] =
"@(#) $Header: /cvsroot/nsnam/ns-2/tcp/tcp-nc.cc,v 1.37 2014/03/03 18:58:12 johnh Exp $ (NCSU/IBM)";
#endif

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <math.h>
#include <vector>
#include <algorithm>

#include "ip.h"
#include "tcp.h"
#include "flags.h"

static class TcpNcClass : public TclClass {
    public:
        TcpNcClass() : TclClass("Agent/TCP/NC") {}
        TclObject* create(int, const char*const*) {
            return (new TcpNcAgent());
        }
} class_tcpnc;


TcpNcAgent::TcpNcAgent() : VegasTcpAgent() {
    nc_coding_window_ = NULL;
    nc_send_times_ = new std::vector<double>();
    nc_coding_window_size_ = 0;
}

TcpNcAgent::~TcpNcAgent() {
    if (nc_coding_window_) {
        delete []nc_coding_window_;
    }
    nc_send_times_->clear();
    delete nc_send_times_;
}

void TcpNcAgent::delay_bind_init_all() {
    delay_bind_init_one("nc_r_");
    VegasTcpAgent::delay_bind_init_all();
    reset();
}

int TcpNcAgent::delay_bind_dispatch(const char *varName, const char *localName, TclObject *tracer) {
    if (delay_bind(varName, localName, "nc_r_", &nc_r_, tracer)) return TCL_OK;
    return VegasTcpAgent::delay_bind_dispatch(varName, localName, tracer);
}

void TcpNcAgent::reset() {
    nc_tx_serial_num_ = 0;
    nc_num_ = 0;
    nc_coding_window_size_ = 0;

    VegasTcpAgent::reset();
}

void TcpNcAgent::recv_newack_helper(Packet *pkt) {
    // Rewrite Send time to trick vegas
    hdr_tcp *tcph = hdr_tcp::access(pkt);
    double nc_send_time = nc_send_times_->at(tcph->nc_tx_serial_num());
    v_sendtime_[tcph->seqno() % v_maxwnd_] = nc_send_time;

    int k = last_ack_;
    for(int range = tcph->seqno() - last_ack_; range > 0; range--) {
        k = (k + 1) % v_maxwnd_;
        Packet::free(nc_coding_window_[k]);
        nc_coding_window_[k] = NULL;
        nc_coding_window_size_--;
    }

    return VegasTcpAgent::recv_newack_helper(pkt);
}

void TcpNcAgent::send(Packet* p, Handler* h) {
    if (!nc_coding_window_) {
        nc_coding_window_ = new Packet*[v_maxwnd_];
        for(int i=0;i<v_maxwnd_;i++) {
            nc_coding_window_[i] = NULL;
        }
    }

    hdr_tcp *tcph = hdr_tcp::access(p);
    int index = tcph->seqno() % v_maxwnd_;
    if (!nc_coding_window_[index]) {
        nc_coding_window_size_++;
        nc_coding_window_[index] = p;
    }

    // nc_num_ += nc_r_; // TCP/NC does not specify how nc_num_ should be adjusted
    int r;
    // for (r = 0; r < floor(nc_num_); r++) {
    for (r = 0; r < nc_r_; r++) {
        nc_tx_serial_num_++;

        // int data_size = p->userdata()->size();
        // unsigned char *data = new unsigned char[data_size];
        int *coefficients = new int[nc_coding_window_size_];
        // int d;
        // for (i = 0; i < data_size; i++) {
        //     data[i] = '\0';
        // }

        for (int i = 0; i < nc_coding_window_size_; i++) {
            // Packet *it = nc_coding_window_[(last_ack_ + 1 + i) % v_maxwnd_];
            // unsigned char *p_data = it->accessdata();
            int c = (rand() % 255) + 1; // TODO: Bind Fieldsize for testing
            coefficients[i] = c;
            // for (d = 0; d < data_size; d++) {
            //     data[d] = data[d] + (c * p_data[d]);
            // }
        }

        Packet* linear_combination = p->copy();
        tcph = hdr_tcp::access(linear_combination);
        tcph->nc_tx_serial_num() = nc_tx_serial_num_;
        tcph->nc_coding_wnd_size() = nc_coding_window_size_;
        tcph->nc_coefficients_ = coefficients;

        // ((PacketData*)linear_combination->userdata())->set_data(data);

        // record send time for nc_tx_serial_num
        nc_send_times_->push_back(vegastime());

        VegasTcpAgent::send(linear_combination, h);
    }
}
