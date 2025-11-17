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
#include <string> 


#define DEFAULT_BUFLEN 512
#define SERVER_PORT 27018


using namespace std;

long int power(long long int a, long long int b, long long int P) {
        if (b == 1) return a;
        else return (((long long int)pow(a, b)) % P);
}

string SzyfrujDeszyfruj(string text, long long int key) {
    string output = text;
    //char k = key % 256; // key reduced to a single byte
    for (int i = 0; i < text.size(); i++)
        output[i] = text[i] ^ key;
    return output;
}

int main() {

    int my_socket = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in my_Address;
    my_Address.sin_family = AF_INET;
    
    string SRV_IP;
    cout << "Give server IP: ";
    cin >> SRV_IP;
    //SRV_IP = "127.0.0.1";

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, SRV_IP.c_str(), &serverAddr.sin_addr);
    connect(my_socket, (struct sockaddr*)&serverAddr, sizeof(serverAddr));

    char message[] = "Hello, Server!";
    send(my_socket, message, strlen(message), 0);

    char buffer[DEFAULT_BUFLEN];
    int bytesReceived = recv(my_socket, buffer, sizeof(buffer)-1, 0);
    if (bytesReceived > 0) {
        buffer[bytesReceived] = '\0';
        printf("Received from server: %s\n", buffer);
    }

    string STR_PRIVATE_KEY;
    long long int PRIVATE_KEY;
    if (strcmp(buffer, "Hello, Client!") == 0) {
        //declaring default variables
        long long int M, H, x, a;
        M = 23;
        H = 5;
    
        //seting up generating random number and generating it (from 1 to 100)
        std::mt19937 rng(std::random_device{}()); 
        int N = 10;
        a = rng() % N + 1;
        x = power(H, a, M);
        //cout << "tajne cos clienta: " << x << endl;

        string x_char = to_string(x);

        send(my_socket, x_char.c_str(), x_char.size(), 0);
        memset(buffer, 0, sizeof(buffer));        // <-- IMPORTANT

        int bob_y = recv(my_socket, buffer, sizeof(buffer)-1, 0);

        buffer[bob_y] = '\0';
        //printf("Received: %s\n", buffer);

        long long int y_str = atol(buffer);
        PRIVATE_KEY = power(y_str, a, M);
        cout << "Private key is: " << PRIVATE_KEY << endl;
        STR_PRIVATE_KEY = to_string(PRIVATE_KEY);

        //Get UID
        //cout << "Write your username: ";
        //cin >> UID;

        //getting new port and connectiong to it
        memset(buffer, 0, sizeof(buffer));        // <-- IMPORTANT
        int bytes = recv(my_socket, buffer, sizeof(buffer)-1, 0);
        buffer[bytes] = '\0';
        int new_port = atoi(buffer);

        //close last one and open new socket
        close(my_socket);
        my_socket = socket(AF_INET, SOCK_STREAM, 0);
        serverAddr.sin_port = htons(new_port);
        connect(my_socket, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
    }


    while (true) {
        //send reply
        string mess;
        cout << "Your message: ";
        cin >> mess;
        string enc_mess = SzyfrujDeszyfruj(mess, PRIVATE_KEY);
        cout << "Send(encrypted): " << enc_mess << endl;
        send(my_socket, enc_mess.c_str(), enc_mess.size(), 0);

        // Receive data
        memset(buffer, 0, sizeof(buffer));        // <-- IMPORTANT
        int bytesReceived = recv(my_socket, buffer, sizeof(buffer)-1, 0);
    
        buffer[bytesReceived] = '\0';
        printf("Received(encrypted): %s\n", buffer);
        printf("Received(unencrypted): %s\n", SzyfrujDeszyfruj(buffer, PRIVATE_KEY).c_str());

    }
    
}