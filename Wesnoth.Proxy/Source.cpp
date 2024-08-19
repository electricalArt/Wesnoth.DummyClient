#undef UNICODE

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <zlib.h>

// Need to link with Ws2_32.lib
#pragma comment (lib, "Ws2_32.lib")
// #pragma comment (lib, "Mswsock.lib")

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27015"

void send_data(const unsigned char* data, size_t len, SOCKET s)
{
    int iResult = 0;
    char recvbuf[DEFAULT_BUFLEN];
    int recvbuflen = sizeof(recvbuf);

    gzFile temp_data = gzopen("packet.gz", "wb");
    gzwrite(temp_data, data, len);
    gzclose(temp_data);

    FILE* temp_file = NULL;
    fopen_s(&temp_file, "packet.gz", "rb");
    if (temp_file) {
        unsigned int compress_len = 0;
        unsigned char buff[DEFAULT_BUFLEN] = { 0 };
        compress_len = fread(buff, 1, sizeof(buff), temp_file);
        fclose(temp_file);

        char buff_packet[DEFAULT_BUFLEN] = { 0 };
        // by default, int values are read with Big-endian notation. 
        // This is, int containing 4 will be read os 2e 00 00 00.
        // Here, buff_packet will look like 00 00 00 2e 00 00 00 ... (int copying).
        memcpy(buff_packet + 3, &compress_len, 4);
        memcpy(buff_packet + 4, buff, compress_len);
        printf("package to send: \n%s\n", data);

        iResult = send(s, (const char*)buff_packet, compress_len + 4, 0);
        printf("Bytes send (chat message): %ld\n", iResult);
    }
}

void get_decompressed_data(char* dest, size_t destLen, const char* packet, size_t packetLen)
{
    FILE* temp_file = NULL;
    fopen_s(&temp_file, "packet_recv.gz", "wb");
    if (temp_file == NULL)
        exit(5);
    unsigned char compressed_data[DEFAULT_BUFLEN];
    BYTE compressed_data_len = 0;
    // We should take packet + 4 because first 4 bytes of packet contains
    //   compressed data length (in bytes).
    memcpy(&compressed_data_len, packet + 3, 1);
    memcpy(compressed_data, packet + 4, packetLen);
    fwrite(compressed_data, 1, packetLen - 4, temp_file);
    fclose(temp_file);

    gzFile data_temp_file = gzopen("packet_recv.gz", "rbf");
    if (data_temp_file == NULL)
        exit(6);
    ZeroMemory(dest, destLen);
    gzread(data_temp_file, dest, compressed_data_len);
    gzclose(data_temp_file);
}

bool is_wave_message(const char* packet, size_t len)
{
    char decompressed_data[DEFAULT_BUFLEN] = { 0 };
    get_decompressed_data(decompressed_data, sizeof(decompressed_data), packet, len);

    return strstr((const char*)decompressed_data, "\\wave");
}


