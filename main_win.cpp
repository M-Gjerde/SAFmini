#undef UNICODE

#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x501
#define PUGIXML_HEADER_ONLY

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>

//#include "csv-parser/single_include/csv.hpp"
#include "pugixml/pugixml.hpp"
#include "rapidcsv.h"

// Need to link with Ws2_32.lib
#pragma comment (lib, "Ws2_32.lib")
// #pragma comment (lib, "Mswsock.lib")

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "3000"

int processXML(const std::string &xmlString, std::string *row, std::string *column) {


    // XML parser
    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_string(xmlString.c_str());
    if (!result)
        throw std::runtime_error("Failed to open processXML");

    //std::cout << "Load result: " << doc.child("rfid").attribute("version").value() << ", mesh name: "<< doc.child("rfid").child("object").child("name").child_value() << std::endl;

    std::ofstream outfile;
    outfile.open("../operation_logs.txt", std::ios::app);

    for (pugi::xml_node tool: doc.child("rfid")) {
        std::string station = tool.child_value();
        std::string carrier = tool.child("carrier").child_value(); // Load object as string name
        std::string time = tool.child("timestamp").child_value();
        //printf("Value: %s\n", name.c_str());
        *row = carrier;
        *column = station;


        // Write timestamp, carrier and station id to file to log of operation

        outfile << time << std::endl;
        outfile << station << std::endl;
        outfile << carrier << std::endl;
        outfile << "______________________________" << std::endl << std::endl;


    }
    outfile.close();

    return 0;
}


int getTimeFromLocalCSVFile(const std::string& carrier, const std::string& station){
    rapidcsv::Document doc("../processing_times_table.csv", rapidcsv::LabelParams(0, 0),rapidcsv::SeparatorParams(','));
    return doc.GetCell<long long>(station, carrier);
}


int __cdecl main(void) {

    std::string xml = "<rfid version=\"1.0.0\">"
                      "<station>Station#01"
                      "<carrier>Carrier#2</carrier>"
                      "<timestamp>#2020/12/02, 14:58:02</timestamp>"
                      "</station>"
                      "</rfid>";
    std::vector<std::string> param;

    std::string row, column;
    processXML(xml, &row, &column);

    printf("carrier: %s, station %s\n", row.c_str(), column.c_str());


    WSADATA wsaData;
    int iResult;

    SOCKET ListenSocket = INVALID_SOCKET;
    SOCKET ClientSocket = INVALID_SOCKET;

    struct addrinfo *result = NULL;
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

    // Create a SOCKET for connecting to server
    ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (ListenSocket == INVALID_SOCKET) {
        printf("socket failed with error: %ld\n", WSAGetLastError());
        freeaddrinfo(result);
        WSACleanup();
        return 1;
    }

    // Setup the TCP listening socket
    iResult = bind(ListenSocket, result->ai_addr, (int) result->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        printf("bind failed with error: %d\n", WSAGetLastError());
        freeaddrinfo(result);
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    freeaddrinfo(result);

    printf("Waiting for a client...\n");
    iResult = listen(ListenSocket, SOMAXCONN);
    if (iResult == SOCKET_ERROR) {
        printf("listen failed with error: %d\n", WSAGetLastError());
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    // Accept a client socket
    ClientSocket = accept(ListenSocket, NULL, NULL);
    if (ClientSocket == INVALID_SOCKET) {
        printf("accept failed with error: %d\n", WSAGetLastError());
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    } else {
        printf("Client accepted\n");
    }

    // No longer need server socket
    closesocket(ListenSocket);


    // Receive until the peer shuts down the connection
    do {
        iResult = recv(ClientSocket, recvbuf, recvbuflen, 0);
        if (iResult > 0) {
            printf("Bytes received: %d\n", iResult);
            std::string xmlString(recvbuf);
            std::string carrier, station;
            processXML(xmlString, &carrier, &station);
            // retrieve timek
            int processing_time = getTimeFromLocalCSVFile(carrier, station);
            char processTimeChar[10];
            std::sprintf(processTimeChar, "%d", processing_time);

            // Send Time back to PLC
            iSendResult = send(ClientSocket, processTimeChar, sizeof(processTimeChar), 0);
            if (iSendResult == SOCKET_ERROR) {
                printf("send failed with error: %d\n", WSAGetLastError());
                closesocket(ClientSocket);
                WSACleanup();
                return 1;
            }
            printf("Bytes sent: %d\n", iSendResult);

        } else if (iResult == 0)
            printf("Connection closing...\n");
        else {
            printf("recv failed with error: %d\n", WSAGetLastError());
            closesocket(ClientSocket);
            WSACleanup();
            return 1;
        }

    } while (iResult > 0);



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