#ifndef FINGERPRINTSENSOR_H
#define FINGERPRINTSENSOR_H

#include "FingerPrintSensor_defines.h"

class R558
{
private:
    uint8_t SerialPort; // es SER_P_COMx
    int baudrate;
    HANDLE serialHandle;

public:
    R558();
    R558(uint8_t serPort, int baudrate);
    ~R558();

    /* Enroll fingerprint (captures twice, creates model and stores at page_id). */
    SENS_StatusTypeDef R558_Enroll(uint16_t page_id);

    /* Verify fingerprint */
    SENS_StatusTypeDef R558_Verify(uint16_t *out_page_id, uint16_t *out_score);

    /* Verify device password. Pass the 32-bit password (module default is often 0x00000000 or 0x00000001). */
    SENS_StatusTypeDef R558_VerifyPassword(uint32_t password);

    /* Clear all templates (empty library) */
    SENS_StatusTypeDef R558_ClearAllFingerprints(void);

    /* Delete Fingerprints starting from -> from_ID, number of FP to be deleted -> num_fp */
    SENS_StatusTypeDef R558_DeleteFingerprints(uint16_t from_ID, uint16_t num_fp);

    void SendHello();
    void GetHello();

    SENS_StatusTypeDef R558_GetTemplateNum(uint16_t *out_temp_num);

private:
    bool openConnection();

    uint16_t R558_BuildCommand(uint8_t *out_buf, uint8_t instruction, const uint8_t *params, uint16_t params_len);
    SENS_StatusTypeDef R558_SendCommand(uint8_t *cmd, uint16_t cmd_len, uint8_t *response, uint16_t response_max_len);
    SENS_StatusTypeDef R558_ClearAllTemplates(void);
    SENS_StatusTypeDef R558_DeleteTemplate(uint16_t page_id, uint16_t count);
    SENS_StatusTypeDef WaitForFingerRemoval(uint32_t timeout_ms);
    SENS_StatusTypeDef WaitForFingerPlacement(uint32_t timeout_ms);
    SENS_StatusTypeDef R558_CaptureFinger(void);
    SENS_StatusTypeDef R558_Image2Tz(uint8_t buffer_id);
    SENS_StatusTypeDef R558_GenerateTemplate(void);
    SENS_StatusTypeDef R558_StoreTemplate(uint8_t buffer_id, uint16_t page_id);
    SENS_StatusTypeDef R558_SearchDatabase(uint16_t *out_page_id, uint16_t *out_score, uint16_t start_page, uint16_t page_num);

    SENS_StatusTypeDef SENS_UART_Transmit(uint8_t *cmd, uint16_t cmd_len);
    SENS_StatusTypeDef SENS_UART_Receive(uint8_t *response, uint16_t resp_len);
};

#define USE_DEBUG_LOG 1 // Debug all the steps
#define USE_MAIN_LOG 1  // only the user intractable Logs

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