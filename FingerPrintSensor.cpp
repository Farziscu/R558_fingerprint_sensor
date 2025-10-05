
#include <iostream>
#include <string.h>
#include <chrono>

#include <windows.h>

#include "FingerPrintSensor.h"

char Port_Name[][10] = {
    "\\\\.\\COM1",
    "\\\\.\\COM2",
    "\\\\.\\COM3",
    "\\\\.\\COM4",
    "\\\\.\\COM5",
    "\\\\.\\COM6",
    "\\\\.\\COM7",
    "\\\\.\\COM8",
    "\\\\.\\COM9",
    "\\\\.\\COM10",
};

static uint32_t getTick();

bool R558::openConnection()
{
    std::cout << "Connessione al sensore..." << std::endl;
    FP_LOG("Connessione al sensore...");
    FP_LOG("Port: %s", Port_Name[SerialPort]);
    FP_LOG("baudrate: %d", baudrate);
    serialHandle = CreateFile(Port_Name[SerialPort], GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);

    if (serialHandle == INVALID_HANDLE_VALUE)
    {
        // GetLastError....
        std::cout << "INVALID_HANDLE_VALUE : errore creating connection!" << std::endl;
        return false;
    }

    // Do some basic settings
    DCB serialParams = {0};
    serialParams.DCBlength = sizeof(serialParams);

    if (!GetCommState(serialHandle, &serialParams))
    {
        std::cerr << "Errore in GetCommState. Codice: " << GetLastError() << std::endl;
        CloseHandle(serialHandle);
        return false;
    }
    serialParams.BaudRate = baudrate;
    serialParams.ByteSize = 8;
    serialParams.StopBits = ONESTOPBIT;
    serialParams.Parity = NOPARITY;
    if (!SetCommState(serialHandle, &serialParams))
    {
        std::cerr << "Errore in SetCommState. Codice: " << GetLastError() << std::endl;
        CloseHandle(serialHandle);
        return false;
    }

    // Set timeouts
    COMMTIMEOUTS timeout = {0};

#if 0
    // Opzione 1: Bloccante fino a un byte
    timeout.ReadIntervalTimeout = MAXDWORD;
    timeout.ReadTotalTimeoutConstant = 0;
#else
    // Opzione 2: Timeout massimo 1 secondi (raccomandato)
    timeout.ReadIntervalTimeout = 0;
    timeout.ReadTotalTimeoutConstant = 500; // 4000;
#endif
    timeout.ReadTotalTimeoutMultiplier = 0;

    timeout.WriteTotalTimeoutConstant = 50;
    timeout.WriteTotalTimeoutMultiplier = 10;

    if (!SetCommTimeouts(serialHandle, &timeout))
    {
        std::cerr << "Errore in SetCommTimeouts. Codice: " << GetLastError() << std::endl;
        CloseHandle(serialHandle);
        return false;
    }

    return true;
}

R558::R558()
{
    SerialPort = SER_P_COM4;
    baudrate = SER_P_BAUDRATE_57600;
    openConnection();
}

R558::R558(uint8_t serPort, int baudrate)
{
    SerialPort = serPort;
    this->baudrate = baudrate;
    openConnection();
}

R558::~R558()
{
    std::cout << "Closing connection!" << std::endl;
    CloseHandle(serialHandle);
}

///////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////    PUBLIC METHODS    /////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////

