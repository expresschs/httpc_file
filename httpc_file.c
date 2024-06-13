#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include "common.h"
#include "httpc_file.h"

#ifdef HTTPC_USE_LWIP
    #include "lwip/sockets.h"
    #ifdef HTTPC_USE_DNS
        #include "lwip/netdb.h"
    #endif
    #define httpc_calloc    rt_calloc
    #define httpc_free      rt_free
#else
    #include <sys/socket.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #ifdef HTTPC_USE_DNS
        #include "netdb.h"
    #endif
    #define httpc_calloc    calloc
    #define httpc_free      free
#endif


typedef struct {
    int socket;

    char url[HTTPC_URL_SZ];
    char host[HTTPC_HOST_SZ];
    char port[HTTPC_PORT_SZ];

    int file_len;

    //uint32_t send_timeout;
    //uint32_t recv_timeout;
    //uint32_t retry_cnt;

    uint8_t sbuf[HTTPC_SEND_BUFSZ];
    uint8_t rbuf[HTTPC_RECV_BUFSZ];

    httpc_user *user;
} httpc_session_t;



//static struct iap_http_session g_session;

static uint8_t httpc_connect(httpc_session_t *s)
{
    uint8_t ret = ERR_COMMON;

    struct timeval timeout;
    timeout.tv_sec = HTTPC_DEFAULT_TIMEO;
    timeout.tv_usec = 0;

#ifdef HTTPC_USE_DNS
    struct addrinfo hint = {0};
    struct addrinfo *res = NULL;

    if (0 == getaddrinfo(s->host, s->port, &hint, &res)) {
        if ((s->socket = socket(res->ai_family, SOCK_STREAM, IPPROTO_TCP)) >= 0) {
            setsockopt(s->socket, SOL_SOCKET, SO_RCVTIMEO, (void *) &timeout, sizeof(timeout));
            setsockopt(s->socket, SOL_SOCKET, SO_SNDTIMEO, (void *) &timeout, sizeof(timeout));
            if (0 == connect(s->socket, res->ai_addr, res->ai_addrlen)) {
                if (NULL != s->user->on_connected) {
                    if (ERR_OK == s->user->on_connected(s->user->user)) {
                        ret = ERR_OK;
                    }
                }
            }
        }
        freeaddrinfo(res);
    }
#else
    struct sockaddr_in addr = {0};

    addr.sin_family = AF_INET;
    addr.sin_port = htons(strtoul(s->port, 0, 10));
    addr.sin_addr.s_addr = inet_addr(s->host);
    if ((s->socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) >= 0) {
        setsockopt(s->socket, SOL_SOCKET, SO_RCVTIMEO, (void *) &timeout, sizeof(timeout));
        setsockopt(s->socket, SOL_SOCKET, SO_SNDTIMEO, (void *) &timeout, sizeof(timeout));
        if (0 == connect(s->socket, (struct sockaddr*)&addr, sizeof(addr))) {
            if (NULL != s->user->on_connected) {
                if (ERR_OK == s->user->on_connected(s->user->user)) {
                    ret = ERR_OK;
                }
            }
        }
    }
#endif

    return ret;
}

static int httpc_send_header(httpc_session_t *s)
{
    uint8_t ret = ERR_COMMON;
    uint32_t wsize = 0;
    uint32_t left = 0;
    uint32_t widx = 0;

    if (s->socket >= 0) {
        snprintf((char *)s->sbuf, sizeof(s->sbuf) - 1, \
            "GET %s HTTP/1.1\r\nHost: %s:%s\r\nUser-Agent: Chs HTTP Agent\r\n\r\n", \
            s->url, s->host, s->port);
        left = strlen((const char *)s->sbuf);
        do {
            if ((wsize = send(s->socket, s->sbuf + widx, left, 0)) <= 0) {
                break;
            }
            left -= wsize;
            widx += wsize;
        } while(left);
    }
    if (0 == left) {
        ret = ERR_OK;
    }

    return ret;
}

static int httpc_recv_resp(httpc_session_t *s)
{
    uint8_t ret = ERR_COMMON;
    const char sep[] = {'\r','\n','\r','\n'};
    int idx = 0;
    char c = 0;

    if (s->socket >= 0 && sizeof(sep) < sizeof(s->rbuf)) {
        while (idx < sizeof(s->rbuf)) {
            if (1 == recv(s->socket, &c, 1, 0)) {
                s->rbuf[idx++] = c;
                if (0 == memcmp(&s->rbuf[idx - sizeof(sep)], sep, sizeof(sep))) {
                    s->rbuf[idx++] = '\0';
                    ret = ERR_OK;
                    break;
                }
            } else {
                break;
            }
        }
    }

    return ret;
}

