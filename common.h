#ifndef _COMMON_H_
#define _COMMON_H_

typedef union{
    uint32_t Byte;
    struct{
        uint32_t Bit0  :1;
        uint32_t Bit1  :1;
        uint32_t Bit2  :1;
        uint32_t Bit3  :1; 
        uint32_t Bit4  :1;
        uint32_t Bit5  :1;
        uint32_t Bit6  :1;
        uint32_t Bit7  :1;  
    }Bits;
}BitsFlag;

/* defines of error code */
#define ERR_OK           0             /* OK */
#define ERR_COMMON       1             /* Common error of a device */
#define ERR_RANGE        2             /* Parameter out of range. */
#define ERR_VALUE        3             /* Parameter of incorrect value. */
#define ERR_OVERFLOW     4             /* Timer overflow. */
#define ERR_MATH         5             /* Overflow during evaluation. */
#define ERR_ENABLED      6             /* Device is enabled. */
#define ERR_DISABLED     7             /* Device is disabled. */
#define ERR_BUSY         8             /* Device is busy. */
#define ERR_NOTAVAIL     9             /* Requested value or method not available. */
#define ERR_RXEMPTY      10            /* No data in receiver. */
#define ERR_TXFULL       11            /* Transmitter is full. */
#define ERR_BUSOFF       12            /* Bus not available. */
#define ERR_OVERRUN      13            /* Overrun error is detected. */
#define ERR_FRAMING      14            /* Framing error is detected. */
#define ERR_PARITY       15            /* Parity error is detected. */
#define ERR_IDLE         16            /* Idle error is detected. */
#define ERR_FAULT        17            /* Fault error is detected. */
#define ERR_BREAK        18            /* Break char is received during communication. */
#define ERR_CRC          19            /* CRC error is detected. */
#define ERR_ARBITR       20            /* A node losts arbitration. This error occurs if two nodes start transmission at the same time. */
#define ERR_PROTECT      21            /* Protection error is detected. */
#define ERR_UNDERFLOW    22            /* Underflow error is detected. */
#define ERR_UNDERRUN     23            /* Underrun error is detected. */

#define BOOL_TRUE          1
#define BOOL_FALSE         0

#endif

