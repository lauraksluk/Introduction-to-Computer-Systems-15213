/*
 * Name: Laura (Kai Sze) Luk
 * AndrewID: kluk
 *
 * proxy.c - A web proxy that accepts incoming connections, reads, and parses
 *          requests.
 *           The proxy then will search through their web-content-cache for
 *          the response corresponding to the URI request.
 *              - If found, proxy immediately forwards the response
 *          to the client.
 *              - Otherwise, it will send the request to web servers, through
 *          a GET (HTTP) request, including request headers. The proxy then
 *          reads the servers' responses, and forward the responses to the
 *          corresponding clients.
 *                  (these new responses are added into the cache)
 *
 *           Network communications are implemented with sockets.
 *           Multiple concurrent connections allowed, through the use of
 *          threads and mutex (locks) to control the accessing of shared
 *          variables/data structures.
 *
 */

#include "cache.h"
#include "csapp.h"

#include <assert.h>
#include <ctype.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/types.h>

// request header strings
static const char *user_agent_header = "User-Agent: Mozilla/5.0"
                                       " (X11; Linux x86_64; rv:3.10.0)"
                                       " Gecko/20191101 Firefox/63.0.1\r\n";
static const char *connection_header = "Connection: close\r\n";
static const char *proxy_connection_header = "Proxy-Connection: close\r\n";
static const char *version_head = "HTTP/1.0\r\n";

// helper functions declaration
void doit(int *argp);
void read_request(int fd, char *host, char *hostname, char *port, char *path,
                  char *uri, char *req);
int send_request_to_server(char *hostname, char *port, char *request);
void send_response_to_client(int client_fd, int server_fd, char *response,
                             size_t *size);
void clienterror(int fd, const char *errnum, const char *shortmsg,
                 const char *longmsg);

// cache (recently accessed web-content)
cache_t *web_cache;

// lock (for threads)
static pthread_mutex_t mutex;

// time counter (to be used in determining LRU evictions)
static size_t time_count = 0;

/**
 * @brief Main function that starts program.
 *
 * This function obtains listening port from command line, and opens its
 * file descriptor. Then, a connection (connecting file decriptor) is made.
 * Threads are spawned per request.
 * Additionally, SIGPIPE signals are ignored.
 * Initializing/freeing the cache is done here.
 *
 * Function arguments – number of arguments on command line, and string
 *                      of the command line arguments
 * Return value – general error management
 *
 */
int main(int argc, char **argv) {
    int listenfd, *connfd;
    socklen_t clientlen;
    struct sockaddr_in clientaddr;
    const char *port;
    pthread_t tid;

    // ignore SIGPIPE signals
    Signal(SIGPIPE, SIG_IGN);

    if (argc != 2) {
        return -1;
    }

    // listening port/file descriptor
    port = argv[1];
    listenfd = open_listenfd(port);

    // error handling
    if (listenfd < 0) {
        return -1;
    }
    clientlen = sizeof(clientaddr);

    // initialize cache
    web_cache = init_cache();

    while (1) {

        // allows for sharing (on heap)
        connfd = Malloc(sizeof(int));

        // establish connection
        *connfd = accept(listenfd, (struct sockaddr *)&clientaddr,
                         (socklen_t *)&clientlen);

        // spawn thread per request
        pthread_create(&tid, NULL, (void *)doit, connfd);
    }

    // free cache
    free_cache(web_cache);

    return 0;
}

/**
 * @brief Begins the proxy operations.
 *
 * Each new thread is detached to automatically reap/clean resources.
 * The proxy reads the request from the client (parses).
 * Responses that are already in cache, are directly forwarded to client.
 * Otherwise, an HTTP request is sent to the server.
 * The response from the server is read, and sent back to the client.
 * Additionally, it is inserted into the cache for future lookups.
 *
 * The file descriptors (client and server) are then closed.
 *
 * Function arguments – int pointer to connecting file descriptor
 *
 */
