#ifndef TCPCLIENTMANAGER_H
#define TCPCLIENTMANAGER_H

#include "TcpClient.h"
#include "TcpClientWrapper.h"
#include <msclr/marshal_cppstd.h>
#include <unordered_map>
#include <mutex>

using namespace System;
using namespace System::Runtime::InteropServices;

namespace CLIWrapper {
    public ref class TcpClientManager {
    public:
        static int CreateClient() {
            const int id = _idCounter++;

            if (_clients->Length <= id) {
                Array::Resize(_clients, id + 1);
            }

            TcpClientWrapper^ client = gcnew TcpClientWrapper(id);
            _clients[id] = client;
            return id;
        }

        static void SetOnConnect(int id, Action^ onConnect) {
            _clients[id]->SetOnConnect(onConnect);
        }

        static void SetOnDisconnect(int id, Action^ onDisconnect) {
			_clients[id]->SetOnDisconnect(onDisconnect);
		}

        static void SetOnPacketReceived(int id, Action<int, int, short, array<Byte>^>^ onPacketReceived) {
			_clients[id]->SetOnPacketReceived(onPacketReceived);
		}

        static void Connect(int id, String^ ip, int port) {
			_clients[id]->Connect(ip, port);
		}

        static void Disconnect(int id) {
			_clients[id]->Disconnect();
            _clients[id] = nullptr;
		}

        static void Send(int id, short packetId, array<Byte>^ data) {
			_clients[id]->Send(packetId, data);
		}

    private:
        TcpClientManager() {}
        ~TcpClientManager() {
            
        }

    private:
        static int _idCounter = 0;
        static array<TcpClientWrapper ^>^ _clients;
    };
}

#endif