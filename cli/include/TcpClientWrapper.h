#ifndef TCP_CLIENT_WRAPPER_H
#define TCP_CLIENT_WRAPPER_H

#include "TcpClient.h"
#include "PacketWrapper.h"
#include <msclr/marshal_cppstd.h>
#include <functional>

using namespace System;
using namespace System::Runtime::InteropServices;

namespace CLIWrapper {
    public ref class TcpClientWrapper {
    private:
        flaw::TcpClient* tcpClient;

        static boost::asio::io_context* ioContext;
        static std::thread* contextThread;

        static Action^ onSessionStartCallback;
        static Action^ onSessionEndCallback;
        static Action<PacketWrapper^>^ onPacketReceivedCallback;

        static void RunIoContextStatic() {
            ioContext->run();
        }

        static void OnSessionStart() {
            if (onSessionStartCallback != nullptr) {
                onSessionStartCallback->Invoke();
            }
        }

        static void OnSessionEnd() {
            if (onSessionEndCallback != nullptr) {
                onSessionEndCallback->Invoke();
            }
        }

        static void OnPacketReceived(std::shared_ptr<flaw::Packet> packet) {
            if (onPacketReceivedCallback != nullptr) {     
                array<Byte>^ arr = gcnew array<Byte>(packet->serializedData.size());
                Marshal::Copy(IntPtr(const_cast<char*>(packet->serializedData.data())), arr, 0, packet->serializedData.size());

                auto packetWrapper = gcnew PacketWrapper(
                    packet->header.senderId, 
                    packet->header.packetId, 
                    arr
                );

                onPacketReceivedCallback->Invoke(packetWrapper);
            }
        }

    public:
        TcpClientWrapper() {
            if (ioContext == nullptr) {
                ioContext = new boost::asio::io_context();
                boost::asio::io_context::work idleWork(*ioContext);
                contextThread = new std::thread(RunIoContextStatic);
            }

            tcpClient = new flaw::TcpClient(*ioContext);
        }

        ~TcpClientWrapper() {
            this->!TcpClientWrapper();
        }

        !TcpClientWrapper() {
            tcpClient->Disconnect();
            ioContext->stop();
            contextThread->join();

            delete tcpClient;
            delete contextThread;
            delete ioContext;
        }

        void Connect(String^ ip, short port) {
            std::string nativeIp = msclr::interop::marshal_as<std::string>(ip);
            tcpClient->Connect(nativeIp, port);
        }

        void Send(PacketWrapper^ packetWrapper) {
            tcpClient->Send(std::shared_ptr<flaw::Packet>(packetWrapper->GetPacket()));
        }

        void SetOnSessionStart(Action^ callback) {
            onSessionStartCallback = callback;
            tcpClient->SetOnSessionStart(TcpClientWrapper::OnSessionStart);
        }

        void SetOnSessionEnd(Action^ callback) {
            onSessionEndCallback = callback;
            tcpClient->SetOnSessionEnd(TcpClientWrapper::OnSessionEnd);
        }

        void SetOnPacketReceived(Action<PacketWrapper^>^ callback) {
            onPacketReceivedCallback = callback;
            tcpClient->SetOnPacketReceived(std::bind(TcpClientWrapper::OnPacketReceived, std::placeholders::_1));
        }
    };
}

#endif