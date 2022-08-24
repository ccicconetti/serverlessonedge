/*
              __ __ __
             |__|__|  | __
             |  |  |  ||__|
  ___ ___ __ |  |  |  |
 |   |   |  ||  |  |  |    Ubiquitous Internet @ IIT-CNR
 |   |   |  ||  |  |  |    C++ edge computing libraries and tools
 |_______|__||__|__|__|    https://github.com/ccicconetti/serverlessonedge

Licensed under the MIT License <http://opensource.org/licenses/MIT>
Copyright (c) 2021 C. Cicconetti <https://ccicconetti.github.io/>

Permission is hereby  granted, free of charge, to any  person obtaining a copy
of this software and associated  documentation files (the "Software"), to deal
in the Software  without restriction, including without  limitation the rights
to  use, copy,  modify, merge,  publish, distribute,  sublicense, and/or  sell
copies  of  the Software,  and  to  permit persons  to  whom  the Software  is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE  IS PROVIDED "AS  IS", WITHOUT WARRANTY  OF ANY KIND,  EXPRESS OR
IMPLIED,  INCLUDING BUT  NOT  LIMITED TO  THE  WARRANTIES OF  MERCHANTABILITY,
FITNESS FOR  A PARTICULAR PURPOSE AND  NONINFRINGEMENT. IN NO EVENT  SHALL THE
AUTHORS  OR COPYRIGHT  HOLDERS  BE  LIABLE FOR  ANY  CLAIM,  DAMAGES OR  OTHER
LIABILITY, WHETHER IN AN ACTION OF  CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE  OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

//
// the code in this file is heavily inspired from (a.k.a. copied with minor
// edits) the `echo` sample bundled with the Meta's mvfst library:
//   https://github.com/facebookincubator/mvfst
//

#include "gtest/gtest.h"

#include "Support/queue.h"

#include <chrono>
#include <quic/QuicConstants.h>
#include <quic/QuicException.h>
#include <quic/api/QuicSocket.h>
#include <quic/client/QuicClientTransport.h>
#include <quic/codec/Types.h>
#include <quic/common/BufUtil.h>
#include <quic/fizz/client/handshake/FizzClientQuicHandshakeContext.h>
#include <quic/server/QuicServer.h>
#include <quic/server/QuicServerTransport.h>
#include <quic/server/QuicSharedUDPSocketFactory.h>
#include <quic/state/QuicTransportStatsCallback.h>

#include <fizz/protocol/CertificateVerifier.h>

#include <folly/fibers/Baton.h>
#include <folly/io/async/EventBase.h>
#include <folly/io/async/ScopedEventBaseThread.h>
#include <folly/ssl/OpenSSLCertUtils.h>
#include <folly/ssl/OpenSSLPtrTypes.h>

#include <glog/logging.h>

#include <iostream>
#include <string>
#include <thread>

namespace uiiit {
namespace mvfst {

class LogQuicStats : public quic::QuicTransportStatsCallback
{
 public:
  explicit LogQuicStats(const std::string& prefix)
      : prefix_(prefix + " ") {
  }

  ~LogQuicStats() override = default;

  void onPacketReceived() override {
    VLOG(2) << prefix_ << "onPacketReceived";
  }

  void onDuplicatedPacketReceived() override {
    VLOG(2) << prefix_ << "onDuplicatedPacketReceived";
  }

  void onOutOfOrderPacketReceived() override {
    VLOG(2) << prefix_ << "onOutOfOrderPacketReceived";
  }

  void onPacketProcessed() override {
    VLOG(2) << prefix_ << "onPacketProcessed";
  }

  void onPacketSent() override {
    VLOG(2) << prefix_ << "onPacketSent";
  }

  void onDSRPacketSent(size_t pktSize) override {
    VLOG(2) << prefix_ << "onDSRPacketSent size=" << pktSize;
  }

  void onPacketRetransmission() override {
    VLOG(2) << prefix_ << "onPacketRetransmission";
  }

  void onPacketLoss() override {
    VLOG(2) << prefix_ << "onPacketLoss";
  }

  void onPacketSpuriousLoss() override {
    VLOG(2) << prefix_ << "onPacketSpuriousLoss";
  }

  void onPersistentCongestion() override {
    VLOG(2) << prefix_ << "onPersistentCongestion";
  }

  void onPacketDropped(PacketDropReason reason) override {
    VLOG(2) << prefix_ << "onPacketDropped reason=" << toString(reason);
  }

  void onPacketForwarded() override {
    VLOG(2) << prefix_ << "onPacketForwarded";
  }

  void onForwardedPacketReceived() override {
    VLOG(2) << prefix_ << "onForwardedPacketReceived";
  }

  void onForwardedPacketProcessed() override {
    VLOG(2) << prefix_ << "onForwardedPacketProcessed";
  }

  void onClientInitialReceived(quic::QuicVersion version) override {
    VLOG(2) << prefix_
            << "onClientInitialReceived, version: " << quic::toString(version);
  }

  void onConnectionRateLimited() override {
    VLOG(2) << prefix_ << "onConnectionRateLimited";
  }

  void onConnectionWritableBytesLimited() override {
    VLOG(2) << prefix_ << "onConnectionWritableBytesLimited";
  }

  void onNewTokenReceived() override {
    VLOG(2) << prefix_ << "onNewTokenReceived";
  }

  void onNewTokenIssued() override {
    VLOG(2) << prefix_ << "onNewTokenIssued";
  }

  void onTokenDecryptFailure() override {
    VLOG(2) << prefix_ << "onTokenDecryptFailure";
  }

  // connection level metrics:
  void onNewConnection() override {
    VLOG(2) << prefix_ << "onNewConnection";
  }

  void onConnectionClose(
      folly::Optional<quic::QuicErrorCode> code = folly::none) override {
    VLOG(2) << prefix_ << "onConnectionClose reason="
            << quic::toString(code.value_or(quic::LocalErrorCode::NO_ERROR));
  }

  void onConnectionCloseZeroBytesWritten() override {
    VLOG(2) << prefix_ << "onConnectionCloseZeroBytesWritten";
  }

  // stream level metrics
  void onNewQuicStream() override {
    VLOG(2) << prefix_ << "onNewQuicStream";
  }

  void onQuicStreamClosed() override {
    VLOG(2) << prefix_ << "onQuicStreamClosed";
  }

  void onQuicStreamReset(quic::QuicErrorCode code) override {
    VLOG(2) << prefix_ << "onQuicStreamReset reason=" << quic::toString(code);
  }

  // flow control / congestion control / loss recovery related metrics
  void onConnFlowControlUpdate() override {
    VLOG(2) << prefix_ << "onConnFlowControlUpdate";
  }

  void onConnFlowControlBlocked() override {
    VLOG(2) << prefix_ << "onConnFlowControlBlocked";
  }

  void onStatelessReset() override {
    VLOG(2) << prefix_ << "onStatelessReset";
  }

  void onStreamFlowControlUpdate() override {
    VLOG(2) << prefix_ << "onStreamFlowControlUpdate";
  }

  void onStreamFlowControlBlocked() override {
    VLOG(2) << prefix_ << "onStreamFlowControlBlocked";
  }

  void onCwndBlocked() override {
    VLOG(2) << prefix_ << "onCwndBlocked";
  }

  void onNewCongestionController(quic::CongestionControlType type) override {
    VLOG(2) << prefix_ << "onNewCongestionController type="
            << quic::congestionControlTypeToString(type);
  }

  // Probe timeout counter (aka loss timeout counter)
  void onPTO() override {
    VLOG(2) << prefix_ << "onPTO";
  }

  // metrics to track bytes read from / written to wire
  void onRead(size_t bufSize) override {
    VLOG(2) << prefix_ << "onRead size=" << bufSize;
  }

  void onWrite(size_t bufSize) override {
    VLOG(2) << prefix_ << "onWrite size=" << bufSize;
  }

  void onUDPSocketWriteError(SocketErrorType errorType) override {
    VLOG(2) << prefix_
            << "onUDPSocketWriteError errorType=" << toString(errorType);
  }

  void onConnectionD6DStarted() override {
    VLOG(2) << prefix_ << "onConnectionD6DStarted";
  }

  void onConnectionPMTURaised() override {
    VLOG(2) << prefix_ << "onConnectionPMTURaised";
  }

  void onConnectionPMTUBlackholeDetected() override {
    VLOG(2) << prefix_ << "onConnectionPMTUBlackholeDetected";
  }

  void onConnectionPMTUUpperBoundDetected() override {
    VLOG(2) << prefix_ << "onConnectionPMTUUpperBoundDetected";
  }

  void onTransportKnobApplied(quic::TransportKnobParamId knobType) override {
    VLOG(2) << prefix_
            << "onTransportKnobApplied knobType=" << knobType._to_string();
  }

  void onTransportKnobError(quic::TransportKnobParamId knobType) override {
    VLOG(2) << prefix_
            << "onTransportKnboError knobType=" << knobType._to_string();
  }

  void onTransportKnobOutOfOrder(quic::TransportKnobParamId knobType) override {
    VLOG(2) << prefix_
            << "onTransportKnobOutOfOrder knobType=" << knobType._to_string();
  }

  void onServerUnfinishedHandshake() override {
    VLOG(2) << prefix_ << "onServerUnfinishedHandshake";
  }

  void onZeroRttBuffered() override {
    VLOG(2) << prefix_ << "onZeroRttBuffered";
  }

  void onZeroRttBufferedPruned() override {
    VLOG(2) << prefix_ << "onZeroRttBufferedPruned";
  }

  void onZeroRttAccepted() override {
    VLOG(2) << prefix_ << "onZeroRttAccepted";
  }

  void onZeroRttRejected() override {
    VLOG(2) << prefix_ << "onZeroRttRejected";
  }

  void onDatagramRead(size_t datagramSize) override {
    VLOG(2) << prefix_ << "onDatagramRead size=" << datagramSize;
  }

  void onDatagramWrite(size_t datagramSize) override {
    VLOG(2) << prefix_ << "onDatagramWrite size=" << datagramSize;
  }

  void onDatagramDroppedOnWrite() override {
    VLOG(2) << prefix_ << "onDatagramDroppedOnWrite";
  }

  void onDatagramDroppedOnRead() override {
    VLOG(2) << prefix_ << "onDatagramDroppedOnRead";
  }

  void onShortHeaderPadding(size_t padSize) override {
    VLOG(2) << prefix_ << "onShortHeaderPadding size=" << padSize;
  }

  void onPacerTimerLagged() override {
    VLOG(2) << prefix_ << __func__;
  }

 private:
  std::string prefix_;
};

class LogQuicStatsFactory : public quic::QuicTransportStatsCallbackFactory
{
 public:
  ~LogQuicStatsFactory() override = default;

  std::unique_ptr<quic::QuicTransportStatsCallback> make() override {
    return std::make_unique<LogQuicStats>("server");
  }
};

class TestCertificateVerifier : public fizz::CertificateVerifier
{
 public:
  ~TestCertificateVerifier() override = default;

  std::shared_ptr<const folly::AsyncTransportCertificate>
  verify(const std::vector<std::shared_ptr<const fizz::PeerCert>>& certs)
      const override {
    return certs.front();
  }

  [[nodiscard]] std::vector<fizz::Extension>
  getCertificateRequestExtensions() const override {
    return std::vector<fizz::Extension>();
  }
};

static constexpr folly::StringPiece kP256Certificate = R"(
-----BEGIN CERTIFICATE-----
MIIB7jCCAZWgAwIBAgIJAMVp7skBzobZMAoGCCqGSM49BAMCMFQxCzAJBgNVBAYT
AlVTMQswCQYDVQQIDAJOWTELMAkGA1UEBwwCTlkxDTALBgNVBAoMBEZpenoxDTAL
BgNVBAsMBEZpenoxDTALBgNVBAMMBEZpenowHhcNMTcwNDA0MTgyOTA5WhcNNDEx
MTI0MTgyOTA5WjBUMQswCQYDVQQGEwJVUzELMAkGA1UECAwCTlkxCzAJBgNVBAcM
Ak5ZMQ0wCwYDVQQKDARGaXp6MQ0wCwYDVQQLDARGaXp6MQ0wCwYDVQQDDARGaXp6
MFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAEnYe8rdtl2Nz234sUipZ5tbcQ2xnJ
Wput//E0aMs1i04h0kpcgmESZY67ltZOKYXftBwZSDNDkaSqgbZ4N+Lb8KNQME4w
HQYDVR0OBBYEFDxbi6lU2XUvrzyK1tGmJEncyqhQMB8GA1UdIwQYMBaAFDxbi6lU
2XUvrzyK1tGmJEncyqhQMAwGA1UdEwQFMAMBAf8wCgYIKoZIzj0EAwIDRwAwRAIg
NJt9NNcTL7J1ZXbgv6NsvhcjM3p6b175yNO/GqfvpKUCICXFCpHgqkJy8fUsPVWD
p9fO4UsXiDUnOgvYFDA+YtcU
-----END CERTIFICATE-----
)";
constexpr folly::StringPiece        kP256Key         = R"(
-----BEGIN EC PRIVATE KEY-----
MHcCAQEEIHMPeLV/nP/gkcgU2weiXl198mEX8RbFjPRoXuGcpxMXoAoGCCqGSM49
AwEHoUQDQgAEnYe8rdtl2Nz234sUipZ5tbcQ2xnJWput//E0aMs1i04h0kpcgmES
ZY67ltZOKYXftBwZSDNDkaSqgbZ4N+Lb8A==
-----END EC PRIVATE KEY-----
)";

class EchoClient : public quic::QuicSocket::ConnectionSetupCallback,
                   public quic::QuicSocket::ConnectionCallback,
                   public quic::QuicSocket::ReadCallback,
                   public quic::QuicSocket::WriteCallback,
                   public quic::QuicSocket::DatagramCallback
{
 public:
  EchoClient(const std::string&      host,
             uint16_t                port,
             std::list<std::string>& responses)
      : host_(host)
      , port_(port)
      , responses_(responses) {
    // noop
  }

  void readAvailable(quic::StreamId streamId) noexcept override {
    auto readData = quicClient_->read(streamId, 0);
    if (readData.hasError()) {
      LOG(ERROR) << "EchoClient failed read from stream=" << streamId
                 << ", error=" << (uint32_t)readData.error();
    }
    auto copy = readData->first->clone();
    if (recvOffsets_.find(streamId) == recvOffsets_.end()) {
      recvOffsets_[streamId] = copy->length();
    } else {
      recvOffsets_[streamId] += copy->length();
    }
    responses_.emplace_back(copy->moveToFbString().toStdString());
    LOG(INFO) << "Client received data=" << responses_.back()
              << " on stream=" << streamId;
  }

  void readError(quic::StreamId  streamId,
                 quic::QuicError error) noexcept override {
    LOG(ERROR) << "EchoClient failed read from stream=" << streamId
               << ", error=" << toString(error);
    // A read error only terminates the ingress portion of the stream state.
    // Your application should probably terminate the egress portion via
    // resetStream
  }

  void onNewBidirectionalStream(quic::StreamId id) noexcept override {
    LOG(INFO) << "EchoClient: new bidirectional stream=" << id;
    quicClient_->setReadCallback(id, this);
  }

  void onNewUnidirectionalStream(quic::StreamId id) noexcept override {
    LOG(INFO) << "EchoClient: new unidirectional stream=" << id;
    quicClient_->setReadCallback(id, this);
  }

  void onStopSending(quic::StreamId id,
                     quic::ApplicationErrorCode /*error*/) noexcept override {
    VLOG(10) << "EchoClient got StopSending stream id=" << id;
  }

  void onConnectionEnd() noexcept override {
    LOG(INFO) << "EchoClient connection end";
  }

  void onConnectionSetupError(quic::QuicError error) noexcept override {
    onConnectionError(std::move(error));
  }

  void onConnectionError(quic::QuicError error) noexcept override {
    LOG(ERROR) << "EchoClient error: " << toString(error.code)
               << "; errStr=" << error.message;
    startDone_.post();
  }

  void onTransportReady() noexcept override {
    startDone_.post();
  }

  void onStreamWriteReady(quic::StreamId id,
                          uint64_t       maxToSend) noexcept override {
    LOG(INFO) << "EchoClient socket is write ready with maxToSend="
              << maxToSend;
    sendMessage(id, pendingOutput_[id]);
  }

  void onStreamWriteError(quic::StreamId  id,
                          quic::QuicError error) noexcept override {
    LOG(ERROR) << "EchoClient write error with stream=" << id
               << " error=" << toString(error);
  }

  void onDatagramsAvailable() noexcept override {
    auto res = quicClient_->readDatagrams();
    if (res.hasError()) {
      LOG(ERROR) << "EchoClient failed reading datagrams; error="
                 << res.error();
      return;
    }
    for (const auto& datagram : *res) {
      LOG(INFO) << "Client received datagram ="
                << datagram.bufQueue()
                       .front()
                       ->cloneCoalesced()
                       ->moveToFbString()
                       .toStdString();
    }
  }

  void start(std::string token, support::Queue<std::string>* msgQueue) {
    folly::ScopedEventBaseThread networkThread("EchoClientThread");
    auto                         evb = networkThread.getEventBase();
    folly::SocketAddress         addr(host_.c_str(), port_);

    evb->runInEventBaseThreadAndWait([&] {
      auto sock = std::make_unique<folly::AsyncUDPSocket>(evb);
      auto fizzClientContext =
          quic::FizzClientQuicHandshakeContext::Builder()
              .setCertificateVerifier(
                  std::make_unique<TestCertificateVerifier>())
              .build();
      quicClient_ = std::make_shared<quic::QuicClientTransport>(
          evb, std::move(sock), std::move(fizzClientContext));
      quicClient_->setHostname("echo.com");
      quicClient_->addNewPeerAddress(addr);
      if (!token.empty()) {
        quicClient_->setNewToken(token);
      }

      quic::TransportSettings settings;
      settings.datagramConfig.enabled      = false;
      settings.selfActiveConnectionIdLimit = 10; // max active concurrent conns
      settings.disableMigration            = true;
      quicClient_->setTransportSettings(settings);

      quicClient_->setTransportStatsCallback(
          std::make_shared<LogQuicStats>("client"));

      LOG(INFO) << "EchoClient connecting to " << addr.describe();
      quicClient_->start(this, this);
    });

    startDone_.wait();

    std::string message;
    bool        closed = false;
    auto        client = quicClient_;

    auto sendMessageInStream = [&]() {
      if (message == "/close") {
        quicClient_->close(folly::none);
        closed = true;
        return;
      }

      // create new stream for each message
      auto streamId = client->createBidirectionalStream().value();
      client->setReadCallback(streamId, this);
      pendingOutput_[streamId].append(folly::IOBuf::copyBuffer(message));
      sendMessage(streamId, pendingOutput_[streamId]);
    };

    if (msgQueue == nullptr) {
      // loop until Ctrl+D
      while (!closed && std::getline(std::cin, message)) {
        if (message.empty()) {
          continue;
        }
        evb->runInEventBaseThreadAndWait([=] { sendMessageInStream(); });
      }
    } else {
      auto queueClosed = false;
      while (!closed && !queueClosed) {
        try {
          message = msgQueue->pop();
          if (message.empty()) {
            continue;
          }
          evb->runInEventBaseThreadAndWait([=] { sendMessageInStream(); });
        } catch (const support::QueueClosed&) {
          queueClosed = true;
        }
      }
    }
    LOG(INFO) << "EchoClient stopping client";
  }

  ~EchoClient() override = default;

 private:
  void sendMessage(quic::StreamId id, quic::BufQueue& data) {
    auto message = data.move();
    auto res     = quicClient_->writeChain(id, message->clone(), true);
    if (res.hasError()) {
      LOG(ERROR) << "EchoClient writeChain error=" << uint32_t(res.error());
    } else {
      auto str = message->moveToFbString().toStdString();
      LOG(INFO) << "EchoClient wrote \"" << str << "\""
                << ", len=" << str.size() << " on stream=" << id;
      // sent whole message
      pendingOutput_.erase(id);
    }
  }

  std::string                                host_;
  uint16_t                                   port_;
  std::shared_ptr<quic::QuicClientTransport> quicClient_;
  std::map<quic::StreamId, quic::BufQueue>   pendingOutput_;
  std::map<quic::StreamId, uint64_t>         recvOffsets_;
  folly::fibers::Baton                       startDone_;
  std::list<std::string>&                    responses_;
};

