#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <fstream>

const char* SERVER_IP = "127.0.0.1";
const int PORT = 2121;
const int BUFFER_SIZE = 4096;

int main() {
    int client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0) {
        perror("Socket creation failed");
        return 1;
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
    server_addr.sin_port = htons(PORT);

    if (connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Connection failed." << std::endl;
        return 1;
    }

    std::string command;
    while (true) {
        std::cout << "Enter command (PUT <filename>, DELETE <filename>, LIST, QUIT): ";
        std::getline(std::cin, command);
        send(client_socket, command.c_str(), command.size(), 0);

        if (command.substr(0, 4) == "PUT ") {
            std::string filename = command.substr(4);
            std::ifstream infile(filename, std::ios::binary);
            if (!infile) {
                std::cerr << "Failed to open file for reading: " << filename << std::endl;
                continue;
            }

            // Inform the server about the upload
            send(client_socket, command.c_str(), command.size(), 0);

            // Wait for server confirmation for overwrite
            char response[BUFFER_SIZE];
            int bytes_received = recv(client_socket, response, sizeof(response), 0);
            response[bytes_received] = '\0';

            // Check if the server asks for confirmation
            if (strcmp(response, "File exists. Overwrite? (y/n): ") == 0) {
                char confirm;
                std::cout << response;
                std::cin >> confirm;
                std::cin.ignore(); // Ignore the newline
                send(client_socket, &confirm, sizeof(confirm), 0);
                if (confirm != 'y') {
                    std::cout << "Upload canceled." << std::endl;
                    continue;
                }
            }

            // Send the file contents
            char buffer[BUFFER_SIZE];
            while (infile.read(buffer, sizeof(buffer))) {
                send(client_socket, buffer, infile.gcount(), 0);
            }
            // Send EOF signal
            send(client_socket, "", 0, 0); // Send an empty string to signal EOF

            infile.close();
            std::cout << "File uploaded: " << filename << std::endl;

        } else if (command.substr(0, 7) == "DELETE ") {
            char response[BUFFER_SIZE];
            int bytes_received = recv(client_socket, response, sizeof(response), 0);
            response[bytes_received] = '\0';
            std::cout << "Response: " << response << std::endl;

        } else if (command == "LIST") {
            char response[BUFFER_SIZE];
            int bytes_received = recv(client_socket, response, sizeof(response), 0);
            response[bytes_received] = '\0';
            std::cout << "Files on server:\n" << response << std::endl;

        } else if (command == "QUIT") {
            break;
        }
    }

    close(client_socket);
    return 0;
}
