=== 1 - C++ driver for R558 fingerprint sensor. ===
=== 2 - Python R558 fingerprint simulator. ===

=== 1 - C++ driver for R558 fingerprint sensor. ===
It's a simple driver to use R558 finerprint sensor from windows desktop.
It exposes following methods:
    - R558();
    - R558(uint8_t serPort, int baudrate);
    - bool R558_isConnected();
    - SENS_StatusTypeDef R558_Enroll(uint16_t page_id);
    - SENS_StatusTypeDef R558_Verify(uint16_t *out_page_id, uint16_t *out_score);
    - SENS_StatusTypeDef R558_VerifyPassword(uint32_t password);
    - SENS_StatusTypeDef R558_ClearAllFingerprints(void);
    - SENS_StatusTypeDef R558_DeleteFingerprints(uint16_t from_ID, uint16_t num_fp);
    - SENS_StatusTypeDef R558_ManageLED();
    - SENS_StatusTypeDef R558_Sleep();
    - SENS_StatusTypeDef R558_GetTemplateNum(uint16_t *out_temp_num);
    - SENS_StatusTypeDef R558_ReadSystemParameters();
    - SENS_StatusTypeDef R558_ShowSystemParameters();
    - SENS_StatusTypeDef R558_ReadInformationPage(uint8_t *infOut, uint16_t infOutLen, uint16_t *totalLen);
    - SENS_StatusTypeDef R558_UploadImage(uint16_t page_id, uint8_t *infOut, uint16_t infOutLen, uint16_t *totalLen);

=== 2 - Python R558 fingerprint simulator. ===
The script in python simulates the sensor if it is not provided physically.
Press:
    - 'p' to simulate finger presence
    - 'n' to simulate finger removal
    - 'esc' to quit simulator