void doit(int *argp) {
    node *found;
    char uri[MAXLINE], host[MAXLINE], path[MAXLINE], request[MAXLINE];
    char hostname[MAXLINE], port[MAXLINE], response[MAX_OBJECT_SIZE];
    int fd, server_fd;
    size_t size = 0;
    size_t len;

    // obtain connection file descriptor
    fd = *argp;

    // detach each new thread
    pthread_detach(pthread_self());

    // read request from client
    read_request(fd, host, hostname, port, path, uri, request);

    // initialize/lock
    if (pthread_mutex_init(&mutex, NULL) != 0) {
        close(fd);
        return;
    }
    pthread_mutex_lock(&mutex);

    // search through cache for request URI
    found = search_cache(web_cache, uri);

    if (found != NULL) {
        // counter updated when cache is accessed
        time_count += 1;

        // forward response from cache to client
        len = read_response(found, response, time_count);
        pthread_mutex_unlock(&mutex);
        rio_writen(fd, response, len);
    } else {
        pthread_mutex_unlock(&mutex);

        // forward request to server
        server_fd = send_request_to_server(hostname, port, request);
        if (server_fd < 0) {
            close(fd);
            return;
        }

        // read servers' response and forward back to client
        send_response_to_client(fd, server_fd, response, &size);

        if (size) {
            pthread_mutex_lock(&mutex);

            // confirm not in cache
            found = search_cache(web_cache, uri);

            if (found == NULL) {
                time_count += 1;
                // add to cache
                add_to_cache(web_cache, uri, response, size, time_count);
            }
            pthread_mutex_unlock(&mutex);
        }
        close(server_fd);
    }
    close(fd);
    free(argp);
}

/**
 * @brief Forwards servers' response back to corresponding client.
 *
 * This function reads the response from the web server, then writes the
 * contents of the response to the client.
 * The last 2 arguments are to allow for storage/passing on info. to
 * other functions.
 *
 * Function arguments – the client file descriptor, server file
 *                      descriptor, string to hold contents of the response,
 *                      and pointer to hold size of response str
 *
 */
void send_response_to_client(int client_fd, int server_fd, char *response,
                             size_t *size) {
    rio_t server_rio;
    char buf[MAX_OBJECT_SIZE], temp[MAX_OBJECT_SIZE];
    size_t leftover = MAX_OBJECT_SIZE;
    size_t n;

    // read response from server
    rio_readinitb(&server_rio, server_fd);

    while ((n = rio_readnb(&server_rio, buf, MAX_OBJECT_SIZE)) > 0) {
        memcpy(temp, buf, n);

        // copy over response (in segments), to store
        if (leftover > 0) {
            memcpy(response + *size, temp, n);
            leftover -= n;
            *size += n;

            // response length exceeds maximum obj size in cache
        } else {
            *size = 0;
        }

        // forward response back to client (error = -1)
        rio_writen(client_fd, temp, n);
    }
}

/**
 * @brief Forwards the client's request to the web server.
 *
 * This function establishes a connection with the server and writes the
 * contents of the request from the client, to the server's file descriptor.
 *
 * Function arguments – the server's hostname, server's port number, and
 *                      the request string from the client
 *
 * Return value – the server's file descriptor (-1 on error)
 *
 */
int send_request_to_server(char *hostname, char *port, char *request) {
    int server_fd;

    // establish connection with server
    server_fd = open_clientfd(hostname, port);

    // error handling
    if (server_fd < 0) {
        return -1;
    }

    // forward request to server (requests treated as text strings)
    if (rio_writen(server_fd, request, strlen(request)) < 0) {
        return -1;
    }

    return server_fd;
}

/**
 * @brief Reads the request from client, and parses it to obtain information.
 *
 * This function obtains and stores:
 *      - the host, hostname, port number, path, URI
 *
 * Request headers are constructed and saved in the function argument string
 * req.
 *
 * Function arguments – client file descriptor, and strings for the host,
 *                      hostname, port number, path, request contents, and URI
 *
 */