class EchoHandler : public quic::QuicSocket::ConnectionSetupCallback,
                    public quic::QuicSocket::ConnectionCallback,
                    public quic::QuicSocket::ReadCallback,
                    public quic::QuicSocket::WriteCallback,
                    public quic::QuicSocket::DatagramCallback
{
 public:
  using StreamData = std::pair<quic::BufQueue, bool>;

  explicit EchoHandler(folly::EventBase*            evbIn,
                       support::Queue<std::string>& commands)
      : evb(evbIn)
      , commands_(commands) {
    // noop
  }

  void setQuicSocket(std::shared_ptr<quic::QuicSocket> socket) {
    sock = socket;
  }

  void onNewBidirectionalStream(quic::StreamId id) noexcept override {
    LOG(INFO) << "Got bidirectional stream id=" << id;
    sock->setReadCallback(id, this);
  }

  void onNewUnidirectionalStream(quic::StreamId id) noexcept override {
    LOG(INFO) << "Got unidirectional stream id=" << id;
    sock->setReadCallback(id, this);
  }

  void onStopSending(quic::StreamId             id,
                     quic::ApplicationErrorCode error) noexcept override {
    LOG(INFO) << "Got StopSending stream id=" << id << " error=" << error;
  }

  void onConnectionEnd() noexcept override {
    LOG(INFO) << "Socket closed";
  }

  void onConnectionSetupError(quic::QuicError error) noexcept override {
    onConnectionError(std::move(error));
  }

  void onConnectionError(quic::QuicError error) noexcept override {
    LOG(ERROR) << "Socket error=" << toString(error.code) << " "
               << error.message;
  }

  void readAvailable(quic::StreamId id) noexcept override {
    LOG(INFO) << "read available for stream id=" << id;

    auto res = sock->read(id, 0);
    if (res.hasError()) {
      LOG(ERROR) << "Got error=" << toString(res.error());
      return;
    }
    if (input_.find(id) == input_.end()) {
      input_.emplace(id, std::make_pair(quic::BufQueue(), false));
    }
    quic::Buf  data    = std::move(res.value().first);
    bool       eof     = res.value().second;
    auto       dataLen = (data ? data->computeChainDataLength() : 0);
    const auto dataStr =
        (data) ? data->clone()->moveToFbString().toStdString() : std::string();
    commands_.push(dataStr);
    LOG(INFO) << "Got len=" << dataLen << " eof=" << uint32_t(eof)
              << " total=" << input_[id].first.chainLength() + dataLen
              << " data=" << dataStr;
    input_[id].first.append(std::move(data));
    input_[id].second = eof;
    if (eof) {
      echo(id, input_[id]);
    }
  }

  void readError(quic::StreamId id, quic::QuicError error) noexcept override {
    LOG(ERROR) << "Got read error on stream=" << id
               << " error=" << toString(error);
    // A read error only terminates the ingress portion of the stream state.
    // Your application should probably terminate the egress portion via
    // resetStream
  }

  void readErrorWithGroup(quic::StreamId      id,
                          quic::StreamGroupId groupId,
                          quic::QuicError     error) noexcept override {
    LOG(ERROR) << "Got read error on stream=" << id << "; group=" << groupId
               << " error=" << toString(error);
  }

  void onDatagramsAvailable() noexcept override {
    auto res = sock->readDatagrams();
    if (res.hasError()) {
      LOG(ERROR) << "readDatagrams() error: " << res.error();
      return;
    }
    LOG(WARNING) << "received " << res->size() << " datagrams: ignored";
  }

  void onStreamWriteReady(quic::StreamId id,
                          uint64_t       maxToSend) noexcept override {
    LOG(INFO) << "socket is write ready with maxToSend=" << maxToSend;
    echo(id, input_[id]);
  }

  void onStreamWriteError(quic::StreamId  id,
                          quic::QuicError error) noexcept override {
    LOG(ERROR) << "write error with stream=" << id
               << " error=" << toString(error);
  }

  folly::EventBase* getEventBase() {
    return evb;
  }

  folly::EventBase*                 evb;
  std::shared_ptr<quic::QuicSocket> sock;
  support::Queue<std::string>&      commands_;

 private:
  void echo(quic::StreamId id, StreamData& data) {
    if (!data.second) {
      // only echo when eof is present
      return;
    }
    auto echoedData = folly::IOBuf::copyBuffer("echo ");
    echoedData->prependChain(data.first.move());
    auto res = sock->writeChain(id, std::move(echoedData), true, nullptr);
    if (res.hasError()) {
      LOG(ERROR) << "write error=" << toString(res.error());
    } else {
      // echo is done, clear EOF
      data.second = false;
    }
  }

  std::map<quic::StreamId, StreamData> input_;
};

