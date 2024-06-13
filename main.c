#include <stdint.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "common.h"
#include "httpc_file.h"

#define FILE_NAME_SIZE      (32U)

typedef struct {
    char filename[FILE_NAME_SIZE];  /* file name */
    int fd;                         /* file descriptor */
    uint64_t tlen;                  /* file total len */
    uint64_t rlen;                  /* file received len */
    uint8_t progress;               /* file download progress */
} t_download;

/* callback when http connected */
static uint8_t _http_connected(void *user)
{
    uint8_t ret = ERR_COMMON;
    t_download *dl = (t_download *)user;
    int fd = -1;

    dl->tlen = 0;
    dl->rlen = 0;
    if ((fd = open(dl->filename, O_CREAT|O_WRONLY|O_TRUNC, 0)) >= 0) {
        printf("httpc_file: connected !\n");
        dl->fd = fd;
        ret = ERR_OK;
    }

    return ret;
}

/* callback when http data header received and parsed */
static void _http_file_parsed(void *user, uint32_t file_len)
{
    t_download *dl = (t_download *)user;

    dl->tlen = file_len;
    printf("httpc_file: parsed, file_len: %d bytes!\n", file_len);

    return;
}

/* callback when http data block received */
static uint8_t _http_received(void *user, uint8_t *data, uint16_t len)
{
    uint8_t ret = ERR_COMMON;
    t_download *dl = (t_download *)user;
    uint16_t _len = 0;

    if (-1 != dl->fd) {
        _len = write(dl->fd, data, len);
        if (_len == len) {
            dl->rlen += len;
            if (dl->progress != (dl->rlen * 100) / dl->tlen) {
                dl->progress = (dl->rlen * 100) / dl->tlen;
                printf("httpc_file: progress: %d%%\n", dl->progress);
            }
            ret = ERR_OK;
        }
    }

    return ret;
}

/* callback when http has closed */
static uint8_t _http_closed(void *user, uint8_t err)
{
    uint8_t ret = ERR_COMMON;
    t_download *dl = (t_download *)user;

    printf("\nhttpc_file: closed,");
    if (-1 != dl->fd) {
        if (0 == close(dl->fd)) {
            if (err) {
                unlink(dl->filename);
                printf(" fail!\n");
            } else {
                printf(" success!\n");
            }
            ret = ERR_OK;
        }
    }

    return ret;
}

int main(int argc,char *argv[])
{
    httpc_user c = {0};
    char uri[64] = {0};
    t_download dl;    /* files to download */

    dl.fd = -1;
    snprintf(dl.filename, FILE_NAME_SIZE, "hello.txt");
    c.user = &dl;
    c.on_connected = _http_connected;
    c.on_file_parsed = _http_file_parsed;
    c.on_received = _http_received;
    c.on_closed = _http_closed;
    snprintf(uri, sizeof(uri) - 1, "http://%s:%s/%s", "172.16.166.138", "80", dl.filename);
    c.uri = uri;
    httpc_get_file(&c);

    return 0;
}

