#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <zlib.h>
#include <string.h>


// Need to link with Ws2_32.lib, Mswsock.lib, and Advapi32.lib
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")


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

bool is_wave_message(const char* input_message, size_t len)
{
    FILE* temp_file = NULL;
    fopen_s(&temp_file, "packet_recv.gz", "wb");

    if (temp_file) {
        unsigned char data[DEFAULT_BUFLEN];
        memcpy(data, input_message + 4, len);

        // + 4 because first 4 bytes contain length of data.
        fwrite(data, 1, len - 4, temp_file);

        fclose(temp_file);
    }

    unsigned char decompressed_data[DEFAULT_BUFLEN] = { 0 };
    gzFile data_temp_file = gzopen("packet_recv.gz", "rb");
    if (data_temp_file) {
        gzread(data_temp_file, decompressed_data, sizeof(decompressed_data));

        printf("decompressed_data: %s\n", decompressed_data);

        gzclose(data_temp_file);
    }

    return strstr((const char*)decompressed_data, "\\wave");
}

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

    printf("Socket: %d\n", ConnectSocket);

    // Send handshake
    const unsigned char buff_handshake_p1[] = {
        0x00, 0x00, 0x00, 0x00
    };
    iResult = send(ConnectSocket, (const char*)buff_handshake_p1, sizeof(buff_handshake_p1), 0);
    printf("Bytes send: %ld\n", iResult);
    iResult = recv(ConnectSocket, recvbuf, recvbuflen, 0);
    printf("Bytes received: %ld\n", iResult);

    // Send game version
    unsigned char version[] = "[version]\nversion=\"1.14.9\"\n[/version]";
    send_data(version, sizeof(version), ConnectSocket);

    // Send name "FFFAAAKKKEEE"
    unsigned char name[] = "[login]\nusername=\"ChatBot\"\n[/login]";
    send_data(name, sizeof(name), ConnectSocket);

    // Send chat message "ChatBot connected".
    unsigned char message1[] = "[message]\nmessage=\"ChatBot connected\"\n[/message]";
    send_data(message1, sizeof(message1), ConnectSocket);

    // Send "z" chat message


    // Receive until the peer closes the connection
    do {
        puts("================================");

        iResult = recv(ConnectSocket, recvbuf, recvbuflen, 0);
        if (iResult > 0)
            printf("Bytes received: %d\n", iResult);
        else if (iResult == 0)
            printf("Connection closed\n");
        else
            printf("recv failed with error: %d\n", WSAGetLastError());

        if (is_wave_message(recvbuf, iResult)) {
            unsigned char hello_message[DEFAULT_BUFLEN] = "[message]\nmessage=Hello!\n[/message]";
            send_data(hello_message, sizeof(hello_message), ConnectSocket);
        }

    } while (iResult > 0);


    // cleanup
    closesocket(ConnectSocket);
    WSACleanup();

    return 0;
}