int __cdecl main(void)
{
    WSADATA wsaData;
    int iResult;

    SOCKET ListenSocket = INVALID_SOCKET;
    SOCKET ClientSocket = INVALID_SOCKET;

    struct addrinfo* result = NULL;
    struct addrinfo hints;

    int iSendResult;
    char recvbuf[DEFAULT_BUFLEN];
    int recvbuflen = DEFAULT_BUFLEN;

    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed with error: %d\n", iResult);
        return 1;
    }

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    // Resolve the server address and port
    iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
    if (iResult != 0) {
        printf("getaddrinfo failed with error: %d\n", iResult);
        WSACleanup();
        return 1;
    }

    // Create a SOCKET for the server to listen for client connections.
    ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (ListenSocket == INVALID_SOCKET) {
        printf("socket failed with error: %ld\n", WSAGetLastError());
        freeaddrinfo(result);
        WSACleanup();
        return 1;
    }

    // Setup the TCP listening socket
    iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        printf("bind failed with error: %d\n", WSAGetLastError());
        freeaddrinfo(result);
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    freeaddrinfo(result);

    iResult = listen(ListenSocket, SOMAXCONN);
    if (iResult == SOCKET_ERROR) {
        printf("listen failed with error: %d\n", WSAGetLastError());
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    // Permits an incoming connection attempt to the socket.
    // ClientSocket is represents the first client of pending clients.
    puts("Waiting for the `Wesnoth` client...");
    ClientSocket = accept(ListenSocket, NULL, NULL);
    if (ClientSocket == INVALID_SOCKET) {
        printf("accept failed with error: %d\n", WSAGetLastError());
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }
    printf("`Wesnoth` client socket: %lld\n", ClientSocket);

    // No longer need (this application) server socket
    closesocket(ListenSocket);


    // Creating `Wesnoth` server socket
    SOCKET ServerSocket = INVALID_SOCKET;
    ZeroMemory(&hints, sizeof(hints));

    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    iResult = getaddrinfo(
        "127.0.0.1",
        "15000",
        &hints,
        &result);
    ServerSocket = socket(
        result->ai_family,
        result->ai_socktype,
        result->ai_protocol);
    iResult = connect(
        ServerSocket,
        result->ai_addr,
        result->ai_addrlen);
    freeaddrinfo(result);
    printf("`Wesnoth` server socket: %lld\n", ServerSocket);

    // Setting server and client response timeout
    DWORD dwTimeout = 1000;
    setsockopt(ServerSocket, SOL_SOCKET, SO_RCVTIMEO, (char*)&dwTimeout, sizeof(dwTimeout));


    // Receive until the peer shuts down the connection
    do {
        puts("=================");

        // Sleep to receive all packets that client wants to send.
        Sleep(100);
        iResult = recv(ClientSocket, recvbuf, recvbuflen, 0);
        printf("Bytes received from client: %d\n", iResult);
        if (iResult <= 0) {
            continue;
        }

        // Man-in-the-Middle
        // If client sends message with "\wave" text included, then send
        //   another chat message.
        char decompressed_data[DEFAULT_BUFLEN];
        get_decompressed_data(decompressed_data, sizeof(decompressed_data), recvbuf, recvbuflen);
        printf("decompressed data (from client): \n%s\n", decompressed_data);
        if (is_wave_message(recvbuf, recvbuflen)) {
            const unsigned char hello_message[] =
                "[message]\nmessage=\"Hello there!\"\n[/message]";
            send_data(hello_message, sizeof(hello_message), ServerSocket);
        }

        // Relay the buffer to `Wesnoth` server
        iSendResult = send(ServerSocket, recvbuf, iResult, 0);
        printf("Bytes sent to server: %d\n", iSendResult);

        // Sleep to receive all packets that server wants to send.
        Sleep(100);
        // Wait for respons from `Wesnoth` server and relay it to client
        iResult = recv(ServerSocket, recvbuf, recvbuflen, 0);
        printf("Bytes received from server: %d\n", iResult);
        if (iResult <= 0) {
            continue;
        }
        get_decompressed_data(decompressed_data, sizeof(decompressed_data), recvbuf, recvbuflen);
        printf("decompressed data (from server): \n%s\n", decompressed_data);

        iSendResult = send(ClientSocket, recvbuf, iResult, 0);
        printf("Bytes send to client: %d\n", iResult);
    } while (iResult > 0 || WSAGetLastError() == WSAETIMEDOUT);

    if (iResult == 0) {
        printf("Connection closing...\n");
    } else {
        printf("recv failed with error: %d\n", WSAGetLastError());
    }


    // shutdown the connection since we're done
    iResult = shutdown(ClientSocket, SD_SEND);
    if (iResult == SOCKET_ERROR) {
        printf("shutdown failed with error: %d\n", WSAGetLastError());
        closesocket(ClientSocket);
        WSACleanup();
        return 1;
    }

    // cleanup
    closesocket(ClientSocket);
    WSACleanup();

    return 0;
}