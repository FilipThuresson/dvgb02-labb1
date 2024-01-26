#include <fcntl.h>
#include <netinet/in.h>
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

#define PORT 8080
#define BUFF_SIZE 104857600

char *url_decode(const char *src);

const char *get_mime_type(const char *file_ext);

void build_http_response(const char *file_name, const char *file_ext, char *response, size_t *response_len);

const char *get_filename_ext(const char *filename);

int main() {
    printf("Server Starting...\n");

    int opt = 1;
    int s_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (s_fd < 0) {
        perror("Socket error");
        exit(EXIT_FAILURE);
    }

    if(setsockopt(s_fd,
                  SOL_SOCKET,
                  SO_REUSEADDR | SO_REUSEPORT,
                  &opt,
                  sizeof(opt))
    ) {
        perror("Set socket options failed");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in addr;

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);

    socklen_t addr_len = sizeof(addr);

    if(bind(s_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("Binding Error, cant bind socket fd to addr options");
        exit(EXIT_FAILURE);
    }

    if(listen(s_fd, 1) < 0) {
        perror("Listen error");
        exit(EXIT_FAILURE);
    }
    while(1) {
        int new_socket = accept(s_fd, (struct sockaddr*) &addr, &addr_len);

        if(new_socket < 0) {
            perror("New socket error");
            exit(EXIT_FAILURE);
        }
        char buff[1024] = {0};

        read(new_socket, buff,1024 - 1);

        regex_t regex;
        regcomp(&regex, "^GET /([^ ]*) HTTP/1", REG_EXTENDED);
        regmatch_t matches[2];
        char *filename;

        if(regexec(&regex, buff, 2, matches, 0) == 0) {
            buff[matches[1].rm_eo] = '\0';
            const char *url_encoded_filename = buff + matches[1].rm_so;
            filename = url_decode(url_encoded_filename);
        }

        if(strcmp(filename, "") == 0) {
            strcpy(filename, "index.html");
        }

        char file_ext[32];
        strcpy(file_ext, get_filename_ext(filename));

        // build HTTP response
        char *response = (char *)malloc(BUFF_SIZE * 2 * sizeof(char));
        size_t response_len;
        build_http_response(filename, file_ext, response, &response_len);

        send(new_socket, response, response_len, 0);
        free(response);

        close(new_socket);
    }

    close(s_fd);
    return 0;
}

char *url_decode(const char *src) {
    size_t src_len = strlen(src);
    char *decoded = malloc(src_len + 1);
    size_t decoded_len = 0;

    // decode %2x to hex
    for (size_t i = 0; i < src_len; i++) {
        if (src[i] == '%' && i + 2 < src_len) {
            int hex_val;
            sscanf(src + i + 1, "%2x", &hex_val);
            decoded[decoded_len++] = hex_val;
            i += 2;
        } else {
            decoded[decoded_len++] = src[i];
        }
    }

    // add null terminator
    decoded[decoded_len] = '\0';
    return decoded;
}

void build_http_response(const char *file_name,
                         const char *file_ext,
                         char *response,
                         size_t *response_len) {
    // build HTTP header
    const char *mime_type = get_mime_type(file_ext);
    char *header = (char *)malloc(BUFF_SIZE * sizeof(char));
    snprintf(header, BUFF_SIZE,
             "HTTP/1.1 200 OK\r\n"
             "Content-Type: %s\r\n"
             "\r\n",
             mime_type);

    // if file not exist, response is 404 Not Found
    int file_fd = open(file_name, O_RDONLY);
    if (file_fd == -1) {
        snprintf(response, BUFF_SIZE,
                 "HTTP/1.1 404 Not Found\r\n"
                 "Content-Type: text/plain\r\n"
                 "\r\n"
                 "404 Not Found");
        *response_len = strlen(response);
        return;
    }

    // get file size for Content-Length
    struct stat file_stat;
    fstat(file_fd, &file_stat);
    //off_t file_size = file_stat.st_size;

    // copy header to response buffer
    *response_len = 0;
    memcpy(response, header, strlen(header));
    *response_len += strlen(header);

    // copy file to response buffer
    ssize_t bytes_read;
    while ((bytes_read = read(file_fd,
                              response + *response_len,
                              BUFF_SIZE - *response_len)) > 0) {
        *response_len += bytes_read;
    }
    free(header);
    close(file_fd);
}

const char *get_mime_type(const char *file_ext) {

    if (strcasecmp(file_ext, "html") == 0 || strcasecmp(file_ext, "htm") == 0) {
        return "text/html";
    } else if (strcasecmp(file_ext, "txt") == 0) {
        return "text/plain";
    } else if (strcasecmp(file_ext, "jpg") == 0 || strcasecmp(file_ext, "jpeg") == 0) {
        return "image/jpeg";
    } else if (strcasecmp(file_ext, "png") == 0) {
        return "image/png";
    } else {
        return "text/octet-stream";
    }
}



const char *get_filename_ext(const char *filename) {
    const char *dot = strrchr(filename, '.');
    if(!dot || dot == filename) return "";
    return dot + 1;
}