/* Enroll fingerprint (captures twice, creates model and stores at page_id). */
SENS_StatusTypeDef R558::R558_Enroll(uint16_t page_id)
{
    FP_LOG("\r\n=== ENROLL FINGERPRINT ===");
    FP_LOG("Target ID = %d", page_id);

    // 1st capture
    FP_LOG("Place your finger...");
    if (WaitForFingerPlacement(20000) != SENS_OK)
    {
        return SENS_TIMEOUT;
    }

    FP_LOG("Capturing finger...");
    while (R558_CaptureFinger() != SENS_OK)
    {
        Sleep(200); // wait and retry
    }

    if (R558_Image2Tz(1) != SENS_OK)
    {
        FP_LOG("Failed to convert first image");
        return SENS_ERROR;
    }
    FP_LOG("First image captured and converted");

    // Wait until finger is removed (with timeout)
    FP_LOG("Please remove your finger...");
    if (WaitForFingerRemoval(10000) != SENS_OK)
    {
        return SENS_TIMEOUT;
    }

    FP_LOG("\n\n");

    // 2nd capture
    FP_LOG("Place the same finger again...");
    if (WaitForFingerPlacement(20000) != SENS_OK)
    {
        return SENS_TIMEOUT;
    }

    FP_LOG("Capturing finger again...");
    while (R558_CaptureFinger() != SENS_OK)
    {
        Sleep(200); // wait and retry
    }

    if (R558_Image2Tz(2) != SENS_OK)
    {
        FP_LOG("Failed to convert second image");
        return SENS_ERROR;
    }
    FP_LOG("Second image captured and converted");

    FP_LOG("Please remove your finger...");
    if (WaitForFingerRemoval(10000) != SENS_OK)
    {
        return SENS_TIMEOUT;
    }

    FP_LOG("\n\n Acquisition completed.");

    // Merge buffers into template
    FP_LOG("Generating template (merge)");
    if (R558_GenerateTemplate() != SENS_OK)
    {
        FP_LOG("Failed to generate template (merge)");
        return SENS_ERROR;
    }
    FP_LOG("Template merged");

    // Store model at page_id (store from buffer 1)
    if (R558_StoreTemplate(1, page_id) != SENS_OK)
    {
        FP_LOG("Failed to store template");
        return SENS_ERROR;
    }

    FP_LOG("Enroll successful. Stored at ID=%d", page_id);

    return SENS_OK;
}

/* Verify fingerprint (capture -> convert -> search over library range 0..1000).
* out_page_id / out_score are optional outputs.
*
Fingerprint Match Score Reference (R558):

Score Range        Meaning
-----------        -------
0 - 50             Poor match, likely not the same finger
51 - 100           Weak match, possible false match
101 - 150          Acceptable match for casual use
151 - 200          Good match, very likely the same finger
>200               Excellent match, highly reliable

Default threshold for considering a fingerprint as matched: 150
*/
SENS_StatusTypeDef R558::R558_Verify(uint16_t *out_page_id, uint16_t *out_score)
{
    SENS_StatusTypeDef status;

    FP_LOG("\r\n=== VERIFY FINGERPRINT ===");
    FP_LOG("Place your finger...");
    if (WaitForFingerPlacement(20000) != SENS_OK)
    {
        return SENS_TIMEOUT;
    }

    FP_LOG("Capturing finger...");
    while (R558_CaptureFinger() != SENS_OK)
    {
        Sleep(200); // wait and retry
    }

    if (R558_Image2Tz(1) != SENS_OK)
    {
        return SENS_ERROR;
    }

    // Search entire library; change range if you have different capacity
    uint16_t page_id = 0, score = 0;
    FP_LOG("Searching Database...");
    if (R558_SearchDatabase(&page_id, &score, 0x0000, 0x03E8) == SENS_OK)
    {
        FP_LOG("Match found: ID=%d Score=%d", page_id, score);
        if (out_page_id)
            *out_page_id = page_id;
        if (out_score)
            *out_score = score;
        status = SENS_OK;
    }
    else
    {
        status = SENS_ERROR;
    }

    FP_LOG("Please remove your finger...");

    //       if (WaitForFingerRemoval(10000) != SENS_OK) {
    //           status = SENS_TIMEOUT;
    //       }
    return status;
}

