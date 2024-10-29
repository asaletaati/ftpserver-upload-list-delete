#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <vector>
#include <unistd.h>
#include <arpa/inet.h>
#include <dirent.h>

const int PORT = 2121;

void handle_client(int client_socket) {
    char buffer[4096];
    while (true) {
        memset(buffer, 0, sizeof(buffer));
        int bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
        if (bytes_received <= 0) break;

        buffer[bytes_received] = '\0'; // Null-terminate the received command
        std::string command(buffer);
        std::cout << "Received command: " << command << std::endl;

        if (command.substr(0, 4) == "PUT ") { // Upload file
            std::string filename = command.substr(4);

            // Check if the file already exists
            if (std::ifstream(filename)) {
                const char* msg = "File exists. Overwrite? (y/n): ";
                send(client_socket, msg, strlen(msg), 0);
                
                char confirm[2] = {0};
                recv(client_socket, confirm, sizeof(confirm), 0);
                if (confirm[0] != 'y') {
                    const char* msg = "Upload canceled.";
                    send(client_socket, msg, strlen(msg), 0);
                    continue;
                }
            }

            std::ofstream file(filename, std::ios::binary);
            if (!file) {
                const char* msg = "Failed to open file for writing.";
                send(client_socket, msg, strlen(msg), 0);
                continue;
            }

            while ((bytes_received = recv(client_socket, buffer, sizeof(buffer), 0)) > 0) {
                file.write(buffer, bytes_received);
            }
            file.close();
            const char* msg = "File uploaded successfully.";
            send(client_socket, msg, strlen(msg), 0);
        } else if (command.substr(0, 7) == "DELETE ") { // Delete file
            std::string filename = command.substr(7);
            if (remove(filename.c_str()) == 0) {
                const char* msg = "File deleted successfully.";
                send(client_socket, msg, strlen(msg), 0);
            } else {
                const char* msg = "Failed to delete file.";
                send(client_socket, msg, strlen(msg), 0);
            }
        } else if (command == "LIST") { // List files
            std::string files;
            DIR* dir = opendir(".");
            if (dir) {
                struct dirent* entry;
                while ((entry = readdir(dir)) != nullptr) {
                    files += entry->d_name;
                    files += "\n";
                }
                closedir(dir);
            }
            send(client_socket, files.c_str(), files.size(), 0);
        } else if (command == "QUIT") { // Exit
            break;
        }
    }
    close(client_socket);
}

int main() {
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Socket creation failed");
        return 1;
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        return 1;
    }

    if (listen(server_socket, 5) < 0) {
        perror("Listen failed");
        return 1;
    }

    std::cout << "FTP Server listening on port " << PORT << std::endl;

    while (true) {
        sockaddr_in client_addr{};
        socklen_t client_len = sizeof(client_addr);
        int client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);
        if (client_socket < 0) {
            perror("Accept failed");
            continue;
        }
        std::cout << "Client connected." << std::endl;
        handle_client(client_socket);
    }

    close(server_socket);
    return 0;
}
