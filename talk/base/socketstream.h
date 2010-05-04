/*
 * libjingle
 * Copyright 2005, Google Inc.
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

#ifndef TALK_BASE_SOCKET_STREAM_H__
#define TALK_BASE_SOCKET_STREAM_H__

#include "talk/base/asyncsocket.h"
#include "talk/base/common.h"
#include "talk/base/stream.h"

namespace talk_base {

///////////////////////////////////////////////////////////////////////////////

class SocketStream : public StreamInterface, public sigslot::has_slots<> {
 public:
  SocketStream(AsyncSocket* socket) : socket_(NULL) {
    Attach(socket);
  }
  virtual ~SocketStream() { delete socket_; }

  void Attach(AsyncSocket* socket) {
    if (socket_)
      delete socket_;
    socket_ = socket;
    if (socket_) {
      socket_->SignalConnectEvent.connect(this, &SocketStream::OnConnectEvent);
      socket_->SignalReadEvent.connect(this,    &SocketStream::OnReadEvent);
      socket_->SignalWriteEvent.connect(this,   &SocketStream::OnWriteEvent);
      socket_->SignalCloseEvent.connect(this,   &SocketStream::OnCloseEvent);
    }
  }

  AsyncSocket* Detach() {
    AsyncSocket* socket = socket_;
    if (socket_) {
      socket_->SignalConnectEvent.disconnect(this);
      socket_->SignalReadEvent.disconnect(this);
      socket_->SignalWriteEvent.disconnect(this);
      socket_->SignalCloseEvent.disconnect(this);
      socket_ = NULL;
    }
    return socket;
  }

  AsyncSocket* GetSocket() { return socket_; }

  virtual StreamState GetState() const {
    ASSERT(socket_ != NULL);
    switch (socket_->GetState()) {
      case Socket::CS_CONNECTED:
        return SS_OPEN;
      case Socket::CS_CONNECTING:
        return SS_OPENING;
      case Socket::CS_CLOSED:
      default:
        return SS_CLOSED;
    }
  }

  virtual StreamResult Read(void* buffer, size_t buffer_len,
                            size_t* read, int* error) {
    ASSERT(socket_ != NULL);
    int result = socket_->Recv(buffer, buffer_len);
    if (result < 0) {
      if (socket_->IsBlocking())
        return SR_BLOCK;
      if (error)
        *error = socket_->GetError();
      return SR_ERROR;
    }
    if ((result > 0) || (buffer_len == 0)) {
      if (read)
        *read = result;
      return SR_SUCCESS;
    }
    return SR_EOS;
  }

  virtual StreamResult Write(const void* data, size_t data_len,
                             size_t* written, int* error) {
    ASSERT(socket_ != NULL);
    int result = socket_->Send(data, data_len);
    if (result < 0) {
      if (socket_->IsBlocking())
        return SR_BLOCK;
      if (error)
        *error = socket_->GetError();
      return SR_ERROR;
    }
    if (written)
      *written = result;
    return SR_SUCCESS;
  }

  virtual void Close() { ASSERT(socket_ != NULL); socket_->Close(); }

 private:
  void OnConnectEvent(AsyncSocket* socket) {
    ASSERT(socket == socket_);
    SignalEvent(this, SE_OPEN | SE_READ | SE_WRITE, 0);
  }
  void OnReadEvent(AsyncSocket* socket) {
    ASSERT(socket == socket_);
    SignalEvent(this, SE_READ, 0);
  }
  void OnWriteEvent(AsyncSocket* socket) {
    ASSERT(socket == socket_);
    SignalEvent(this, SE_WRITE, 0);
  }
  void OnCloseEvent(AsyncSocket* socket, int err) {
    ASSERT(socket == socket_);
    SignalEvent(this, SE_CLOSE, err);
  }
  
  AsyncSocket* socket_;

  DISALLOW_EVIL_CONSTRUCTORS(SocketStream);
};

///////////////////////////////////////////////////////////////////////////////

}  // namespace talk_base

#endif  // TALK_BASE_SOCKET_STREAM_H__
