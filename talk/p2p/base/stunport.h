/*
 * libjingle
 * Copyright 2004--2005, Google Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  1. Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright notice,
 *     this list of conditions and the following disclaimer in the documentation
 *     and/or other materials provided with the distribution.
 *  3. The name of the author may not be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef TALK_P2P_BASE_STUNPORT_H_
#define TALK_P2P_BASE_STUNPORT_H_

#include <string>

#include "talk/base/asyncpacketsocket.h"
#include "talk/p2p/base/port.h"
#include "talk/p2p/base/stunrequest.h"

// TODO(mallinath) - Rename stunport.cc|h to udpport.cc|h.
namespace talk_base {
class AsyncResolver;
class SignalThread;
}

namespace cricket {

// Communicates using the address on the outside of a NAT.
class UDPPort : public Port {
 public:
  static UDPPort* Create(talk_base::Thread* thread,
                         talk_base::Network* network,
                         talk_base::AsyncPacketSocket* socket,
                         const std::string& username,
                         const std::string& password) {
    UDPPort* port = new UDPPort(thread, network, socket, username, password);
    if (!port->Init()) {
      delete port;
      port = NULL;
    }
    return port;
  }

  static UDPPort* Create(talk_base::Thread* thread,
                         talk_base::PacketSocketFactory* factory,
                         talk_base::Network* network,
                         const talk_base::IPAddress& ip,
                         int min_port, int max_port,
                         const std::string& username,
                         const std::string& password) {
    UDPPort* port = new UDPPort(thread, factory, network,
                                 ip, min_port, max_port,
                                 username, password);
    if (!port->Init()) {
      delete port;
      port = NULL;
    }
    return port;
  }
  virtual ~UDPPort();

  talk_base::SocketAddress GetLocalAddress() const {
    return socket_->GetLocalAddress();
  }

  const talk_base::SocketAddress& server_addr() const { return server_addr_; }
  void set_server_addr(const talk_base::SocketAddress& addr) {
    server_addr_ = addr;
  }

  const talk_base::SocketAddress& server_addr2() const { return server_addr2_; }
  void set_server_addr2(const talk_base::SocketAddress& addr) {
    server_addr2_ = addr;
  }

  virtual void PrepareAddress();

  virtual Connection* CreateConnection(const Candidate& address,
                                       CandidateOrigin origin);
  virtual int SetOption(talk_base::Socket::Option opt, int value);
  virtual int GetError();

  virtual bool HandleIncomingPacket(
      talk_base::AsyncPacketSocket* socket, const char* data, size_t size,
      const talk_base::SocketAddress& remote_addr) {
    // All packets given to UDP port will be consumed.
    OnReadPacket(socket, data, size, remote_addr);
    return true;
  }

 protected:
  UDPPort(talk_base::Thread* thread, talk_base::PacketSocketFactory* factory,
          talk_base::Network* network, const talk_base::IPAddress& ip,
          int min_port, int max_port,
          const std::string& username, const std::string& password);

  UDPPort(talk_base::Thread* thread, talk_base::Network* network,
          talk_base::AsyncPacketSocket* socket,
          const std::string& username, const std::string& password);

  bool Init();

  virtual int SendTo(const void* data, size_t size,
                     const talk_base::SocketAddress& addr, bool payload);

  void OnLocalAddressReady(talk_base::AsyncPacketSocket* socket,
                           const talk_base::SocketAddress& address);
  void OnReadPacket(talk_base::AsyncPacketSocket* socket,
                    const char* data, size_t size,
                    const talk_base::SocketAddress& remote_addr);

  // This method will send STUN binding request if STUN server address is set.
  void MaybePrepareStunCandidate();

  void SendStunBindingRequest();

 private:
  // DNS resolution of the STUN server.
  void ResolveStunAddress();
  void OnResolveResult(talk_base::SignalThread* thread);
  // Sends STUN requests to the server.
  void OnSendPacket(const void* data, size_t size, StunRequest* req);

  talk_base::SocketAddress server_addr_;
  talk_base::SocketAddress server_addr2_;
  StunRequestManager requests_;
  talk_base::AsyncPacketSocket* socket_;
  int error_;
  talk_base::AsyncResolver* resolver_;

  friend class StunBindingRequest;
};

class StunPort : public UDPPort {
 public:
  static StunPort* Create(talk_base::Thread* thread,
                          talk_base::PacketSocketFactory* factory,
                          talk_base::Network* network,
                          const talk_base::IPAddress& ip,
                          int min_port, int max_port,
                          const std::string& username,
                          const std::string& password,
                          const talk_base::SocketAddress& server_addr) {
    StunPort* port = new StunPort(thread, factory, network,
                                  ip, min_port, max_port,
                                  username, password, server_addr);
    if (!port->Init()) {
      delete port;
      port = NULL;
    }
    return port;
  }

  virtual ~StunPort() {}

  virtual void PrepareAddress() {
    SendStunBindingRequest();
  }

 protected:
  StunPort(talk_base::Thread* thread, talk_base::PacketSocketFactory* factory,
           talk_base::Network* network, const talk_base::IPAddress& ip,
           int min_port, int max_port,
           const std::string& username, const std::string& password,
           const talk_base::SocketAddress& server_address)
     : UDPPort(thread, factory, network, ip, min_port, max_port, username,
               password) {
    // UDPPort will set these to local udp, updating these to STUN.
    set_type(STUN_PORT_TYPE);
    set_server_addr(server_address);
  }
};

}  // namespace cricket

#endif  // TALK_P2P_BASE_STUNPORT_H_
