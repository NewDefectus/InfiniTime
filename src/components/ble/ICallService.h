//
// Created by user on 2/15/23.
//
#pragma once


namespace Pinetime {
  namespace System {
    class SystemTask;
  }
  namespace Controllers {
    class ICallService {
    public:
      
      virtual void AcceptIncomingCall();
      virtual void RejectIncomingCall();
      virtual void MuteIncomingCall();
    };
  }
}