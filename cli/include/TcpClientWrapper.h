#ifndef TCP_CLIENT_WRAPPER_H
#define TCP_CLIENT_WRAPPER_H

#include "TcpClient.h"
#include <msclr/marshal_cppstd.h>
#include <msclr/gcroot.h>
#include <functional>
#include <vcclr.h>

using namespace System;
using namespace System::Runtime::InteropServices;

public delegate void OnConnectDelegate();

namespace CLIWrapper {
    class OnConnectWrapper {
    public:
        gcroot<Action^> _onConnect;

        OnConnectWrapper(Action^ onConnect) {
			_onConnect = onConnect;
		}

        void operator()() {
			_onConnect->Invoke();
		}
    };

    class OnDisconnectWrapper {
    public:
        gcroot<Action^> _onDisconnect;

        OnDisconnectWrapper(Action^ onDisconnect) {
            _onDisconnect = onDisconnect;
        }

        void operator()() {
			_onDisconnect->Invoke();
		}
	};

    class OnPacketReceivedWrapper {
    public:
        int clientID;

        // _1 : clientID, _2 : senderID, _3 : packetID, _4 : rawData
        gcroot<Action<int, int, short, array<Byte>^>^> _onPacketReceived;

        OnPacketReceivedWrapper(int clientID, Action<int, int, short, array<Byte>^>^ onPacketReceived) {
            this->clientID = clientID;
            _onPacketReceived = onPacketReceived;
        }

        void operator()(std::shared_ptr<flaw::Packet> packet) {
			array<Byte>^ data = gcnew array<Byte>(packet->serializedData.size());
			Marshal::Copy(IntPtr((void*)packet->serializedData.data()), data, 0, packet->serializedData.size());
			_onPacketReceived->Invoke(clientID, packet->header.senderId, packet->header.packetId, data);
		}
    };

    public ref class TcpClientWrapper {
    public:
        TcpClientWrapper(int clientID) {
            _clientID = clientID;
            _ioContext = new boost::asio::io_context();
            boost::asio::io_context::work idleWork(*_ioContext);
            System::Threading::ThreadStart^ threadStart = gcnew System::Threading::ThreadStart(this, &TcpClientWrapper::RunIOContext);
            _contextThread = gcnew System::Threading::Thread(threadStart);
            _client = new flaw::TcpClient(*_ioContext);
        }

        ~TcpClientWrapper() {
            this->!TcpClientWrapper();
        }

        !TcpClientWrapper() {
            _ioContext->stop();

            delete _client;
            delete _ioContext;
        }

        void Connect(String^ ipAddress, int port) {
            std::string nativeIp = msclr::interop::marshal_as<std::string>(ipAddress);
            _client->Connect(nativeIp, port);
        }

        void Disconnect() {
			_client->Disconnect();
		}

        void SetOnConnect(Action^ onConnect) {
            _client->SetOnSessionStart(OnConnectWrapper(onConnect));
        }

        void SetOnDisconnect(Action^ onDisconnect) {
			_client->SetOnSessionEnd(OnDisconnectWrapper(onDisconnect));
		}

        void SetOnPacketReceived(Action<int, int, short, array<Byte>^>^ onPacketReceived) {
			_client->SetOnPacketReceived(OnPacketReceivedWrapper(_clientID, onPacketReceived));
		}

        void Send(short packetId, array<Byte>^ data) {
            std::vector<char> rawData(data->Length);
            Marshal::Copy(data, 0, IntPtr(rawData.data()), data->Length);
            // 아직 senderId는 0으로 설정
            std::shared_ptr<flaw::Packet> packet = std::make_shared<flaw::Packet>(0, packetId, rawData);
            _client->Send(packet);
		}

    private:
        void RunIOContext() {
            _ioContext->run();
        }

    public:
        int _clientID;

        flaw::TcpClient* _client;
        boost::asio::io_context* _ioContext;
        System::Threading::Thread^ _contextThread;
    };
}

#endif