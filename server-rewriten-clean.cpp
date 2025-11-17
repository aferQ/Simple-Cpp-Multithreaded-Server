#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <iostream>
#include <random>
#include <cmath>
#include <vector>
#include <thread>

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT 27018
#define DESIRED_ADDRESS "127.0.0.1"

using namespace std;

// Forward declarations
long int power(long long int a, long long int b, long long int P);
string SzyfrujDeszyfruj(string text, long long int key);

// ----------------------------
// Listener struct
// ----------------------------
struct Listener {
    int sock;
    int port;
    string UID;
    long long int Key;
};

// ----------------------------
// Create a listening socket on a random port
// ----------------------------
Listener create_listener() {
    Listener L;
    L.sock = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(DESIRED_ADDRESS);
    addr.sin_port = 0;  // OS chooses a random free port

    bind(L.sock, (sockaddr*)&addr, sizeof(addr));

    socklen_t len = sizeof(addr);
    getsockname(L.sock, (sockaddr*)&addr, &len);

    L.port = ntohs(addr.sin_port);
    return L;
}

// ----------------------------
// Handle handshake with a client
// Returns a Listener struct for the client-specific port
// ----------------------------
Listener do_handshake(int server_socket) {

    int client_socket = accept(server_socket, nullptr, nullptr);

    char buffer[DEFAULT_BUFLEN];
    int bytesReceived = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
    if (bytesReceived > 0) {
        buffer[bytesReceived] = '\0';
        printf("Received: %s\n", buffer);
    }

    string STR_PRIVATE_KEY;
    long long int PRIVATE_KEY;

    if (strcmp(buffer, "Hello, Server!") == 0) {

        // Send greeting to client
        char message[] = "Hello, Client!";
        send(client_socket, message, strlen(message), 0);

        // Diffie-Hellman variables
        long long int M = 23;
        long long int H = 5;
        long long int y, b;

        // Random private number
        std::mt19937 rng(std::random_device{}());
        int N = 10;
        b = rng() % N + 1;
        y = power(H, b, M);

        string y_char = to_string(y);

        // Receive client's public number
        memset(buffer, 0, sizeof(buffer));
        int alice_x = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
        send(client_socket, y_char.c_str(), y_char.size(), 0);

        buffer[alice_x] = '\0';
        long long int x_str = atol(buffer);

        // Compute shared private key
        PRIVATE_KEY = power(x_str, b, M);
        cout << "Private key is: " << PRIVATE_KEY << endl;
        STR_PRIVATE_KEY = to_string(PRIVATE_KEY);

        // Create new Listener for client communication
        Listener L = create_listener();
        listen(L.sock, 1);
        L.Key = PRIVATE_KEY;

        // Send client the port number
        string new_port = to_string(L.port);
        send(client_socket, new_port.c_str(), new_port.size(), 0);

        int 

        return L;
    }

    // If handshake failed, return empty Listener
    return Listener{};
}

// ----------------------------
// Handle actual client communication
// ----------------------------
void handle_client(int client_socket, int PRIVATE_KEY) {
    char buffer[DEFAULT_BUFLEN];

    while (true) {
        int bytesReceived = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
        if (bytesReceived <= 0) break;  // disconnected or error

        buffer[bytesReceived] = '\0';
        string decrypted = SzyfrujDeszyfruj(buffer, PRIVATE_KEY);

        cout << "[Client] Encrypted: " << buffer << endl;
        cout << "[Client] Decrypted: " << decrypted << endl;

        // Echo back encrypted
        string reply = SzyfrujDeszyfruj(decrypted, PRIVATE_KEY);
        send(client_socket, reply.c_str(), reply.size(), 0);
    }
}

// ----------------------------
// Thread wrapper: accept client on listener and handle communication
// ----------------------------
void handle_listener(Listener L) {
    int client_socket = accept(L.sock, nullptr, nullptr);
    handle_client(client_socket, L.Key);
}

// ----------------------------
// XOR encryption/decryption function
// ----------------------------
string SzyfrujDeszyfruj(string text, long long int key) {
    string output = text;
    for (int i = 0; i < text.size(); i++)
        output[i] = text[i] ^ key;
    return output;
}

// ----------------------------
// Modular exponentiation (simple version)
// ----------------------------
long int power(long long int a, long long int b, long long int P) {
    if (b == 1) return a;
    else return (((long long int)pow(a, b)) % P);
}

// ----------------------------
// Main server loop
// ----------------------------
int main(int argc, char **argv) {
    vector<Listener> listeners;
    vector<thread> threads;

    // Create server socket on DEFAULT_PORT
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in serverAddress{};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(DEFAULT_PORT);
    serverAddress.sin_addr.s_addr = inet_addr(DESIRED_ADDRESS);

    bind(server_socket, (sockaddr*)&serverAddress, sizeof(serverAddress));
    listen(server_socket, 4);

    // Infinite handshake loop
    while (true) {
        Listener L = do_handshake(server_socket);
        listeners.push_back(L);
        threads.push_back(thread(handle_listener, L));
        threads.back().detach();  // allow thread to run independently
    }
}
