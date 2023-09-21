#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <limits.h>
#include <ctype.h>
#include <regex.h>
#include <sys/stat.h>
#define BUFFER_SIZE 2048
#include "asgn2_helper_funcs.h"

struct Request {
    char *method;
    char *url;
    char *version;
    char *header;
    // sample message body variable will remove later
    char *message;
    char *content;
    int c;
};

int main(int argc, char *argv[]) {
    // check correct number of commands
    if (argc != 2) {
        fprintf(stderr, "Usage: ./echoserv <port>\n");
        exit(EXIT_FAILURE);
    }

    // store port in str value
    char str[60000];
    strcpy(str, argv[1]);

    // check if port is within valid range and converts port from string to int
    for (int i = 0; str[i] != '\0'; i++) {
        if (!isdigit(str[i])) {
            fprintf(stderr, "Invalid Port\n");
            return 1;
        }
    }

    int port = atoi(str);

    if (port < 1 || port > 65535) {
        fprintf(stderr, "Invalid Port\n");
        return 1;
    }

    // create and initialize port
    Listener_Socket sock;
    int result = listener_init(&sock, port);
    if (result == -1) {
        fprintf(stderr, "Invalid Port\n");
        return 1;
    }

    // initialize buffer to read into
    char buf[BUFFER_SIZE + 1];

    // create loop to process incoming connections
    while (1) {
        // check for incoming connection, act on connection if it exists, do nothing if it doesn't
        int fd = listener_accept(&sock);

        if (fd > 0) {
            int bytes_read;
            // set server response components
            char response_destination[BUFFER_SIZE] = "HTTP/1.1 ";
            char response_response[BUFFER_SIZE];
            char response_header[BUFFER_SIZE];
            char response_message[BUFFER_SIZE] = "";

            bytes_read = read_until(fd, buf, BUFFER_SIZE, "");
            if (bytes_read > 0) {
                // null terminate
                buf[bytes_read] = 0;
                // check if read is less than 0
                if (bytes_read < 0) {
                    strcpy(response_response, "500 Internal Server Error\r\n");
                    strcpy(response_header, "Content-Length: 22\r\n\r\n");
                    strcpy(response_message, "Internal Server Error\n");
                    write_all(fd, response_destination, strlen(response_destination));
                    write_all(fd, response_response, strlen(response_response));
                    write_all(fd, response_header, strlen(response_header));
                    write_all(fd, response_message, strlen(response_message));
                    close(fd);
                }

                regex_t reegex;

                // Variable for return type
                int value;

                // Creation of regex
                value = regcomp(&reegex,
                    "^([a-zA-Z]{1,8}) (/[a-zA-Z0-9.-]{2,64}) "
                    "(HTTP/[0-9]\\.[0-9])(\r\n([a-zA-Z0-9_.-]{1,128}: "
                    "[!-~]{1,128}\r\n)*\r\n)([\\s\\S]*)",
                    REG_NEWLINE | REG_EXTENDED);
                if (value != 0) {
                    strcpy(response_response, "500 Internal Server Error\r\n");
                    strcpy(response_header, "Content-Length: 22\r\n\r\n");
                    strcpy(response_message, "Internal Server Error\n");
                    write_all(fd, response_destination, strlen(response_destination));
                    write_all(fd, response_response, strlen(response_response));
                    write_all(fd, response_header, strlen(response_header));
                    write_all(fd, response_message, strlen(response_message));
                    close(fd);
                }

                // string in reg
                regmatch_t pmatch[7];

                // regex match
                value = regexec(&reegex, buf, 7, pmatch, 0);
                if (value != 0) {
                    strcpy(response_response, "400 Bad Request\r\n");
                    strcpy(response_header, "Content-Length: 12\r\n\r\n");
                    strcpy(response_message, "Bad Request\n");
                    write_all(fd, response_destination, strlen(response_destination));
                    write_all(fd, response_response, strlen(response_response));
                    write_all(fd, response_header, strlen(response_header));
                    write_all(fd, response_message, strlen(response_message));
                    close(fd);
                }

                // allocate space for the texts
                struct Request request;
                request.c = -1;
                request.method = calloc(pmatch[1].rm_eo - pmatch[1].rm_so + 1, sizeof(char));
                request.url = calloc(pmatch[2].rm_eo - pmatch[2].rm_so + 1, sizeof(char));
                request.version = calloc(pmatch[3].rm_eo - pmatch[3].rm_so + 1, sizeof(char));
                request.header = calloc(pmatch[4].rm_eo - pmatch[4].rm_so + 1, sizeof(char));

                // copy values into struct
                strncpy(request.method, buf + pmatch[1].rm_so, pmatch[1].rm_eo - pmatch[1].rm_so);
                request.method[pmatch[1].rm_eo - pmatch[1].rm_so] = '\0';
                strncpy(request.url, buf + pmatch[2].rm_so, pmatch[2].rm_eo - pmatch[2].rm_so);
                request.url[pmatch[2].rm_eo - pmatch[2].rm_so] = '\0';
                strncpy(request.version, buf + pmatch[3].rm_so, pmatch[3].rm_eo - pmatch[3].rm_so);
                request.version[pmatch[3].rm_eo - pmatch[3].rm_so] = '\0';
                strncpy(request.header, buf + pmatch[4].rm_so, pmatch[4].rm_eo - pmatch[4].rm_so);
                request.header[pmatch[4].rm_eo - pmatch[4].rm_so] = '\0';

                // message
                size_t message_size = bytes_read - pmatch[4].rm_eo;
                request.message = (char *) calloc(message_size + 1, sizeof(char));
                memcpy(request.message, buf + pmatch[4].rm_eo, message_size);
                request.message[message_size] = '\0';

                // free the regex
                regfree(&reegex);

                // check the header!!!
                regex_t ree;
                // header regex pattern over here
                value = regcomp(&ree, "([a-zA-Z0-9_.-]{1,128}): ([[:print:]]{1,128})",
                    REG_NEWLINE | REG_EXTENDED);
                if (value != 0) {
                    strcpy(response_response, "500 Internal Server Error\r\n");
                    strcpy(response_header, "Content-Length: 22\r\n\r\n");
                    strcpy(response_message, "Internal Server Error\n");
                    write_all(fd, response_destination, strlen(response_destination));
                    write_all(fd, response_response, strlen(response_response));
                    write_all(fd, response_header, strlen(response_header));
                    write_all(fd, response_message, strlen(response_message));
                    free(request.method);
                    free(request.url);
                    free(request.version);
                    free(request.header);
                    free(request.message);
                    close(fd);
                }

                // check each header
                regmatch_t matches[3];
                int match_offset = 0;

                // check each header
                while (regexec(&ree, request.header + match_offset, 3, matches, 0) == 0) {
                    // get key
                    int group_1_start = matches[1].rm_so;
                    int group_1_end = matches[1].rm_eo;
                    char header[128];
                    strncpy(header, request.header + group_1_start + match_offset,
                        group_1_end - group_1_start);
                    header[group_1_end - group_1_start] = '\0';
                    if (strcmp(header, "Content-Length") == 0) {
                        if (request.c != -1) {
                            strcpy(response_response, "400 Bad Request\r\n");
                            strcpy(response_header, "Content-Length: 12\r\n\r\n");
                            strcpy(response_message, "Bad Request\n");
                            write_all(fd, response_destination, strlen(response_destination));
                            write_all(fd, response_response, strlen(response_response));
                            write_all(fd, response_header, strlen(response_header));
                            write_all(fd, response_message, strlen(response_message));
                            free(request.method);
                            free(request.url);
                            free(request.version);
                            free(request.header);
                            free(request.message);
                            close(fd);
                        }

                        // get value
                        int group_2_start = matches[2].rm_so;
                        int group_2_end = matches[2].rm_eo;
                        request.content = (char *) malloc(group_2_end - group_2_start + 1);
                        strncpy(request.content, request.header + group_2_start + match_offset,
                            group_2_end - group_2_start);
                        request.content[group_2_end - group_2_start] = '\0';

                        // check is the content is a number
                        for (int i = 0; request.content[i] != '\0'; i++) {
                            if (!isdigit(request.content[i])) {
                                strcpy(response_response, "400 Bad Request\r\n");
                                strcpy(response_header, "Content-Length: 12\r\n\r\n");
                                strcpy(response_message, "Bad Request\n");
                                write_all(fd, response_destination, strlen(response_destination));
                                write_all(fd, response_response, strlen(response_response));
                                write_all(fd, response_header, strlen(response_header));
                                write_all(fd, response_message, strlen(response_message));
                                free(request.method);
                                free(request.url);
                                free(request.version);
                                free(request.header);
                                free(request.message);
                                close(fd);
                            }
                        }

                        request.c = atoi(request.content);
                        free(request.content);
                    }

                    // increase offset
                    int match_end = matches[0].rm_eo + match_offset;
                    match_offset = match_end;
                }
                regfree(&ree);

                // convert method to upper case
                for (int i = 0; request.method[i]; i++) {
                    str[i] = toupper(request.method[i]);
                }

                // commit the get and put reuests
                if (strcmp(request.method, "GET") == 0) {
                    if (strcmp(request.message, "") == 0) {
                        // check if file is valid
                        char filepath[65] = ".";
                        strcat(filepath, request.url);
                        int f_two = open(filepath, O_RDONLY, 0666);

                        // checks if open has any errors
                        if (f_two == -1) {
                            if (errno == ENOENT) {
                                strcpy(response_response, "404 Not Found\r\n");
                                strcpy(response_header, "Content-Length: 10\r\n\r\n");
                                strcpy(response_message, "Not Found\n");
                            } else if (errno == EACCES) {
                                strcpy(response_response, "403 Forbidden\r\n");
                                strcpy(response_header, "Content-Length: 10\r\n\r\n");
                                strcpy(response_message, "Forbidden\n");
                            }
                            write_all(fd, response_destination, strlen(response_destination));
                            write_all(fd, response_response, strlen(response_response));
                            write_all(fd, response_header, strlen(response_header));
                            write_all(fd, response_message, strlen(response_message));
                            free(request.method);
                            free(request.url);
                            free(request.version);
                            free(request.header);
                            free(request.message);
                            close(fd);
                        }

                        int res = lseek(f_two, 0, SEEK_SET);

                        char bu[BUFFER_SIZE];

                        res = read(f_two, bu, BUFFER_SIZE);
                        if (errno == EISDIR) {
                            strcpy(response_response, "403 Forbidden\r\n");
                            strcpy(response_header, "Content-Length: 10\r\n\r\n");
                            strcpy(response_message, "Forbidden\n");
                            write_all(fd, response_destination, strlen(response_destination));
                            write_all(fd, response_response, strlen(response_response));
                            write_all(fd, response_header, strlen(response_header));
                            write_all(fd, response_message, strlen(response_message));
                            free(request.method);
                            free(request.url);
                            free(request.version);
                            free(request.header);
                            free(request.message);
                            close(f_two);
                            close(fd);
                        }
                        while (res > 0) {
                            res = read(f_two, bu, BUFFER_SIZE);
                        }

                        if (res == -1) {

                            strcpy(response_response, "500 Internal Server Error\r\n");
                            strcpy(response_header, "Content-Length: 22\r\n\r\n");
                            strcpy(response_message, "Internal Server Error\n");
                            write_all(fd, response_destination, strlen(response_destination));
                            write_all(fd, response_response, strlen(response_response));
                            write_all(fd, response_header, strlen(response_header));
                            write_all(fd, response_message, strlen(response_message));
                            close(fd);
                        } else {
                            strcpy(response_response, "200 Ok\r\n");
                        }

                        struct stat fileStat;
                        if (stat(filepath, &fileStat) == 0) {
                            char buffer[BUFFER_SIZE];
                            snprintf(buffer, BUFFER_SIZE, "Content-Length: %ld\r\n\r\n",
                                fileStat.st_size);
                            strcpy(response_header, buffer);
                        } else {
                            strcpy(response_response, "500 Internal Server Error\r\n");
                            strcpy(response_header, "Content-Length: 22\r\n\r\n");
                            strcpy(response_message, "Internal Server Errore\n");
                            write_all(fd, response_destination, strlen(response_destination));
                            write_all(fd, response_response, strlen(response_response));
                            write_all(fd, response_header, strlen(response_header));
                            write_all(fd, response_message, strlen(response_message));
                            free(request.method);
                            free(request.url);
                            free(request.version);
                            free(request.header);
                            free(request.message);
                            close(fd);
                        }

                        // checks the version
                        char one = request.version[strlen(request.version) - 3];
                        char two = request.version[strlen(request.version) - 1];

                        if (one != '1' || two != '1') {
                            strcpy(response_response, "505 Version Not Supported\r\n");
                            strcpy(response_header, "Content-Length: 22\r\n\r\n");
                            strcpy(response_message, "Version Not Supported\n");
                            write_all(fd, response_destination, strlen(response_destination));
                            write_all(fd, response_response, strlen(response_response));
                            write_all(fd, response_header, strlen(response_header));
                            write_all(fd, response_message, strlen(response_message));
                            close(fd);
                        }

                        write_all(fd, response_destination, strlen(response_destination));
                        write_all(fd, response_response, strlen(response_response));
                        write_all(fd, response_header, strlen(response_header));
                        if (strlen(response_message) == 0) {
                            lseek(f_two, 0, SEEK_SET);
                            res = pass_bytes(f_two, fd, fileStat.st_size);
                        } else {
                            res = write_all(fd, response_message, strlen(response_message));
                        }

                        free(request.method);
                        free(request.url);
                        free(request.version);
                        free(request.header);
                        free(request.message);

                        close(f_two);
                    } else {
                        strcpy(response_response, "400 Bad Request\r\n");
                        strcpy(response_header, "Content-Length: 12\r\n\r\n");
                        strcpy(response_message, "Bad Request\n");
                        write_all(fd, response_destination, strlen(response_destination));
                        write_all(fd, response_response, strlen(response_response));
                        write_all(fd, response_header, strlen(response_header));
                        free(request.method);
                        free(request.url);
                        free(request.version);
                        free(request.header);
                        free(request.message);
                    }
                } else if (strcmp(request.method, "PUT") == 0) {
                    if (request.c != -1) {
                        // check if file is valid
                        char filepath[65] = ".";
                        strcat(filepath, request.url);
                        int f_two = open(filepath, O_RDONLY | O_WRONLY | O_TRUNC, 0666);
                        strcpy(response_response, "200 OK\r\n");
                        strcpy(response_header, "Content-Length: 3\r\n\r\n");
                        strcpy(response_message, "OK\n");

                        if (f_two == -1) {
                            f_two = creat(filepath, 0666);
                            if (errno == EACCES) {
                                strcpy(response_response, "403 Forbidden\r\n");
                                strcpy(response_header, "Content-Length: 10\r\n\r\n");
                                strcpy(response_message, "Forbidden\n");
                                write_all(fd, response_destination, strlen(response_destination));
                                write_all(fd, response_response, strlen(response_response));
                                write_all(fd, response_header, strlen(response_header));
                                write_all(fd, response_message, strlen(response_message));
                                free(request.method);
                                free(request.url);
                                free(request.version);
                                free(request.header);
                                free(request.message);
                                close(fd);
                            }

                            strcpy(response_response, "201 Created\r\n");
                            strcpy(response_header, "Content-Length: 8\r\n\r\n");
                            strcpy(response_message, "Created\n");
                        }

                        // checks the version
                        char one = request.version[strlen(request.version) - 3];
                        char two = request.version[strlen(request.version) - 1];

                        if (one != '1' || two != '1') {
                            strcpy(response_response, "505 Version Not Supported\r\n");
                            strcpy(response_header, "Content-Length: 22\r\n\r\n");
                            strcpy(response_message, "Version Not Supported\n");
                            write_all(fd, response_destination, strlen(response_destination));
                            write_all(fd, response_response, strlen(response_response));
                            write_all(fd, response_header, strlen(response_header));
                            write_all(fd, response_message, strlen(response_message));
                            close(f_two);
                            close(fd);
                        }

                        int res = lseek(f_two, 0, SEEK_SET);

                        // writes message to file
                        if (message_size > 0) {
                            res = write_all(f_two, request.message, request.c);
                        }
                        res = pass_bytes(fd, f_two, request.c);

                        if (res == -1) {
                            strcpy(response_response, "500 Internal Server Error\r\n");
                            strcpy(response_header, "Content-Length: 22\r\n\r\n");
                            strcpy(response_message, "Internal Server Error\n");
                        }

                        write_all(fd, response_destination, strlen(response_destination));
                        write_all(fd, response_response, strlen(response_response));
                        write_all(fd, response_header, strlen(response_header));
                        write_all(fd, response_message, strlen(response_message));
                        free(request.method);
                        free(request.url);
                        free(request.version);
                        free(request.header);
                        free(request.message);

                        res = close(f_two);
                    } else {
                        strcpy(response_response, "400 Bad Request\r\n");
                        strcpy(response_header, "Content-Length: 12\r\n\r\n");
                        strcpy(response_message, "Bad Request\n");
                        write_all(fd, response_destination, strlen(response_destination));
                        write_all(fd, response_response, strlen(response_response));
                        write_all(fd, response_header, strlen(response_header));
                        write_all(fd, response_message, strlen(response_message));
                        free(request.method);
                        free(request.url);
                        free(request.version);
                        free(request.header);
                        free(request.message);
                    }
                } else {
                    strcpy(response_response, "501 Not Implemented\r\n");
                    strcpy(response_header, "Content-Length: 16\r\n\r\n");
                    strcpy(response_message, "Not Implemented\n");
                    write_all(fd, response_destination, strlen(response_destination));
                    write_all(fd, response_response, strlen(response_response));
                    write_all(fd, response_header, strlen(response_header));
                    write_all(fd, response_message, strlen(response_message));
                    free(request.method);
                    free(request.url);
                    free(request.version);
                    free(request.header);
                    free(request.message);
                }
            }

            // close connection
            close(fd);
        }
    }

    return 0;
}
