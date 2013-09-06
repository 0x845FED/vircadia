//
//  UDPSocket.h
//  interface
//
//  Created by Stephen Birarda on 1/28/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__UDPSocket__
#define __interface__UDPSocket__

#ifdef _WIN32
#include "Syssocket.h"
#else
#include <netinet/in.h>
#include <netdb.h>
#endif

#define MAX_BUFFER_LENGTH_BYTES 1500

class UDPSocket {    
public:
    UDPSocket(unsigned short int listeningPort);
    ~UDPSocket();
    
    bool init();
    unsigned short int getListeningPort() const { return _listeningPort; }
    
    void setBlocking(bool blocking);
    bool isBlocking() const { return blocking; }
    void setBlockingReceiveTimeoutInUsecs(int timeoutUsecs);
    
    int send(sockaddr* destAddress, const void* data, size_t byteLength) const;
    int send(char* destAddress, int destPort, const void* data, size_t byteLength) const;
    
    bool receive(void* receivedData, ssize_t* receivedBytes) const;
    bool receive(sockaddr* recvAddress, void* receivedData, ssize_t* receivedBytes) const;
private:
    int handle;
    unsigned short int _listeningPort;
    bool blocking;
};

bool socketMatch(const sockaddr* first, const sockaddr* second);
int packSocket(unsigned char* packStore, in_addr_t inAddress, in_port_t networkOrderPort);
int packSocket(unsigned char* packStore, sockaddr* socketToPack);
int unpackSocket(unsigned char* packedData, sockaddr* unpackDestSocket);
int getLocalAddress();
unsigned short loadBufferWithSocketInfo(char* addressBuffer, sockaddr* socket);
sockaddr_in socketForHostnameAndHostOrderPort(const char* hostname, unsigned short port = 0);

#endif /* defined(__interface__UDPSocket__) */
