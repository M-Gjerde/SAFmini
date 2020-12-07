#undef UNICODE
// Windows socket includes
#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x501
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
// c++ standard libraries
#include <stdlib.h>
#include <stdio.h>
// Parser includes
#define PUGIXML_HEADER_ONLY
#include "pugixml/pugixml.hpp"
#include "rapidcsv.h"

// Need to link with Ws2_32.lib
#pragma comment (lib, "Ws2_32.lib")

#define BUFFER_LENGTH 512
#define PORT "3000"

int processXML(const std::string &xmlString, std::string *row, std::string *column) {
    // XML parser
    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_string(xmlString.c_str());
    if (!result)
        throw std::runtime_error("Failed to open processXML");

    std::ofstream outfile;
    outfile.open("../operation_logs.txt", std::ios::app);

    for (pugi::xml_node tool: doc.child("rfid")) {
        std::string station = tool.child_value();
        std::string carrier = tool.child("carrier").child_value(); // Load object as string name
        std::string time = tool.child("timestamp").child_value();
        //printf("Value: %s\n", name.c_str());
        *row = carrier;
        *column = station;

        printf("carrier: %s, station %s\n", carrier.c_str(), station.c_str());

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
    WSADATA wsaData;
    int iResult;
    SOCKET ListenSocket = INVALID_SOCKET;
    SOCKET PLCSocket = INVALID_SOCKET;
    struct addrinfo *result = NULL;
    struct addrinfo hints;

    int iSendResult;
    char recvbuf[BUFFER_LENGTH];
    int recvbuflen = BUFFER_LENGTH;

    // Initialize WinSock
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
    iResult = getaddrinfo(NULL, PORT, &hints, &result);
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

    // Accept the PLC
    PLCSocket = accept(ListenSocket, NULL, NULL);
    if (PLCSocket == INVALID_SOCKET) {
        printf("accept failed with error: %d\n", WSAGetLastError());
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    } else {
        printf("Client accepted\n");
    }

    // We got a client now. Probably just the PLC so we can close the server socket so we dont get new clients
    closesocket(ListenSocket);

    // Keep connetion alive until PLC closes connection
    do {
        iResult = recv(PLCSocket, recvbuf, recvbuflen, 0);
        if (iResult > 0) {
            // Create std::string from char* to work with parsers
            std::string xmlString(recvbuf);
            // Process the XML, and return carrierID and stationID
            std::string carrierID, station;
            processXML(xmlString, &carrierID, &station);
            // append Carrier# to carrierID to match the first columns in time processing table
            std::string carrierString = "Carrier#";
            carrierString.append(carrierID);

            // retrieve time from the csv file.
            int processing_time = getTimeFromLocalCSVFile(carrierString, station);
            // all times are 4 characters long. therefore a char of 4 bytes.
            char processTimeChar[4];
            std::sprintf(processTimeChar, "%d", processing_time);

            // Send Time back to PLC
            iSendResult = send(PLCSocket, processTimeChar, sizeof(processTimeChar), 0);
            if (iSendResult == SOCKET_ERROR) {
                printf("send failed with error: %d\n", WSAGetLastError());
                closesocket(PLCSocket);
                WSACleanup();
                return 1;
            }
        } else if (iResult == 0)
            printf("Connection closing...\n");
        else {
            printf("recv failed with error: %d\n", WSAGetLastError());
            closesocket(PLCSocket);
            WSACleanup();
            return 1;
        }

    } while (iResult > 0);
    // shutdown the connection since we're done
    iResult = shutdown(PLCSocket, SD_SEND);
    if (iResult == SOCKET_ERROR) {
        printf("shutdown failed with error: %d\n", WSAGetLastError());
        closesocket(PLCSocket);
        WSACleanup();
        return 1;
    }
    // cleanup
    closesocket(PLCSocket);
    WSACleanup();
    return 0;
}