/* Verify device password. Pass the 32-bit password (module default is often 0x00000000 or 0x00000001). */
SENS_StatusTypeDef R558::R558_VerifyPassword(uint32_t password)
{
    // password: 32-bit (most modules default 0x00000000 or 0x00000001)
    uint8_t params[4] = {
        static_cast<uint8_t>((password >> 24) & 0xFF),
        static_cast<uint8_t>((password >> 16) & 0xFF),
        static_cast<uint8_t>((password >> 8) & 0xFF),
        static_cast<uint8_t>((password >> 0) & 0xFF)};
    uint8_t cmd[32];
    uint16_t cmd_len = R558_BuildCommand(cmd, R558_INS_VERIFYPWD, params, 4);

    uint8_t resp[32] = {0};
    DB_LOG("Verifying sensor password...");
    if (R558_SendCommand(cmd, cmd_len, resp, sizeof(resp)) == SENS_OK)
    {
        // resp[6] = packet identifier (0x07 = ACK), resp[9] = confirmation code
        if (resp[6] == 0x07 && resp[9] == R558_CONFIRM_OK)
        {
            FP_LOG("Password verified. Sensor initialized!");
            return SENS_OK;
        }
        DB_LOG("Verify failed (confirmation=0x%02X)", resp[9]);
        return SENS_ERROR;
    }

    DB_LOG("UART timeout while waiting for verify response");
    return SENS_ERROR;
}

// Clear all templates (empty library)
SENS_StatusTypeDef R558::R558_ClearAllFingerprints(void)
{
    return R558_ClearAllTemplates();
}

// Delete Fingerprints starting from -> from_ID, number of FP to be deleted -> num_fp
SENS_StatusTypeDef R558::R558_DeleteFingerprints(uint16_t from_ID, uint16_t num_fp)
{
    return R558_DeleteTemplate(from_ID, num_fp);
}

///////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////    PRIVATE METHODS    ////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////

SENS_StatusTypeDef R558::SENS_UART_Transmit(uint8_t *cmd, uint16_t cmd_len)
{
    FlushFileBuffers(serialHandle);

    DB_LOG("Sending data - cmd_len == %d", cmd_len);

    printf("SENS_UART_Transmit : ");
    for (int i = 0; i < cmd_len; i++)
        printf(" 0x%X; ", cmd[i]);

    printf("\n");

    if (WriteFile(serialHandle, cmd, cmd_len, NULL, NULL) == false)
    {
        DB_LOG("Error Sending data");
        return SENS_ERROR;
    }
    return SENS_OK;
}
SENS_StatusTypeDef R558::SENS_UART_Receive(uint8_t *response, uint16_t resp_len)
{
    DWORD err;
    DWORD numByteRead = 0;
    if (ReadFile(serialHandle, response, resp_len, &numByteRead, NULL) == false)
    {
        err = GetLastError();
        DB_LOG("Error Receivin data: errNo == %d");
        return SENS_ERROR;
    }

    if (numByteRead == 0)
    {
        err = GetLastError();
        if (err == ERROR_NOT_SUPPORTED)
        {
            DB_LOG("Error : ERROR_NOT_SUPPORTED");
        }
        else
        {
            DB_LOG("error no: %d", err);
        }

        DB_LOG("Error Receivin data - numByteRead == 0");
        return SENS_TIMEOUT;
    }

    DB_LOG("Reveiving data - resp_len == %d", resp_len);
    DB_LOG("Receivin data - numByteRead == %d", numByteRead);
    printf("SENS_UART_Receive : ");
    for (int i = 0; i < numByteRead; i++)
        printf(" 0x%X; ", response[i]);

    printf("\n");

    return SENS_OK;
}

