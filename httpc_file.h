#ifndef __HTTPC_FILE_H
#define __HTTPC_FILE_H

//#define HTTPC_USE_LWIP
//#define HTTPC_USE_DNS

/* default receive or send timeout */
#define HTTPC_DEFAULT_TIMEO     (6U) /* 6s */

#define HTTPC_URL_SZ            (64U)
#define HTTPC_HOST_SZ           (32U)
#define HTTPC_PORT_SZ           (8U)

#define HTTPC_SEND_BUFSZ        (512U)
#define HTTPC_RECV_BUFSZ        (2048U)

#define HTTPC_GET_RETRY_MAX     (3U)

enum {
    HTTPC_ERR_OK = 0,
    HTTPC_ERR_COMMON,
    HTTPC_ERR_ALLOC,
    HTTPC_ERR_URI_PARSE,
    HTTPC_ERR_CONNECT,
    HTTPC_ERR_SEND_HEADER,
    HTTPC_ERR_RECV_RESP,
    HTTPC_ERR_RESP_PARSE,
    HTTPC_ERR_RECV_DATA
};

/* user provided */
typedef struct {
/* user */
    void *user;
/* uri */
    const char *uri;
/* http connect success */
    uint8_t (*on_connected)(void *);
/* http connect success */
    void (*on_file_parsed)(void *, uint32_t);
/* recv a block of data */
    uint8_t (*on_received)(void *, uint8_t *, uint16_t);
/* http closed */
    uint8_t (*on_closed)(void *, uint8_t);
/* error code */
    uint8_t err_code;
} httpc_user;

void httpc_get_file(httpc_user *user);

#endif

