#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <zlib.h>


// Need to link with Ws2_32.lib, Mswsock.lib, and Advapi32.lib
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")


#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27015"

int __cdecl main(int argc, char** argv)
{
    WSADATA wsaData;
    SOCKET ConnectSocket = INVALID_SOCKET;
    struct addrinfo* result = NULL,
        * ptr = NULL,
        hints;
    const char* sendbuf = "this is a test";
    char recvbuf[DEFAULT_BUFLEN];
    int iResult;
    int recvbuflen = DEFAULT_BUFLEN;


    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed with error: %d\n", iResult);
        return 1;
    }


    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    // Resolve the server address and port
    iResult = getaddrinfo(
        "127.0.0.1",
        "15000",
        &hints,
        &result
    );
    if (iResult != 0) {
        printf("getaddrinfo failed with error: %d\n", iResult);
        WSACleanup();
        return 1;
    }

    // Attempt to connect to an address until one succeeds
    for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {

        // Create a SOCKET for connecting to server
        ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype,
            ptr->ai_protocol);
        if (ConnectSocket == INVALID_SOCKET) {
            printf("socket failed with error: %ld\n", WSAGetLastError());
            WSACleanup();
            return 1;
        }

        // Connect to server.
        iResult = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
        if (iResult == SOCKET_ERROR) {
            closesocket(ConnectSocket);
            ConnectSocket = INVALID_SOCKET;
            continue;
        }
        break;
    }

    freeaddrinfo(result);

    if (ConnectSocket == INVALID_SOCKET) {
        printf("Unable to connect to server!\n");
        WSACleanup();
        return 1;
    }

    // Send handshake
    const unsigned char buff_handshake_p1[] = {
        0x00, 0x00, 0x00, 0x00
    };
    iResult = send(ConnectSocket, (const char*)buff_handshake_p1, sizeof(buff_handshake_p1), 0);
    printf("Bytes send: %ld\n", iResult);
    iResult = recv(ConnectSocket, recvbuf, recvbuflen, 0);
    printf("Bytes received: %ld\n", iResult);

    // Send game version
    const unsigned char buff_handshake_p2[] = {
        0x00, 0x00, 0x00, 0x2f, 0x1f, 0x8b, 0x08, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0xff, 0x8b, 0x2e, 0x4b, 0x2d, 0x2a, 0xce,
        0xcc, 0xcf, 0x8b, 0xe5, 0xe2, 0x84, 0xb2, 0x6c, 0x95, 0x0c,
        0xf5, 0x0c, 0x4d, 0xf4, 0x2c, 0x95, 0xb8, 0xa2, 0xf5, 0xe1,
        0x92, 0x5c, 0x00, 0xc0, 0x38, 0xd3, 0xd7, 0x28, 0x00, 0x00,
        0x00
    };
    iResult = send(ConnectSocket, (const char*)buff_handshake_p2, sizeof(buff_handshake_p2), 0);
    printf("Bytes send (version): %ld\n", iResult);
    iResult = recv(ConnectSocket, recvbuf, recvbuflen, 0);
    printf("Bytes received: %ld\n", iResult);

    // Send name "FFFAAAKKKEEE"
    const unsigned char buff_name[] = {
        0x00, 0x00, 0x00, 0x3a, 0x1f, 0x8b, 0x08, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0xff, 0x8b, 0xce, 0xc9, 0x4f, 0xcf, 0xcc,
        0x8b, 0xe5, 0xe2, 0x2c, 0x2d, 0x4e, 0x2d, 0xca, 0x4b, 0xcc,
        0x4d, 0xb5, 0x55, 0x72, 0x73, 0x73, 0x73, 0x74, 0x74, 0xf4,
        0xf6, 0xf6, 0x76, 0x75, 0x75, 0x55, 0xe2, 0x8a, 0xd6, 0x87,
        0xaa, 0xe0, 0x02, 0x00, 0xa1, 0xfc, 0x19, 0x4c, 0x2b, 0x00,
        0x00, 0x00
    };
    iResult = send(ConnectSocket, (const char*)buff_name, sizeof(buff_name), 0);
    printf("Bytes send (name): %ld\n", iResult);
    iResult = recv(ConnectSocket, recvbuf, recvbuflen, 0);
    printf("Bytes received: %ld\n", iResult);

    // Send chat message "a".
    const unsigned char buff_chat_message[] = {
        0x00, 0x00, 0x00, 0x4e, 0x1f, 0x8b, 0x08, 0x00,
	    0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x8b, 0xce,
	    0x4d, 0x2d, 0x2e, 0x4e, 0x4c, 0x4f, 0x8d, 0xe5,
	    0xe2, 0x84, 0xb2, 0x6c, 0x95, 0x12, 0x95, 0xb8,
	    0x38, 0x8b, 0xf2, 0xf3, 0x73, 0x6d, 0x95, 0x72,
	    0xf2, 0x93, 0x92, 0x2a, 0x81, 0xbc, 0xe2, 0xd4,
		0xbc, 0x94, 0xd4, 0x22, 0x5b, 0x25, 0x37, 0x37,
        0x37, 0x47, 0x47, 0x47, 0x6f, 0x6f, 0x6f, 0x57,
        0x57, 0x57, 0x25, 0xae, 0x68, 0x7d, 0xb8, 0x66,
        0x2e, 0x00, 0x9b, 0x77, 0x70, 0x14, 0x48, 0x00,
        0x00, 0x00
    };
    iResult = send(ConnectSocket, (const char*)buff_chat_message, sizeof(buff_chat_message), 0);
    printf("Bytes send ('a' chat message): %ld\n", iResult);
    iResult = recv(ConnectSocket, recvbuf, recvbuflen, 0);
    printf("Bytes received: %ld\n", iResult);


    // Send "z" chat message
    // Game chat messages don't contain text file info (that normally produced
    //   by `gzip.exe`. But even with text file info included, package works fine.
    unsigned char buff_custom_package[] = {
        0x00, 0x00, 0x00, 0x6b,  // length
        0x1f, 0x8b, 0x08, 0x08, 0x05, 0xbb, 0xbb, 0x66, 0x00, 0x0b, 0x63, 0x75,  // chat message.
        0x73, 0x74, 0x6f, 0x6d, 0x5f, 0x70, 0x61, 0x63, 0x6b, 0x61, 0x67, 0x65,
        0x2e, 0x74, 0x78, 0x74, 0x00, 0x8b, 0xce, 0x4d, 0x2d, 0x2e, 0x4e, 0x4c,
        0x4f, 0x8d, 0xe5, 0xe2, 0x84, 0xb2, 0x6c, 0x95, 0x32, 0x52, 0x73, 0x72,
        0xf2, 0x15, 0x4a, 0x32, 0x52, 0x8b, 0x52, 0x95, 0xb8, 0x38, 0x8b, 0xf2,
        0xf3, 0x73, 0x6d, 0x95, 0x72, 0xf2, 0x93, 0x92, 0x2a, 0x81, 0xbc, 0xe2,
        0xd4, 0xbc, 0x94, 0xd4, 0x22, 0x5b, 0x25, 0x37, 0x37, 0x37, 0x47, 0x47,
        0x47, 0x6f, 0x6f, 0x6f, 0x57, 0x57, 0x57, 0x25, 0xae, 0x68, 0x7d, 0xb8,
        0x31, 0x5c, 0x00, 0xf7, 0x7d, 0xd3, 0x3f, 0x52, 0x00, 0x00, 0x00
    };
    iResult = send(ConnectSocket, (const char*)buff_custom_package, sizeof(buff_custom_package), 0);
    printf("Bytes send ('hello there' chat message): %ld\n", iResult);
    iResult = recv(ConnectSocket, recvbuf, recvbuflen, 0);
    printf("Bytes received: %ld\n", iResult);


    // Receive until the peer closes the connection
    do {

        iResult = recv(ConnectSocket, recvbuf, recvbuflen, 0);
        if (iResult > 0)
            printf("Bytes received: %d\n", iResult);
        else if (iResult == 0)
            printf("Connection closed\n");
        else
            printf("recv failed with error: %d\n", WSAGetLastError());

    } while (iResult > 0);


    // cleanup
    closesocket(ConnectSocket);
    WSACleanup();

    return 0;
}