static uint8_t httpc_resp_parse(httpc_session_t *s)
{
    uint8_t ret = ERR_COMMON;
    const char sep[] = {'\r','\n'};
    char tmp[16] = {0};
    char *str1 = NULL, *str2 = NULL;

    str1 = strstr((const char *)s->rbuf, "Content-Length:");
    if(NULL == str1) {
        goto exit;
    }
    str1 += sizeof("Content-Length:") - 1;
    str2 = strstr(str1, sep);
    if (NULL == str2) {
        goto exit;
    }
    if (str2 - str1 >= sizeof(tmp)) {
        goto exit;
    }
    memcpy(tmp, str1, str2 - str1);
    tmp[str2 - str1] = '\0';
    s->file_len = strtoul(tmp, 0, 10);
    if (NULL != s->user->on_file_parsed) {
        s->user->on_file_parsed(s->user->user, s->file_len);
    }
    ret = ERR_OK;

exit:
    return ret;
}

static uint8_t httpc_recv_data(httpc_session_t *s)
{
    uint8_t ret = ERR_COMMON;
    uint32_t rlen = 0;
    uint32_t offset = 0;

    if (s->socket >= 0) {
        for (offset = 0; offset < s->file_len;) {
            rlen = recv(s->socket, s->rbuf, \
                    s->file_len - offset > sizeof(s->rbuf) ? sizeof(s->rbuf) : s->file_len - offset, \
                    0);
            if (rlen <= 0) {
                break;
            } else {
                if (NULL != s->user->on_received) {
                    if (ERR_OK != s->user->on_received(s->user->user, s->rbuf, rlen)) {
                        break;
                    }
                }
            }
            offset += rlen;
        }
    }
    if (offset == s->file_len) {
        ret = ERR_OK;
    }

    return ret;
}



static void httpc_close(httpc_session_t *s)
{
    uint8_t err = 0;

    if (NULL != s && s->socket >= 0) {
        close(s->socket);
        s->socket = -1;
    }
    if (NULL != s && NULL != s->user->on_closed) {
        if (HTTPC_ERR_OK != s->user->err_code) {
            err = 1;
        }
        s->user->on_closed(s->user->user, err);
    }

    return;
}

static uint8_t httpc_uri_parse(httpc_session_t *s)
{
    uint8_t ret = ERR_COMMON;
    char *str1 = NULL, *str2 = NULL, *str3 = NULL;

    str1 = strstr(s->user->uri, "//");
    if (NULL == str1) {
        goto exit;
    }
    str1 += sizeof("//") - 1;

    str2 = strstr(str1, ":");
    if(NULL == str2) {
        str2 = strstr(str1, "/");
        if (NULL == str2) {
            goto exit;
        }
        if (sizeof(s->host) < str2 - str1) {
            goto exit;
        }
        memcpy(s->host, str1, str2 - str1);
        s->host[str2 - str1] = '\0';
        /* default port of 80(http) */
        s->port[0] = '8';
        s->port[1] = '0';
        s->port[2] = '\0';
        if (strlen(str2) >= sizeof(s->url)) {
            goto exit;
        }
        strncpy(s->url, str2, sizeof(s->url));
    } else {
        if (sizeof(s->host) < str2 - str1) {
            goto exit;
        }
        memcpy(s->host, str1, str2 - str1);
        s->host[str2 - str1] = '\0';
        str2 += sizeof(":") - 1;
        str3 = strstr(str2, "/");
        if(NULL == str3) {
            goto exit;
        }
        if (sizeof(s->port) < str3 - str2) {
            goto exit;
        }
        memcpy(s->port, str2, str3 - str2);
        s->port[str3 - str2] = '\0';
        if (sizeof(s->url) < strlen(str3)) {
            goto exit;
        }
        strncpy(s->url, str3, sizeof(s->url));
    }
    ret = ERR_OK;

exit:
    return ret;
}


void httpc_get_file(httpc_user *user)
{
    httpc_session_t *session = NULL;

    user->err_code = HTTPC_ERR_COMMON;
    /* 1 alloc http session */
    if (NULL == (session = httpc_calloc(1, sizeof(httpc_session_t)))) {
        user->err_code = HTTPC_ERR_ALLOC;
        goto exit;
    }
    session->user = user;
    /* 2 parse uri */
    if (ERR_OK != httpc_uri_parse(session)) {
        session->user->err_code = HTTPC_ERR_URI_PARSE;
        goto exit;
    }
    /* 3 connect to http server */
    if (ERR_OK != httpc_connect(session)) {
        session->user->err_code = HTTPC_ERR_CONNECT;
        goto exit;
    }
    /* 4 send http header */
    if (ERR_OK != httpc_send_header(session)) {
        session->user->err_code = HTTPC_ERR_SEND_HEADER;
        goto exit;
    }
    /* 5 receive http response */
    if (ERR_OK != httpc_recv_resp(session)) {
        session->user->err_code = HTTPC_ERR_RECV_RESP;
        goto exit;
    }
    /* 6 parse http response */
    if (ERR_OK != httpc_resp_parse(session)) {
        session->user->err_code = HTTPC_ERR_RESP_PARSE;
        goto exit;
    }
    /* 7 receive file data */
    if (ERR_OK != httpc_recv_data(session)) {
        session->user->err_code = HTTPC_ERR_RECV_DATA;
        goto exit;
    }
    session->user->err_code = HTTPC_ERR_OK;

    /* 8 close http and free session */
exit:
    httpc_close(session);
    httpc_free(session);
    return;
}

