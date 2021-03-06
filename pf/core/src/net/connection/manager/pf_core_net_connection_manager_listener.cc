#include "pf/basic/logger.h"
#include "pf/sys/assert.h"
#include "pf/net/socket/listener.h"
#include "pf/net/packet/factorymanager.h"
#include "pf/net/connection/manager/listener.h"

using namespace pf_net::connection::manager;

Listener::Listener() :
  listener_socket_{nullptr} {
  //do nothing
}

Listener::~Listener() {
  //do nothing
}

bool Listener::init(uint16_t max_size, uint16_t port, const std::string &ip) {
  std::unique_ptr<socket::Listener> 
    pointer{new socket::Listener(port, ip)};
  if (is_null(pointer)) return false;
  listener_socket_ = std::move(pointer);
  listener_socket_->set_nonblocking();
  Assert(listener_socket_->get_id() != SOCKET_INVALID);
  return Basic::init(max_size);
}


pf_net::connection::Basic *Listener::accept() {
  uint32_t step = 0;
  bool result = false;
  pf_net::connection::Basic *newconnection = nullptr;
  newconnection = pool_->create();
  if (nullptr == newconnection) return nullptr;
  step = 5;
  newconnection->init(protocol());
  newconnection->clear();
  int32_t socketid = SOCKET_INVALID;
  step = 10;
  try {
    //accept client socket
    result = listener_socket_->accept(newconnection->socket());
    if (!result) {
      step = 15;
      goto EXCEPTION;
    }
  } catch(...) {
    step += 1000;
    goto EXCEPTION;
  }
  try {
    step = 30;
    socketid = newconnection->socket()->get_id();
    if (SOCKET_INVALID == socketid) {
      Assert(false);
      goto EXCEPTION;
    }
    step = 40;
    result = newconnection->socket()->set_nonblocking();
    if (!result) {
      Assert(false);
      goto EXCEPTION;
    }
    step = 50;
    if (newconnection->socket()->error()) {
      Assert(false);
      goto EXCEPTION;
    }
    step = 60;
    result = newconnection->socket()->set_linger(0);
    if (!result) {
      Assert(false);
      goto EXCEPTION;
    }
    step = 70;
    try {
      result = add(newconnection);
      if (!result) {
        Assert(false);
        goto EXCEPTION;
      }
    } catch(...) {
      step += 10000;
      goto EXCEPTION;
    }
  } catch(...) {
    step += 100000;
  }
  newconnection->set_disconnect(false); //connect is success
  FAST_LOG(NET_MODULENAME,
           "[net.connection.manager] (Interface::accept)"
           " host: %s id: %d socketid: %d",
           newconnection->socket()->host(),
           newconnection->get_id(),
           newconnection->socket()->get_id());
  return newconnection;
EXCEPTION:
  newconnection->clear();
  pool_->remove(newconnection->get_id());
  return nullptr;
}
