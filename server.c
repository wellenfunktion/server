#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 1024
#define MAX_ALERTS 100

typedef struct {
    char message[256];
} Alert;

Alert alerts[MAX_ALERTS];
int alert_count = 0;

void handle_get(int client_socket, const char *path);
void handle_post(int client_socket, const char *body);

int main() {
    int server_fd, client_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char buffer[BUFFER_SIZE] = {0};

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Falha na criação do socket");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Falha no bind");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0) {
        perror("Erro no listen");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    printf("Servidor rodando na porta %d\n", PORT);

    while (1) {
        printf("\nAguardando nova conexão...\n");

        if ((client_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0) {
            perror("Erro no accept");
            close(server_fd);
            exit(EXIT_FAILURE);
        }

        read(client_socket, buffer, BUFFER_SIZE);
        printf("Requisição recebida:\n%s\n", buffer);

        if (strncmp(buffer, "GET /status", 11) == 0) {
            handle_get(client_socket, "status");
        } else if (strncmp(buffer, "GET /alerts", 11) == 0) {
            handle_get(client_socket, "alerts");
        } else if (strncmp(buffer, "POST /alert", 11) == 0) {
            char *body = strstr(buffer, "\r\n\r\n") + 4;
            handle_post(client_socket, body);
        } else {
            char *not_found = "HTTP/1.1 404 Not Found\r\nContent-Type: text/plain\r\n\r\n404 Not Found";
            write(client_socket, not_found, strlen(not_found));
        }

        close(client_socket);
        memset(buffer, 0, BUFFER_SIZE);
    }

    close(server_fd);
    return 0;
}

void handle_get(int client_socket, const char *path) {
    if (strcmp(path, "status") == 0) {
        char *response = "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n"
                         "{ \"status\": \"online\", \"generation\": \"5kWh\", \"storage\": \"80%\" }";
        write(client_socket, response, strlen(response));
    } else if (strcmp(path, "alerts") == 0) {
        char response[BUFFER_SIZE] = "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n[";
        
        for (int i = 0; i < alert_count; i++) {
            char alert[300];
            snprintf(alert, sizeof(alert), "{\"message\": \"%s\"}%s", alerts[i].message, i < alert_count - 1 ? "," : "");
            strcat(response, alert);
        }

        strcat(response, "]");
        write(client_socket, response, strlen(response));
    }
}

void handle_post(int client_socket, const char *body) {
    // Verifica o tamanho máximo do alerta
    if (alert_count < MAX_ALERTS) {
        // Copia o corpo da requisição (alerta) para a estrutura de dados
        strncpy(alerts[alert_count].message, body, sizeof(alerts[alert_count].message) - 1);
        alerts[alert_count].message[sizeof(alerts[alert_count].message) - 1] = '\0';  // Garante que a string esteja terminada

        alert_count++;
        char *response = "HTTP/1.1 201 Created\r\nContent-Type: application/json\r\n\r\n"
                         "{ \"status\": \"alert recorded\" }";
        write(client_socket, response, strlen(response));
    } else {
        char *response = "HTTP/1.1 507 Insufficient Storage\r\nContent-Type: application/json\r\n\r\n"
                         "{ \"error\": \"alert storage full\" }";
        write(client_socket, response, strlen(response));
    }
}