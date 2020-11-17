#include <iostream>
#include <netinet/in.h>
#include <zconf.h>
#include <cstring>
#include <vector>
#include "pugixml.hpp"

#define PORT 3000

int document(std::vector<std::string>* cmd) {
    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_file("../doc.xml");
    if (!result)
        throw std::runtime_error("Failed to open document");

    //std::cout << "Load result: " << doc.child("rfid").attribute("version").value() << ", mesh name: "<< doc.child("rfid").child("object").child("name").child_value() << std::endl;

    for (pugi::xml_node tool: doc.child("rfid")) {
        std::string name = tool.child("name").child_value(); // Load object as string name
        std::string cargo = tool.child("cargo").child_value();
        //printf("Value: %s\n", name.c_str());


        if (name == "one" && cargo == "metal") {
            // Send some signal
            printf("Metal carrier detected, transferring..\n");
            cmd->push_back("metal");
        } else if (name == "two")
        {
            cmd->push_back("plastic");
            printf("2nd carrier cargo: %s\n", cargo.c_str());

        }
    }
    return 0;
}

int main() {


    int server_fd, new_socket, valread;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[1024] = {0};
    char *hello = "Hello from server";

    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Forcefully attaching socket to the port 8080
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,
                   &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Forcefully attaching socket to the port 8080
    if (bind(server_fd, (struct sockaddr *) &address,
             sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // Blocking call, wait for a new connection
    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    if ((new_socket = accept(server_fd, (struct sockaddr *) &address,
                             (socklen_t *) &addrlen)) < 0) {
        perror("accept");
        exit(EXIT_FAILURE);
    }

    std::vector<std::string> commandList;


    document(&commandList);

    for (auto &i : commandList){
        if (i == "metal"){
            send(new_socket, i.c_str(), strlen(i.c_str()), 0);
            printf("%s message sent\n", i.c_str());
        } else if (i == "plastic"){
            send(new_socket, i.c_str(), strlen(i.c_str()), 0);
            printf("%s message sent\n", i.c_str());
        }
    }


    return 0;
}