void read_request(int fd, char *host, char *hostname, char *port, char *path,
                  char *uri, char *req) {
    char buf[MAXLINE], method[MAXLINE], version[MAXLINE];
    char tmp[MAXLINE];
    char *temp_port;
    rio_t rio;

    // read request from client
    rio_readinitb(&rio, fd);
    rio_readlineb(&rio, buf, MAXLINE);

    // obtain method, URI, and version
    if (sscanf(buf, "%s %s %s", method, uri, version) != 3) {
        clienterror(fd, "400", "Bad Request",
                    "Proxy received a malformed request");
        return;
    }

    if (strstr(uri, "://")) {

        // obtain host and path from URI
        sscanf(uri, "http://%[^/]%s", host, path);

        // obtain hostname and port number from host
        temp_port = strstr(host, ":");
        if (temp_port) {
            strcpy(port, temp_port + 1);
            sscanf(host, "%[^:]", hostname);

            // default port: 80
        } else {
            strcpy(port, "80");
            strcpy(hostname, host);
        }
    }

    // error handling POST method
    if (!strcmp(method, "POST")) {
        clienterror(fd, "501", "Not Implemented",
                    "Proxy does not implement this method");
    }

    // build request headers
    if (!strcmp(method, "GET")) {
        strcpy(tmp, method);
        strcat(tmp, " ");
        strcat(tmp, path);
        strcat(tmp, " ");
        strcat(tmp, version_head);
        strcpy(req, tmp);

        strcpy(tmp, "Host: ");
        strcat(tmp, host);
        strcat(tmp, "\r\n");
        strcat(req, tmp);

        while (rio_readlineb(&rio, buf, MAXLINE) > 0) {
            if (!strcmp(buf, "\r\n"))
                break;
            if (strstr(buf, "User-Agent"))
                strcat(req, user_agent_header);
            else if (strstr(buf, "Proxy-Connection"))
                strcat(req, proxy_connection_header);
            else if (strstr(buf, "Connection"))
                strcat(req, connection_header);
            else if (strstr(buf, "Host") || strstr(buf, "GET"))
                continue;
            else {
                strcat(req, buf);
            }
        }
        strcat(req, "\r\n");
    }
    return;
}

// Error function from tiny.c (returns err msg to client)
void clienterror(int fd, const char *errnum, const char *shortmsg,
                 const char *longmsg) {
    char buf[MAXLINE];
    char body[MAXBUF];
    size_t buflen;
    size_t bodylen;

    /* Build the HTTP response body */
    bodylen = snprintf(body, MAXBUF,
                       "<!DOCTYPE html>\r\n"
                       "<html>\r\n"
                       "<head><title>Proxy Error</title></head>\r\n"
                       "<body bgcolor=\"ffffff\">\r\n"
                       "<h1>%s: %s</h1>\r\n"
                       "<p>%s</p>\r\n"
                       "<hr /><em>The Proxy Web server</em>\r\n"
                       "</body></html>\r\n",
                       errnum, shortmsg, longmsg);
    if (bodylen >= MAXBUF) {
        return; // Overflow!
    }

    /* Build the HTTP response headers */
    buflen = snprintf(buf, MAXLINE,
                      "HTTP/1.0 %s %s\r\n"
                      "Content-Type: text/html\r\n"
                      "Content-Length: %zu\r\n\r\n",
                      errnum, shortmsg, bodylen);
    if (buflen >= MAXLINE) {
        return; // Overflow!
    }

    /* Write the headers */
    if (rio_writen(fd, buf, buflen) < 0) {
        fprintf(stderr, "Error writing error response headers to client\n");
        return;
    }

    /* Write the body */
    if (rio_writen(fd, body, bodylen) < 0) {
        fprintf(stderr, "Error writing error response body to client\n");
        return;
    }
}