// Helper: build command packet into out_buf, return packet length
// out_buf must be large enough (> 32 bytes recommended)
uint16_t R558::R558_BuildCommand(uint8_t *out_buf, uint8_t instruction, const uint8_t *params, uint16_t params_len)
{
    uint16_t idx = 0;

    // Header + address
    out_buf[idx++] = 0xEF;
    out_buf[idx++] = 0x01;
    out_buf[idx++] = 0xFF;
    out_buf[idx++] = 0xFF;
    out_buf[idx++] = 0xFF;
    out_buf[idx++] = 0xFF;

    // Packet identifier: command packet
    out_buf[idx++] = 0x01;

    // Length = Instruction(1) + params_len + checksum(2)
    uint16_t length_field = 1 + params_len + 2;
    out_buf[idx++] = (length_field >> 8) & 0xFF;
    out_buf[idx++] = length_field & 0xFF;

    // Instruction
    out_buf[idx++] = instruction;

    // Params
    if (params_len && params != NULL)
    {
        memcpy(&out_buf[idx], params, params_len);
        idx += params_len;
    }

    // Compute checksum: sum of bytes from packet identifier (index 6) to last data byte
    uint16_t sum = 0;
    for (uint16_t i = 6; i < idx; ++i)
        sum += out_buf[i];

    // Append checksum (2 bytes)
    out_buf[idx++] = (sum >> 8) & 0xFF;
    out_buf[idx++] = sum & 0xFF;

    return idx;
}

// Send command and receive response. response_max_len must be size of response buffer.
// This function will fill response[] and return SENS_OK on UART success (even if the R558 returns error codes).
SENS_StatusTypeDef R558::R558_SendCommand(uint8_t *cmd, uint16_t cmd_len,
                                          uint8_t *response, uint16_t response_max_len)
{
    SENS_StatusTypeDef status;

    // Transmit full command
    status = SENS_UART_Transmit(cmd, cmd_len);
    if (status != SENS_OK)
    {
        DB_LOG("UART transmit failed");
        return status;
    }

    // Receive header (9 bytes): EF01 + Addr(4) + PID + LEN_H + LEN_L
    status = SENS_UART_Receive(response, 9);
    if (status != SENS_OK)
    {
        DB_LOG("UART receive failed (header)");
        return status;
    }

    // Get length field (this length already includes checksum)
    // for (int i = 0; i < 9; i++)
    //    printf("%d - 0x%X; ", i, response[i]);

    uint16_t payload_len = ((uint16_t)response[7] << 8) | response[8];
    uint16_t total_len = 9 + payload_len; // header + payload_len (payload includes checksum)

    if (total_len > response_max_len)
    {
        DB_LOG("Response too large (%d bytes, buffer max %d)", total_len, response_max_len);
        // Drain remaining bytes if needed? For now return error
        return SENS_ERROR;
    }

    // Read rest of packet (payload_len bytes)
    if (payload_len > 0)
    {
        status = SENS_UART_Receive(&response[9], payload_len);
        if (status != SENS_OK)
        {
            DB_LOG("UART receive failed (payload)");
            return status;
        }
    }

    return SENS_OK;
}

// Clear all templates (empty library) - instruction 0x0D (R558_INS_EMPTY) (Empty)
SENS_StatusTypeDef R558::R558_ClearAllTemplates(void)
{
    uint8_t cmd[16];
    uint16_t cmd_len = R558_BuildCommand(cmd, R558_INS_EMPTY, NULL, 0);

    uint8_t resp[32] = {0};
    DB_LOG("Clearing all templates...");
    if (R558_SendCommand(cmd, cmd_len, resp, sizeof(resp)) == SENS_OK)
    {
        if (resp[9] == R558_CONFIRM_OK)
        {
            DB_LOG("All templates cleared");
            return SENS_OK;
        }
        else
        {
            DB_LOG("Clear all failed (code=0x%02X)", resp[9]);
            return SENS_ERROR;
        }
    }
    return SENS_ERROR;
}

