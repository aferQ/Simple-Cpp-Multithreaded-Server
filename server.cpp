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

long int power(long long int a, long long int b, long long int P);

struct Listener {
    int sock;
    int port;
    string UID;
    long long int Key;
};

Listener create_listener() {
    Listener L;

    L.sock = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(DESIRED_ADDRESS);
    addr.sin_port = 0;

    bind(L.sock, (sockaddr*)&addr, sizeof(addr));

    socklen_t len = sizeof(addr);
    getsockname(L.sock, (sockaddr*)&addr, &len);

    L.port = ntohs(addr.sin_port);

    return L;
}

Listener do_handshake(int server_socket) {

    int client_socket = accept(server_socket, NULL, NULL);


    char buffer[DEFAULT_BUFLEN];
    int bytesReceived = recv(client_socket, buffer, sizeof(buffer)-1, 0);
    if (bytesReceived > 0) {
        buffer[bytesReceived] = '\0';
        printf("Received: %s\n", buffer);
    }

    string STR_PRIVATE_KEY;
    long long int PRIVATE_KEY;
    if (strcmp(buffer, "Hello, Server!") == 0) {

        char message[] = "Hello, Client!";
        send(client_socket, message, strlen(message), 0);
        
        //declaring default variables
        long long int M, H, y, b;
        M = 23;
        H = 5;
        

        //seting up generating random number and generating it (from 1 to 100)
        std::mt19937 rng(std::random_device{}()); 
        int N = 10;
        b = rng() % N + 1;
        y = power(H, b, M);
        //cout << "tajne cos servera: " << y << endl;

        string y_char = to_string(y);

        memset(buffer, 0, sizeof(buffer));        // <-- IMPORTANT

        int alice_x = recv(client_socket, buffer, sizeof(buffer)-1, 0);    
        send(client_socket, y_char.c_str(), y_char.size(), 0);

        buffer[alice_x] = '\0';
        //printf("Received: %s\n", buffer);

        long long int x_str = atol(buffer);
        PRIVATE_KEY = power(x_str, b, M);
        cout << "Private key is: " << PRIVATE_KEY << endl;
        STR_PRIVATE_KEY = to_string(PRIVATE_KEY);

        //recive UID
        memset(buffer, 0, sizeof(buffer));        // <-- IMPORTANT
        int UID = recv(client_socket, buffer, sizeof(buffer)-1, 0);
        buffer[UID] = '\0';     // terminate

        //create a new socket for next connection
        Listener L = create_listener();
        listen(L.sock, 1);  
        //listeners.push_back(L);
        L.Key = PRIVATE_KEY;
        L.UID = buffer;

        //send new port 
        string new_port = to_string(L.port);
        send(client_socket, new_port.c_str(), new_port.size(), 0);
        return L;
    }
    return Listener{};
}

string SzyfrujDeszyfruj(string text, long long int key);

void handle_client(int client_socket, int PRIVATE_KEY, string UID) {
    char buffer[DEFAULT_BUFLEN];

    while (true) {
        int bytesReceived = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
        if (bytesReceived <= 0) break; // disconnected or error

        buffer[bytesReceived] = '\0';
        string decrypted = SzyfrujDeszyfruj(buffer, PRIVATE_KEY);

        cout << "[" << UID << "] Encrypted: " << buffer << endl;
        cout << "[" << UID << "] Decrypted: " << decrypted << endl;

        // echo back encrypted
        string reply = SzyfrujDeszyfruj(decrypted, PRIVATE_KEY);
        send(client_socket, reply.c_str(), reply.size(), 0);
    }

}

void handle_listener(Listener L) {
    int client_socket = accept(L.sock, nullptr, nullptr);
    handle_client(client_socket, L.Key, L.UID);
}

string SzyfrujDeszyfruj(string text, long long int key) {
    string output = text;
    //char k = key % 256; // key reduced to a single byte
    for (int i = 0; i < text.size(); i++)
        output[i] = text[i] ^ key;
    return output;
}

long int power(long long int a, long long int b, long long int P)
    {
        if (b == 1) return a;
        else return (((long long int)pow(a, b)) % P);
    }

int main(int argc, char **argv) {
    vector<Listener> listeners;
    vector<std::thread> threads;

    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(DEFAULT_PORT);
    serverAddress.sin_addr.s_addr = inet_addr(DESIRED_ADDRESS);
    
    bind(server_socket, (struct sockaddr *) &serverAddress, sizeof(serverAddress));
    listen(server_socket, 4);
    
    while(true) {
        Listener L = do_handshake(server_socket);
        listeners.push_back(L);
        threads.push_back(thread(handle_listener, L));
        threads.back().detach();  // optional, lets thread run independently
    }
}