class EchoServerTransportFactory : public quic::QuicServerTransportFactory
{
 public:
  EchoServerTransportFactory(support::Queue<std::string>& commands)
      : commands_(commands) {
    // noop
  }

  ~EchoServerTransportFactory() override {
    while (!echoHandlers_.empty()) {
      auto& handler = echoHandlers_.back();
      handler->getEventBase()->runImmediatelyOrRunInEventBaseThreadAndWait(
          [this] {
            // The evb should be performing a sequential consistency atomic
            // operation already, so we can bank on that to make sure the writes
            // propagate to all threads.
            echoHandlers_.pop_back();
          });
    }
  }

  quic::QuicServerTransport::Ptr
  make(folly::EventBase*                      evb,
       std::unique_ptr<folly::AsyncUDPSocket> sock,
       const folly::SocketAddress&,
       quic::QuicVersion,
       std::shared_ptr<const fizz::server::FizzServerContext> ctx) noexcept
      override {
    CHECK_EQ(evb, sock->getEventBase());
    auto echoHandler = std::make_unique<EchoHandler>(evb, commands_);
    auto transport   = quic::QuicServerTransport::make(
        evb, std::move(sock), echoHandler.get(), echoHandler.get(), ctx);
    echoHandler->setQuicSocket(transport);
    echoHandlers_.push_back(std::move(echoHandler));
    return transport;
  }