// Delete templates starting from page_id, count entries
SENS_StatusTypeDef R558::R558_DeleteTemplate(uint16_t page_id, uint16_t count)
{
    uint8_t params[4] = {
        (uint8_t)((page_id >> 8) & 0xFF),
        (uint8_t)(page_id & 0xFF),
        (uint8_t)((count >> 8) & 0xFF),
        (uint8_t)(count & 0xFF)};
    uint8_t cmd[32];
    uint16_t cmd_len = R558_BuildCommand(cmd, R558_INS_DELETE, params, sizeof(params));

    uint8_t resp[32] = {0};
    DB_LOG("Deleting %d template(s) starting at ID=%d...", count, page_id);
    if (R558_SendCommand(cmd, cmd_len, resp, sizeof(resp)) == SENS_OK)
    {
        if (resp[9] == R558_CONFIRM_OK)
        {
            DB_LOG("Delete OK");
            return SENS_OK;
        }
        else
        {
            DB_LOG("Delete failed (code=0x%02X)", resp[9]);
            return SENS_ERROR;
        }
    }
    return SENS_ERROR;
}

SENS_StatusTypeDef R558::WaitForFingerRemoval(uint32_t timeout_ms)
{
    uint32_t start = getTick();

    while (1)
    {
        uint8_t resp[12];
        uint8_t cmd[16];
        uint16_t cmd_len = R558_BuildCommand(cmd, R558_INS_GENIMG, NULL, 0);

        // Send capture finger command, do not spam logs
        R558_SendCommand(cmd, cmd_len, resp, sizeof(resp));

        // resp[9] = confirmation code
        if (resp[9] == R558_CONFIRM_NOFINGER) // R558_CONFIRM_NOFINGER (0x02) = no finger detected
        {
            FP_LOG("Finger removed");
            return SENS_OK;
        }

        // Check timeout
        if (getTick() - start > timeout_ms)
        {
            FP_LOG("Timeout: finger not removed in time");
            return SENS_TIMEOUT;
        }

        Sleep(300); // Wait before next check
    }
}

/**
 * @brief Wait until a finger is placed on the sensor.
 * @param timeout_ms Maximum time to wait in milliseconds.
 * @return SENS_StatusTypeDef SENS_OK if finger detected, SENS_TIMEOUT if timeout occurs.
 */
SENS_StatusTypeDef R558::WaitForFingerPlacement(uint32_t timeout_ms)
{
    uint32_t start = getTick();
    SENS_StatusTypeDef send_ret = SENS_ERROR;

    while (1)
    {
        uint8_t resp[12];
        uint8_t cmd[16];
        uint16_t cmd_len = R558_BuildCommand(cmd, R558_INS_GENIMG, NULL, 0);

        // Send capture finger command
        send_ret = R558_SendCommand(cmd, cmd_len, resp, sizeof(resp));

        if (send_ret == SENS_OK)
        {
            // for (int i = 0; i < 12; i++)
            //     printf("%d - 0x%X; ", i, resp[i]);

            // resp[9] = confirmation code
            if (resp[9] == R558_CONFIRM_OK) // 0x00 = finger detected
            {
                FP_LOG("Finger detected");
                return SENS_OK;
            }
        }

        // Check timeout
        if (getTick() - start > timeout_ms)
        {
            FP_LOG("Timeout: finger not placed in time");
            return SENS_TIMEOUT;
        }

        Sleep(300); // Wait before next check
    }
}

// GenImg - Capture fingerprint image (no params / no toggles). The R558 returns codes:
// 0x00 success, other values indicate errors (e.g., no finger, movement, etc.)
SENS_StatusTypeDef R558::R558_CaptureFinger(void)
{
    uint8_t cmd[16];
    uint16_t cmd_len = R558_BuildCommand(cmd, R558_INS_GENIMG, NULL, 0);

    uint8_t resp[32] = {0};
    DB_LOG("Capturing finger...");
    if (R558_SendCommand(cmd, cmd_len, resp, sizeof(resp)) == SENS_OK)
    {
        if (resp[9] == R558_CONFIRM_OK)
        {
            DB_LOG("Finger image captured");
            return SENS_OK;
        }
        else
        {
            DB_LOG("Capture failed (code=0x%02X)", resp[9]);
            return SENS_ERROR;
        }
    }
    return SENS_ERROR;
}

