
/* Instruction codes (used by implementation) */
#define R558_INS_GETIMG 0x01           /* GetImg: capture image */
#define R558_INS_IMG2TZ 0x02           /* Img2Tz: image -> char (buffer) */
#define R558_INS_PRECISE_MATCHING 0x03 /* To carry out precise matching of two finger templates Match */
#define R558_INS_SEARCH 0x04           /* Search database */
#define R558_INS_REGMODEL 0x05         /* RegModel: merge buffers -> template */
#define R558_INS_STORE 0x06            /* Store template to flash */
#define R558_INS_LOAD_CHAR 0x07        /* Read template from Flash library */
#define R558_INS_UPIMAGE 0x0A          /* To upload image to upper computer */
#define R558_INS_DELETE 0x0C           /* Delete templates */
#define R558_INS_EMPTY 0x0D            /* Empty library */
#define R558_INS_VERIFYPWD 0x13        /* Verify password */
#define R558_INS_READ_INF_PAGE 0x16    /* Read Information Page */
#define R558_INS_TEMPNUM 0x1D          /* Template Num */
#define R558_INS_SLEEP 0x33            /* Sleep */
#define R558_INS_LED_CONTROL 0x3C      /* LED Control */
#define R558_INS_READ_SYS_PARA 0x0F    /* Read system Parameters */

/* LED Control params */
/* Confirmation codes (common) */
#define R558_CONFIRM_OK 0x00
#define R558_CONFIRM_NOFINGER 0x02
#define R558_CONFIRM_NOMATCH 0x09

/* Function number/code */
#define R558_FC_NORMAL_BREATHING 0X01 // normal breathing light,
#define R558_FC_FLASH_LIGHT 0X02      // flashing light,
#define R558_FC_NORMAL_ON_LIGHT 0X03  // normally on light,
#define R558_FC_NORMAL_OFF_LIGHT 0X04 // normally off light,
#define R558_FC_GRADUALLY_ON 0X05     // Gradually on,
#define R558_FC_GRADUALLY_OFF 0X06    // Gradually off

/* Starting color */
#define R558_START_COLOR_BLUE_ON 0x01   // blue light on,
#define R558_START_COLOR_GREEN_ON 0x02  // green light on,
#define R558_START_COLOR_CYAN_ON 0x03   // cyan light on,
#define R558_START_COLOR_RED_ON 0x04    // red light on,
#define R558_START_COLOR_PURPLE_ON 0x05 // purple light on,
#define R558_START_COLOR_YELLOW_ON 0x06 // yellow light on,
#define R558_START_COLOR_WHITE_ON 0x07  // white light on,
#define R558_START_COLOR_ALL_OFF 0x00   // all off.

/* Ending color */
#define R558_END_COLOR_BLUE_ON 0x01   // blue light on,
#define R558_END_COLOR_GREEN_ON 0x02  // green light on,
#define R558_END_COLOR_CYAN_ON 0x03   // cyan light on,
#define R558_END_COLOR_RED_ON 0x04    // red light on,
#define R558_END_COLOR_PURPLE_ON 0x05 // purple light on,
#define R558_END_COLOR_YELLOW_ON 0x06 // yellow light on,
#define R558_END_COLOR_WHITE_ON 0x07  // white light on,
#define R558_END_COLOR_ALL_OFF 0x00   // all off.
/* LED Control params END */

/* Read System Parameters */
#define R558_SYSTEM_PARAM_SIZE 16
#define R558_SYSTEM_PARAM_NAME_SIZE 7

const char SystemParametersName[R558_SYSTEM_PARAM_NAME_SIZE][35] =
    {
        "Enroll Times:",
        "Fingerprint Template Size:",
        "Fingerprint Database Size:",
        "Score level:",
        "Device Address:",
        "Data Packet Size:",
        "Baud Settings: "};

typedef struct
{
    uint16_t EnrollTimes;
    uint16_t FP_Template_Size;
    uint16_t FP_Database_Size;
    uint16_t ScoreLevel;
    uint32_t DeviceAddress;
    uint16_t DataPacketSize;
    uint16_t BaudSettings;
} SystemParam_t;

/* Parameter lists END */

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

/* Conversion big endianess */
#define TO_UINT16_BE(p) (((uint16_t)(p)[0] << 8) | ((uint16_t)(p)[1]))
#define TO_UINT32_BE(p) (((uint32_t)(p)[0] << 24) | ((uint32_t)(p)[1] << 16) | \
                         ((uint32_t)(p)[2] << 8) | ((uint32_t)(p)[3]))
/* Conversion big endianess END */

typedef enum
{
    SENS_OK = 0x00U,
    SENS_ERROR = 0x01U,
    SENS_BUSY = 0x02U,
    SENS_TIMEOUT = 0x03U,
    SENS_NO_MATCH = 0x04U
} SENS_StatusTypeDef;