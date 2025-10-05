
/* Instruction codes (used by implementation) */
#define R558_INS_GENIMG 0x01    /* GenImg: capture image */
#define R558_INS_IMG2TZ 0x02    /* Img2Tz: image -> char (buffer) */
#define R558_INS_REGMODEL 0x05  /* RegModel: merge buffers -> template */
#define R558_INS_STORE 0x06     /* Store template to flash */
#define R558_INS_SEARCH 0x04    /* Search database */
#define R558_INS_DELETE 0x0C    /* Delete templates */
#define R558_INS_EMPTY 0x0D     /* Empty library */
#define R558_INS_VERIFYPWD 0x13 /* Verify password */
#define R558_INS_TEMPNUM 0x1D   /* Template Num */

/* Confirmation codes (common) */
#define R558_CONFIRM_OK 0x00
#define R558_CONFIRM_NOFINGER 0x02
#define R558_CONFIRM_NOMATCH 0x09

/* Baudrate defines */
#define SER_P_BAUDRATE_9600 9600
#define SER_P_BAUDRATE_19200 19200
#define SER_P_BAUDRATE_38400 38400
#define SER_P_BAUDRATE_57600 57600
#define SER_P_BAUDRATE_115200 115200

/* COMs definition */
#define SER_P_COM1 0
#define SER_P_COM2 1
#define SER_P_COM3 2
#define SER_P_COM4 3
#define SER_P_COM5 4
#define SER_P_COM6 5
#define SER_P_COM7 6
#define SER_P_COM8 7
#define SER_P_COM9 8
#define SER_P_COM10 9

typedef enum
{
    SENS_OK = 0x00U,
    SENS_ERROR = 0x01U,
    SENS_BUSY = 0x02U,
    SENS_TIMEOUT = 0x03U
} SENS_StatusTypeDef;