// Img2Tz - Convert image to characteristic file (buffer 1 or 2)
SENS_StatusTypeDef R558::R558_Image2Tz(uint8_t buffer_id)
{
    if (buffer_id != 1 && buffer_id != 2)
    {
        DB_LOG("Invalid buffer id %d (must be 1 or 2)", buffer_id);
        return SENS_ERROR;
    }
    uint8_t params[1] = {buffer_id};
    uint8_t cmd[16];
    uint16_t cmd_len = R558_BuildCommand(cmd, R558_INS_IMG2TZ, params, 1);

    uint8_t resp[32] = {0};
    DB_LOG("Converting image to char file (buffer %d)...", buffer_id);
    if (R558_SendCommand(cmd, cmd_len, resp, sizeof(resp)) == SENS_OK)
    {
        if (resp[9] == R558_CONFIRM_OK)
        {
            DB_LOG("Image -> char (buffer %d) OK", buffer_id);
            return SENS_OK;
        }
        else
        {
            DB_LOG("Image2Tz failed (code=0x%02X)", resp[9]);
            return SENS_ERROR;
        }
    }
    return SENS_ERROR;
}

// RegModel - Generate template from char files in buffer1 & buffer2 (merge)
SENS_StatusTypeDef R558::R558_GenerateTemplate(void)
{
    // instruction R558_INS_REGMODEL (0x05), no params
    uint8_t cmd[16];
    uint16_t cmd_len = R558_BuildCommand(cmd, R558_INS_REGMODEL, NULL, 0);

    uint8_t resp[32] = {0};
    DB_LOG("Generating template from buffer1 & buffer2...");
    if (R558_SendCommand(cmd, cmd_len, resp, sizeof(resp)) == SENS_OK)
    {
        if (resp[9] == R558_CONFIRM_OK)
        {
            DB_LOG("Template generated (model created)");
            return SENS_OK;
        }
        else
        {
            DB_LOG("RegModel failed (code=0x%02X)", resp[9]);
            return SENS_ERROR;
        }
    }
    return SENS_ERROR;
}

// Store template (store model from buffer into flash page)
// buffer_id usually 1, page_id is 0..(max-1)
SENS_StatusTypeDef R558::R558_StoreTemplate(uint8_t buffer_id, uint16_t page_id)
{
    uint8_t params[3] = {
        buffer_id,
        (uint8_t)((page_id >> 8) & 0xFF),
        (uint8_t)(page_id & 0xFF)};
    uint8_t cmd[32];
    uint16_t cmd_len = R558_BuildCommand(cmd, R558_INS_STORE, params, 3);

    uint8_t resp[32] = {0};
    DB_LOG("Storing template from buffer %d to page %d...", buffer_id, page_id);
    if (R558_SendCommand(cmd, cmd_len, resp, sizeof(resp)) == SENS_OK)
    {
        if (resp[9] == R558_CONFIRM_OK)
        {
            DB_LOG("Template stored at ID=%d", page_id);
            return SENS_OK;
        }
        else
        {
            DB_LOG("Store failed (code=0x%02X)", resp[9]);
            return SENS_ERROR;
        }
    }
    return SENS_ERROR;
}

