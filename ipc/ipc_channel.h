// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IPC_IPC_CHANNEL_H_
#define IPC_IPC_CHANNEL_H_

#include <string>

#include "ipc/ipc_common.h"
#include "ipc/ipc_channel_handle.h"
#include "ipc/ipc_message.h"
#include "ipc/ipc_sender.h"

namespace IPC {

class Listener;
class Thread;

//------------------------------------------------------------------------------
// See
// http://www.chromium.org/developers/design-documents/inter-process-communication
// for overview of IPC in Chromium.

// Channels are implemented using named pipes on Windows, and
// socket pairs (or in some special cases unix domain sockets) on POSIX.
// On Windows we access pipes in various processes by name.
// On POSIX we pass file descriptors to child processes and assign names to them
// in a lookup table.
// In general on POSIX we do not use unix domain sockets due to security
// concerns and the fact that they can leave garbage around the file system
// (MacOS does not support abstract named unix domain sockets).
// You can use unix domain sockets if you like on POSIX by constructing the
// the channel with the mode set to one of the NAMED modes. NAMED modes are
// currently used by automation and service processes.

class Channel : public Sender {
 public:
  // The Hello message is internal to the Channel class.  It is sent
  // by the peer when the channel is connected.  The message contains
  // just the process id (pid).  The message has a special routing_id
  // (MSG_ROUTING_NONE) and type (HELLO_MESSAGE_TYPE).
  enum {
    HELLO_MESSAGE_TYPE = kuint16max  // Maximum value of message type (uint16),
                                     // to avoid conflicting with normal
                                     // message types, which are enumeration
                                     // constants starting from 0.
  };

  // The maximum message size in bytes. Attempting to receive a message of this
  // size or bigger results in a channel error.
  static const size_t kMaximumMessageSize = 128 * 1024 * 1024;

  // Amount of data to read at once from the pipe.
  static const size_t kReadBufferSize = 4 * 1024;

  // Initialize a Channel.
  //
  // |channel_handle| identifies the communication Channel. For POSIX, if
  // the file descriptor in the channel handle is != -1, the channel takes
  // ownership of the file descriptor and will close it appropriately, otherwise
  // it will create a new descriptor internally.
  // |mode| specifies whether this Channel is to operate in server mode or
  // client mode.  In server mode, the Channel is responsible for setting up the
  // IPC object, whereas in client mode, the Channel merely connects to the
  // already established IPC object.
  // |listener| receives a callback on the current thread for each newly
  // received message.
  //
  Channel(const IPC::ChannelHandle &channel_handle,
	  Listener* listener, Thread* runner);

  virtual ~Channel();

  // Connect the pipe.  On the server side, this will initiate
  // waiting for connections.  On the client, it attempts to
  // connect to a pre-existing pipe.  Note, calling Connect()
  // will not block the calling thread and may complete
  // asynchronously.
  bool Connect();

  // Close this Channel explicitly.  May be called multiple times.
  // On POSIX calling close on an IPC channel that listens for connections will
  // cause it to close any accepted connections, and it will stop listening for
  // new connections. If you just want to close the currently accepted
  // connection and listen for new ones, use ResetToAcceptingConnectionState.
  void Close();

  // Get the process ID for the connected peer.
  //
  // Returns base::kNullProcessId if the peer is not connected yet. Watch out
  // for race conditions. You can easily get a channel to another process, but
  // if your process has not yet processed the "hello" message from the remote
  // side, this will fail. You should either make sure calling this is either
  // in response to a message from the remote side (which guarantees that it's
  // been connected), or you wait for the "connected" notification on the
  // listener.
  DWORD peer_pid() const;

  // Send a message over the Channel to the listener on the other end.
  //
  // |message| must be allocated using operator new.  This object will be
  // deleted once the contents of the Message have been sent.
  virtual bool Send(Message* message) override;

  // Returns true if a named server channel is initialized on the given channel
  // ID. Even if true, the server may have already accepted a connection.
  static bool IsNamedServerInitialized(const std::string& channel_id);

  // Generates a channel ID that's non-predictable and unique.
  static std::string GenerateUniqueRandomChannelID();

  // Generates a channel ID that, if passed to the client as a shared secret,
  // will validate that the client's authenticity. On platforms that do not
  // require additional this is simply calls GenerateUniqueRandomChannelID().
  // For portability the prefix should not include the \ character.
  static std::string GenerateVerifiedChannelID(const std::string& prefix);

 protected:
  // Used in Chrome by the TestSink to provide a dummy channel implementation
  // for testing. TestSink overrides the "interesting" functions in Channel so
  // no actual implementation is needed. This will cause un-overridden calls to
  // segfault. Do not use outside of test code!
  Channel() : channel_impl_(0) { }

 private:
  // PIMPL to which all channel calls are delegated.
  class ChannelImpl;
  ChannelImpl *channel_impl_;
};

}  // namespace IPC

#endif  // IPC_IPC_CHANNEL_H_