 private:
  std::vector<std::unique_ptr<EchoHandler>> echoHandlers_;
  support::Queue<std::string>&              commands_;
};

class EchoServer
{
 public:
  explicit EchoServer(const std::string& host, const uint16_t port)
      : host_(host)
      , port_(port)
      , server_(quic::QuicServer::createQuicServer()) {
    server_->setQuicServerTransportFactory(
        std::make_unique<EchoServerTransportFactory>(commands_));
    server_->setTransportStatsCallbackFactory(
        std::make_unique<LogQuicStatsFactory>());
    auto serverCtx = createServerCtx();
    server_->setFizzContext(serverCtx);

    auto settingsCopy                        = server_->getTransportSettings();
    settingsCopy.datagramConfig.enabled      = false;
    settingsCopy.selfActiveConnectionIdLimit = 10;
    settingsCopy.disableMigration            = true;
    server_->setTransportSettings(std::move(settingsCopy));
  }

  void start() {
    // Create a SocketAddress and the default or passed in host.
    folly::SocketAddress addr1(host_.c_str(), port_);
    addr1.setFromHostPort(host_, port_);
    server_->start(addr1, 0);
    LOG(INFO) << "Echo server started at: " << addr1.describe();
    eventbase_.loopForever();
  }

  void stop() {
    eventbase_.terminateLoopSoon();
  }