// Search: search buffer 1 over a range (start_page, page_num)
SENS_StatusTypeDef R558::R558_SearchDatabase(uint16_t *out_page_id, uint16_t *out_score,
                                             uint16_t start_page, uint16_t page_num)
{
    // params: bufferID(1), startPage(2), pageNum(2)
    uint8_t params[5] = {
        0x01, // buffer 1
        (uint8_t)((start_page >> 8) & 0xFF),
        (uint8_t)(start_page & 0xFF),
        (uint8_t)((page_num >> 8) & 0xFF),
        (uint8_t)(page_num & 0xFF)};
    uint8_t cmd[32];
    uint16_t cmd_len = R558_BuildCommand(cmd, R558_INS_SEARCH, params, sizeof(params));

    uint8_t resp[32] = {0};
    DB_LOG("Searching database (start=%d, num=%d)...", start_page, page_num);
    if (R558_SendCommand(cmd, cmd_len, resp, sizeof(resp)) == SENS_OK)
    {
        // resp[9] == confirmation; if 0x00, next bytes are pageid(2) score(2)
        if (resp[9] == R558_CONFIRM_OK)
        {
            if (out_page_id)
                *out_page_id = ((uint16_t)resp[10] << 8) | resp[11];
            if (out_score)
                *out_score = ((uint16_t)resp[12] << 8) | resp[13];
            DB_LOG("Match found: ID=%d Score=%d", (out_page_id ? *out_page_id : 0), (out_score ? *out_score : 0));
            return SENS_OK;
        }
        else if (resp[9] == R558_CONFIRM_NOMATCH)
        {
            DB_LOG("No matching fingerprint found (code=0x09)");
            return SENS_ERROR;
        }
        else
        {
            DB_LOG("Search failed (code=0x%02X)", resp[9]);
            return SENS_ERROR;
        }
    }
    return SENS_ERROR;
}

SENS_StatusTypeDef R558::R558_GetTemplateNum(uint16_t *out_temp_num)
{
    uint8_t cmd[16];
    uint16_t cmd_len = R558_BuildCommand(cmd, R558_INS_TEMPNUM, NULL, 0);

    uint8_t resp[32] = {0};
    DB_LOG("Reading current valid template number...");
    if (R558_SendCommand(cmd, cmd_len, resp, sizeof(resp)) == SENS_OK)
    {
        if (resp[9] == R558_CONFIRM_OK)
        {
            DB_LOG("Read template number OK.");
            //*out_temp_num = (uint16_t)resp[10];
            *out_temp_num = ((uint16_t)resp[10] << 8) | resp[11];
            return SENS_OK;
        }
        else
        {
            DB_LOG("Read template number failed!");
            return SENS_ERROR;
        }
    }
    return SENS_ERROR;
}

///////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////    HELPER FUNCTIONS   ////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////

void R558::SendHello()
{
    char helloString[] = "Hello World form C++ program!\n\r";

    if (serialHandle == INVALID_HANDLE_VALUE)
    {
        std::cout << "INVALID_HANDLE_VALUE : cannot send data." << std::endl;
    }
    else
    {

        std::cout << "Sending hello...." << std::endl;

        if (SENS_UART_Transmit((uint8_t *)helloString, strlen(helloString)) != SENS_OK)
        // if (WriteFile(serialHandle, helloString, strlen(helloString), NULL, NULL) == false)
        {
            std::cout << "Error Sending data" << std::endl;
        }
    }
}

void R558::GetHello()
{
    char helloString[50];
    DWORD NoOfBytesRead = 0;

    if (serialHandle == INVALID_HANDLE_VALUE)
    {
        std::cout << "INVALID_HANDLE_VALUE : cannot receive data." << std::endl;
    }
    else
    {

        std::cout << "Receiving hello.... " << std::endl;

        if (SENS_UART_Receive((uint8_t *)helloString, sizeof(helloString)) != SENS_OK)
        // if (ReadFile(serialHandle, helloString, sizeof(helloString), &NoOfBytesRead, NULL) == false)
        {
            std::cout << "Error Receivin data" << std::endl;
        }
        else
        {
            std::cout << "Received msg: " << helloString << std::endl;
        }
    }
}

static uint32_t getTick()
{
    using namespace std::chrono;
    return static_cast<uint32_t>(duration_cast<milliseconds>(
                                     steady_clock::now().time_since_epoch())
                                     .count());
}