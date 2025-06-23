#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <time.h>
#include <crypt.h>

#define PORT 8080
#define BUFFER_SIZE 4096
#define MAX_SESSIONS 100
#define SESSION_TIMEOUT 3600 // 1 hour in seconds

// Simple session structure
typedef struct {
    char token[64];
    time_t created;
    int active;
} Session;

// Global session storage (in production, use a database)
Session sessions[MAX_SESSIONS];
int session_count = 0;

// Admin credentials (in production, hash these and store securely)
const char* ADMIN_USERNAME = "admin";
const char* ADMIN_PASSWORD = "your_secure_password";

// Generate a simple session token
void generate_token(char* token) {
    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    srand(time(NULL));
    
    for (int i = 0; i < 63; i++) {
        int key = rand() % (sizeof(charset) - 1);
        token[i] = charset[key];
    }
    token[63] = '\0';
}

// Create a new session
char* create_session() {
    if (session_count >= MAX_SESSIONS) {
        return NULL; // Session storage full
    }
    
    Session* session = &sessions[session_count];
    generate_token(session->token);
    session->created = time(NULL);
    session->active = 1;
    session_count++;
    
    return session->token;
}

// Validate session token
int validate_session(const char* token) {
    time_t current_time = time(NULL);
    
    for (int i = 0; i < session_count; i++) {
        if (sessions[i].active && 
            strcmp(sessions[i].token, token) == 0 &&
            (current_time - sessions[i].created) < SESSION_TIMEOUT) {
            return 1; // Valid session
        }
    }
    return 0; // Invalid or expired session
}

// Clean up expired sessions
void cleanup_sessions() {
    time_t current_time = time(NULL);
    
    for (int i = 0; i < session_count; i++) {
        if (sessions[i].active && 
            (current_time - sessions[i].created) >= SESSION_TIMEOUT) {
            sessions[i].active = 0;
        }
    }
}

// Send HTTP response
void send_response(int client_socket, int status_code, const char* content_type, const char* body) {
    char response[BUFFER_SIZE];
    const char* status_text = (status_code == 200) ? "OK" : 
                             (status_code == 401) ? "Unauthorized" : 
                             (status_code == 400) ? "Bad Request" : "Internal Server Error";
    
    snprintf(response, sizeof(response),
        "HTTP/1.1 %d %s\r\n"
        "Content-Type: %s\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
        "Access-Control-Allow-Headers: Content-Type, Authorization\r\n"
        "Content-Length: %lu\r\n"
        "\r\n"
        "%s",
        status_code, status_text, content_type, strlen(body), body);
    
    send(client_socket, response, strlen(response), 0);
}

// Handle login request
void handle_login(int client_socket, const char* body) {
    char username[256] = {0};
    char password[256] = {0};
    
    // Simple JSON parsing (in production, use a proper JSON library)
    char* username_start = strstr(body, "\"username\":");
    char* password_start = strstr(body, "\"password\":");
    
    if (username_start && password_start) {
        // Extract username
        username_start = strchr(username_start, '"');
        username_start = strchr(username_start + 1, '"') + 1;
        char* username_end = strchr(username_start, '"');
        strncpy(username, username_start, username_end - username_start);
        
        // Extract password
        password_start = strchr(password_start, '"');
        password_start = strchr(password_start + 1, '"') + 1;
        char* password_end = strchr(password_start, '"');
        strncpy(password, password_start, password_end - password_start);
        
        // Validate credentials
        if (strcmp(username, ADMIN_USERNAME) == 0 && 
            strcmp(password, ADMIN_PASSWORD) == 0) {
            
            char* token = create_session();
            if (token) {
                char response_body[512];
                snprintf(response_body, sizeof(response_body),
                    "{\"success\": true, \"token\": \"%s\", \"message\": \"Login successful\"}", token);
                send_response(client_socket, 200, "application/json", response_body);
            } else {
                send_response(client_socket, 500, "application/json", 
                    "{\"success\": false, \"message\": \"Session creation failed\"}");
            }
        } else {
            send_response(client_socket, 401, "application/json", 
                "{\"success\": false, \"message\": \"Invalid credentials\"}");
        }
    } else {
        send_response(client_socket, 400, "application/json", 
            "{\"success\": false, \"message\": \"Invalid request format\"}");
    }
}

// Handle protected resource request
void handle_protected(int client_socket, const char* headers) {
    char* auth_header = strstr(headers, "Authorization: Bearer ");
    
    if (auth_header) {
        auth_header += 22; // Skip "Authorization: Bearer "
        char* token_end = strstr(auth_header, "\r\n");
        
        if (token_end) {
            char token[64];
            int token_len = token_end - auth_header;
            strncpy(token, auth_header, token_len);
            token[token_len] = '\0';
            
            if (validate_session(token)) {
                send_response(client_socket, 200, "application/json", 
                    "{\"success\": true, \"message\": \"Access granted\", \"data\": \"Protected admin data\"}");
            } else {
                send_response(client_socket, 401, "application/json", 
                    "{\"success\": false, \"message\": \"Invalid or expired token\"}");
            }
        } else {
            send_response(client_socket, 401, "application/json", 
                "{\"success\": false, \"message\": \"Invalid authorization header\"}");
        }
    } else {
        send_response(client_socket, 401, "application/json", 
            "{\"success\": false, \"message\": \"Authorization header required\"}");
    }
}

// Handle HTTP request
void handle_request(int client_socket) {
    char buffer[BUFFER_SIZE] = {0};
    recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
    
    printf("Received request:\n%s\n", buffer);
    
    // Handle CORS preflight
    if (strncmp(buffer, "OPTIONS", 7) == 0) {
        send_response(client_socket, 200, "text/plain", "");
        return;
    }
    
    // Find the body (after \r\n\r\n)
    char* body = strstr(buffer, "\r\n\r\n");
    if (body) body += 4;
    
    // Route requests
    if (strncmp(buffer, "POST /login", 11) == 0) {
        handle_login(client_socket, body ? body : "");
    } else if (strncmp(buffer, "GET /admin", 10) == 0) {
        handle_protected(client_socket, buffer);
    } else if (strncmp(buffer, "GET /", 5) == 0) {
        send_response(client_socket, 200, "application/json", 
            "{\"message\": \"C Backend API\", \"endpoints\": [\"/login\", \"/admin\"]}");
    } else {
        send_response(client_socket, 404, "application/json", 
            "{\"error\": \"Endpoint not found\"}");
    }
}

int main() {
    int server_fd, client_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    
    // Initialize sessions
    memset(sessions, 0, sizeof(sessions));
    
    // Create socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }
    
    // Set socket options
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt failed");
        exit(EXIT_FAILURE);
    }
    
    // Configure address
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    
    // Bind socket
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }
    
    // Listen for connections
    if (listen(server_fd, 10) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }
    
    printf("Server running on http://localhost:%d\n", PORT);
    printf("Endpoints:\n");
    printf("  POST /login - Admin login\n");
    printf("  GET /admin - Protected admin endpoint\n");
    
    // Main server loop
    while (1) {
        // Clean up expired sessions periodically
        cleanup_sessions();
        
        // Accept connection
        if ((client_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen)) < 0) {
            perror("Accept failed");
            continue;
        }
        
        // Handle request
        handle_request(client_socket);
        
        // Close connection
        close(client_socket);
    }
    
    close(server_fd);
    return 0;
}