  support::Queue<std::string>& commands() {
    return commands_;
  }

 private:
  static std::shared_ptr<fizz::server::FizzServerContext> createServerCtx() {
    // read certificate
    folly::ssl::BioUniquePtr bioCert(BIO_new(BIO_s_mem()));
    BIO_write(bioCert.get(), kP256Certificate.data(), kP256Certificate.size());
    folly::ssl::X509UniquePtr certificate(
        PEM_read_bio_X509(bioCert.get(), nullptr, nullptr, nullptr));

    // read private key
    folly::ssl::BioUniquePtr bioKey(BIO_new(BIO_s_mem()));
    BIO_write(bioKey.get(), kP256Key.data(), kP256Key.size());
    folly::ssl::EvpPkeyUniquePtr privKey(
        PEM_read_bio_PrivateKey(bioKey.get(), nullptr, nullptr, nullptr));

    // build the certificate manager
    std::vector<folly::ssl::X509UniquePtr> certs;
    certs.emplace_back(std::move(certificate));
    auto certManager = std::make_unique<fizz::server::CertManager>();
    certManager->addCert(
        std::make_shared<fizz::SelfCertImpl<fizz::KeyType::P256>>(
            std::move(privKey), std::move(certs)),
        true);
    auto serverCtx = std::make_shared<fizz::server::FizzServerContext>();
    serverCtx->setFactory(std::make_shared<quic::QuicFizzFactory>());
    serverCtx->setCertManager(std::move(certManager));
    serverCtx->setOmitEarlyRecordLayer(true);
    serverCtx->setClock(std::make_shared<fizz::SystemClock>());
    return serverCtx;
  }

