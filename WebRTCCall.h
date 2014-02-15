#ifndef WEBRTC_CALL_DOT_H
#define WEBRTC_CALL_DOT_H

#include <stdint.h>
#include <string>

class WebRTCCallObserver {
public:
  virtual void ReceiveAnswer(const std::string& aAnswer) = 0;
  virtual void ReceiveNextVideoFrame(const unsigned char* aBuffer, const int32_t aBufferSize) = 0;

protected:
  WebRTCCallObserver() {}
  virtual ~WebRTCCallObserver() {}
};

class WebRTCCallSystem {
public:
  // will be called from a thread to notify main tread that an event is ready for processing
  virtual void NotifyNextEventReady() = 0;

protected:
  WebRTCCallSystem() {}
  virtual ~WebRTCCallSystem() {}
};

class WebRTCCallEvent {
public:
  virtual ~WebRTCCallEvent() {}
  virtual void Run() = 0;
protected:
  WebRTCCallEvent() {}
};

class WebRTCCall {
public:
  WebRTCCall();
  WebRTCCall(WebRTCCallSystem& aSystem, WebRTCCallObserver& aObserver);
  ~WebRTCCall();

  void AddEvent(WebRTCCallEvent* event);

  void ProcessNextEvent();

  void RegisterSystem(WebRTCCallSystem& aSystem);
  void ReleaseSystem(WebRTCCallSystem& aSystem);

  void RegisterObserver(WebRTCCallObserver& aObserver);
  void ReleaseObserver(WebRTCCallObserver& aObserver);

  void SetOffer(const std::string& aOffer);
  class State;
protected:
  State* mState;
};

#endif //  WEBRTC_CALL_DOT_H
