#ifndef TCP_CLIENT_NATIVE_H
#define TCP_CLIENT_NATIVE_H

extern "C" {
    __declspec(dllexport) void ConnectToServer(const char* ip, int port);
    __declspec(dllexport) void SendMessage(const char* message);
    __declspec(dllexport) void Disconnect();
}

#endif