 private:
  std::string                       host_;
  uint16_t                          port_;
  folly::EventBase                  eventbase_;
  std::shared_ptr<quic::QuicServer> server_;
  support::Queue<std::string>       commands_;
};

struct TestMvfst : public ::testing::Test {};

TEST_F(TestMvfst, DISABLED_echo_server) {
  EchoServer myServer("::1", 10000);
  myServer.start();
}

TEST_F(TestMvfst, DISABLED_echo_client) {
  std::list<std::string> responses; // unused
  EchoClient             myClient("::1", 10000, responses);
  myClient.start("my-token", nullptr);
}

TEST_F(TestMvfst, test_echo_client_server) {
  const auto                  N = 10;
  support::Queue<std::string> queue;

  std::list<std::thread>      threads;
  std::unique_ptr<EchoServer> echoServer;
  std::list<std::string>      responsesExpected;
  std::list<std::string>      responsesActual;
  std::list<std::string>      commandsExpected;
  threads.emplace_back([&echoServer]() {
    echoServer = std::make_unique<EchoServer>("::1", 10000);
    echoServer->start();
  });
  std::this_thread::sleep_for(std::chrono::milliseconds(200));
  threads.emplace_back(std::thread([&queue, &responsesActual]() {
    EchoClient echoClient("::1", 10000, responsesActual);
    echoClient.start("my-token", &queue);
  }));
  std::this_thread::sleep_for(std::chrono::milliseconds(200));

  for (auto i = 0; i < N; i++) {
    commandsExpected.emplace_back("hello-" + std::to_string(i));
    queue.push(commandsExpected.back());
    responsesExpected.emplace_back("echo " + commandsExpected.back());
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
  queue.push("/closed");
  queue.close();
  echoServer->stop();

  // wait for all the threads to finish
  for (auto& thread : threads) {
    thread.join();
  }

  ASSERT_EQ(responsesExpected, responsesActual);
  ASSERT_EQ(N, echoServer->commands().size());

  std::list<std::string> commandsActual;
  for (auto i = 0; i < N; i++) {
    commandsActual.emplace_back(echoServer->commands().pop());
  }
  ASSERT_EQ(commandsExpected, commandsActual);
}

} // namespace mvfst
} // namespace uiiit
