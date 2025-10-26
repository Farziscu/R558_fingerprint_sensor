#ifndef FINGERPRINTSENSOR_H
#define FINGERPRINTSENSOR_H

#include "FingerPrintSensor_defines.h"

#define USE_MAIN_LOG 1  // only the user intractable Logs
#define USE_DEBUG_LOG 1 // Debug all the steps

#define LOG_UART_READ_WRITE 0 // Prints in/out buffer over serial

class R558
{
private:
    uint8_t SerialPort; // es SER_P_COMx
    int baudrate;
    HANDLE serialHandle = INVALID_HANDLE_VALUE;

    SystemParam_t SystemParameters;
    bool isNotepadDataValid = false;       // true means IDs_Table is valid
    uint8_t IDs_Table[R558_PAGE_SIZE_MAX]; // User Notepad data. 1 page size (32 bytes). Used to store fingerprints indexes

public:
    R558();
    R558(uint8_t serPort, int baudrate);
    ~R558();

    SENS_StatusTypeDef R558_isConnected() { return (serialHandle == INVALID_HANDLE_VALUE) ? SENS_ERROR : SENS_OK; };

    /* Enroll fingerprint (captures for times, creates model and stores at page_id). */
    SENS_StatusTypeDef R558_Enroll(uint16_t page_id);

    /* Enroll fingerprint: calculate next index and then call Enroll into the next index */
    SENS_StatusTypeDef R558_EnrollNextIndex();

    /* Verify fingerprint */
    SENS_StatusTypeDef R558_Verify(uint16_t *out_page_id, uint16_t *out_score);

    /* Check accuracy */
    SENS_StatusTypeDef R558_CheckAccuracy(uint16_t *accuracyVal, uint16_t page_id);

    /* Verify device password. Pass the 32-bit password (module default is often 0x00000000 or 0x00000001). */
    SENS_StatusTypeDef R558_VerifyPassword(uint32_t password);

    /* Clear all templates (empty library) */
    SENS_StatusTypeDef R558_ClearAllFingerprints(void);

    /* Delete Fingerprints starting from -> from_ID, number of FP to be deleted -> num_fp */
    SENS_StatusTypeDef R558_DeleteFingerprints(uint16_t from_ID, uint16_t num_fp);

    // SENS_StatusTypeDef R558_ManageLED();
    SENS_StatusTypeDef R558_ManageLED(const uint8_t *params);

    SENS_StatusTypeDef R558_Sleep();

    /* Write module registers */
    SENS_StatusTypeDef R558_WriteReg();

    /* GET GENERIC INFORMATION FROM THE MODULE */

    /* Check whether the module works properly */
    SENS_StatusTypeDef R558_HandShake(void);

    /* Check whether the sensor works properly */
    SENS_StatusTypeDef R558_CheckSensor(void);

    /* Read the current valid template number of the module */
    SENS_StatusTypeDef R558_GetTemplateNum(uint16_t *out_temp_num);

    /* Read Basic parameter list (16bytes) */
    SENS_StatusTypeDef R558_ReadSystemParameters(void);

    /* Shows the system parameters of the module */
    SENS_StatusTypeDef R558_ShowSystemParameters(void);

    /* Read informaton page (512bytes) */
    SENS_StatusTypeDef R558_ReadInformationPage(uint8_t *infOut, uint16_t infOutLen, uint16_t *totalLen);

    /* Upload the image in Img_Buffer to upper computer */
    SENS_StatusTypeDef R558_UploadImage(uint16_t page_id, uint8_t *infOut, uint16_t infOutLen, uint16_t *totalLen);

    /* Write user's data into FLASH. Input pageId number where userData (32 byte array) will be stored */
    SENS_StatusTypeDef R558_WriteNotepad(uint8_t page_id, uint8_t *userData);

    /* Read user's data into FLASH. Input pageId number to be read and stored into userData ptr */
    SENS_StatusTypeDef R558_ReadNotepad(uint8_t page_id, uint8_t *userData);

private:
    bool openConnection();

    uint16_t R558_BuildCommand(uint8_t *out_buf, uint8_t instruction, const uint8_t *params, uint16_t params_len);
    SENS_StatusTypeDef R558_SendCommand(uint8_t *cmd, uint16_t cmd_len, uint8_t *response, uint16_t response_max_len);
    SENS_StatusTypeDef R558_ClearAllTemplates(void);
    SENS_StatusTypeDef R558_DeleteTemplate(uint16_t page_id, uint16_t count);
    SENS_StatusTypeDef R558_CaptureFinger(void);
    SENS_StatusTypeDef R558_Image2Tz(uint8_t buffer_id);
    SENS_StatusTypeDef R558_GenerateTemplate(void);
    SENS_StatusTypeDef R558_StoreTemplate(uint8_t buffer_id, uint16_t page_id);
    SENS_StatusTypeDef R558_SearchDatabase(uint16_t *out_page_id, uint16_t *out_score, uint16_t start_page, uint16_t page_num);
    SENS_StatusTypeDef R558_LoadChar(uint8_t buffer_id, uint16_t page_id);

    SENS_StatusTypeDef ReceivePackets(uint8_t *infOut, uint16_t infOutMaxLen, uint16_t *totalLen);
    SENS_StatusTypeDef WaitForFingerRemoval(uint32_t timeout_ms);
    SENS_StatusTypeDef WaitForFingerPlacement(uint32_t timeout_ms);
    SENS_StatusTypeDef FindNextIndex(uint8_t *idx);
    SENS_StatusTypeDef UpdateNextIndex(Update_ID choice, uint8_t idx);

    SENS_StatusTypeDef SENS_UART_Transmit(uint8_t *cmd, uint16_t cmd_len);
    SENS_StatusTypeDef SENS_UART_Receive(uint8_t *response, uint16_t resp_len);
};

/* Logging: override FP_LOG before include to route logs elsewhere */
#if USE_MAIN_LOG
#define FP_LOG(fmt, ...) printf("[FP] " fmt "\n", ##__VA_ARGS__)
#else
// If logging is disabled, define an empty macro
#define FP_LOG(fmt, ...) ((void)0)
#endif

#if USE_DEBUG_LOG
#define DB_LOG(fmt, ...) printf("[DB] " fmt "\n", ##__VA_ARGS__)
#else
// If logging is disabled, define an empty macro
#define DB_LOG(fmt, ...) ((void)0)
#endif

#endif // FINGERPRINTSENSOR_H