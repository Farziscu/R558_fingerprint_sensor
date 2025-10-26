
#include <iostream>
// #include <string.h>
#include <string>
#include <sstream>
#include <iomanip>

#include <chrono>
#include <windows.h>

#include <fstream>
#include <cstdint>

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

static void printArray(const char *name, uint8_t *buf, uint16_t bufLen);
static uint32_t getTick();

bool R558::openConnection()
{
    FP_LOG("Connecting sensor...");
    FP_LOG(" Port       : %s", Port_Name[SerialPort]);
    FP_LOG(" Baudrate   : %d", baudrate);
    serialHandle = CreateFile(Port_Name[SerialPort], GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);

    if (serialHandle == INVALID_HANDLE_VALUE)
    {
        // GetLastError....
        FP_LOG("INVALID_HANDLE_VALUE : error creating connection!");
        return false;
    }

    // Clean input buffer
    PurgeComm(serialHandle, PURGE_RXCLEAR | PURGE_TXCLEAR); // to purge TX buffer use: | PURGE_TXCLEAR);

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
    R558(SER_P_COM4, SER_P_BAUDRATE_57600);
}

R558::R558(uint8_t serPort, int baudrate)
{
    memset(&SystemParameters, 0x00, sizeof(SystemParameters));
    SerialPort = serPort;
    this->baudrate = baudrate;
    openConnection();
}

R558::~R558()
{
    FP_LOG("Closing connection!");
    CloseHandle(serialHandle);
}

///////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////    PUBLIC METHODS    /////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////

/* Enroll fingerprint (captures for times, creates model and stores at page_id). */
SENS_StatusTypeDef R558::R558_Enroll(uint16_t page_id)
{
    FP_LOG("\r\n=== ENROLL FINGERPRINT ===");
    FP_LOG("Target ID = %d", page_id);

    for (int idx = 1; idx < 5; idx++)
    {
        // capture number idx
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

        if (R558_Image2Tz(idx) != SENS_OK)
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

/* Enroll fingerprint: calculate next index and then call Enroll into the next index */
SENS_StatusTypeDef R558::R558_EnrollNextIndex()
{
    uint8_t idx = 0;
    if (FindNextIndex(&idx) == SENS_OK)
    {
        if (R558_Enroll(idx) == SENS_OK)
        {
            return UpdateNextIndex(R558_UPDATE_SET, idx);
        }
    }
    return SENS_ERROR;
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
    status = R558_SearchDatabase(&page_id, &score, 0x0000, 0x0064);
    if (status == SENS_OK)
    {
        FP_LOG("Match found: ID=%d Score=%d", page_id, score);
        if (out_page_id)
            *out_page_id = page_id;
        if (out_score)
            *out_score = score;
    }

    FP_LOG("Please remove your finger...");

    // if (WaitForFingerRemoval(10000) != SENS_OK)
    //{
    //     status = SENS_TIMEOUT;
    // }
    return status;
}

/* Check accuracy */
SENS_StatusTypeDef R558::R558_CheckAccuracy(uint16_t *accuracyVal, uint16_t page_id)
{
    SENS_StatusTypeDef ret = SENS_ERROR;
    uint8_t cmd[16];
    uint16_t cmd_len = 0;
    uint8_t resp[32] = {0};

    uint8_t params[3]; // = {BufferID, PageID (high), PageID(low)}
    uint16_t out_score;

    // LoadChar from Flash library into template buffer 1
    R558_LoadChar(2, page_id);

    ///////////////////////////////////////////////////////////////

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

    ///////////////////////////////////////////////////////////////

    if (ret == SENS_OK)
    {
        cmd_len = R558_BuildCommand(cmd, R558_INS_PRECISE_MATCHING, NULL, 0);
        memset(resp, 0x00, sizeof(resp));
        DB_LOG("Send command for precise matching...");
        if (R558_SendCommand(cmd, cmd_len, resp, sizeof(resp)) == SENS_OK)
        {
            if (resp[9] == R558_CONFIRM_OK)
            {
                DB_LOG("Precise matching command OK! Score == %d - %d", resp[10], resp[11]);
                //*accuracyVal = ((uint16_t)resp[10] << 8) | resp[11];
                *accuracyVal = ((uint16_t *)&resp)[5];
                ret = SENS_OK;
            }
            else
            {
                DB_LOG("Send command for precise matching failed (code=0x%02X)", resp[9]);
                ret = SENS_ERROR;
            }
        }
    }

    return ret;
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

SENS_StatusTypeDef R558::ReceivePackets(uint8_t *infOut, uint16_t infOutMaxLen, uint16_t *totalLen)
{
    uint8_t packageID = 0x00;
    uint16_t payload_len = 0;
    uint16_t countBytes = 0;
    uint16_t total_len = 0;
    uint8_t *infPtr = infOut;
    uint8_t header[32] = {0};

    DB_LOG("Receiving packets...");
    *totalLen = 0;

#if 0 // store in file
    std::ofstream file("output.bin", std::ios::binary);
    if (!file)
    {
        std::cerr << "Errore apertura file per scrittura: " << "output.bin" << "\n";
        return SENS_ERROR;
    }
#endif
    do
    {
        // DB_LOG("infPtr == 0x%X--------------------------------------------------------------", infPtr);
        // DB_LOG("countBytes == %d", countBytes);

        // Receive header (9 bytes): EF01 + Addr(4) + PID + LEN_H + LEN_L
        if (SENS_UART_Receive(header, 9) != SENS_OK)
        {
            DB_LOG("UART receive failed (header)");
            return SENS_ERROR;
        }

        /* Read Package identifier */
        packageID = header[6];

        /* Read Package length */
        payload_len = ((uint16_t)header[7] << 8) | header[8];
        total_len = 9 + payload_len; // header + payload_len (payload includes checksum)

        if ((countBytes + payload_len) > infOutMaxLen)
        {
            DB_LOG("Response too large (%d bytes, buffer max %d)", total_len, 512);
            return SENS_ERROR;
        }

        // Read rest of packet (payload_len bytes)
        if (payload_len > 0)
        {
            // Receive Package data + Checksum
            if (SENS_UART_Receive(infPtr, payload_len) != SENS_OK)
            {
                DB_LOG("UART receive failed (payload)");
                return SENS_ERROR;
            }
        }

#if 0
        file.write(reinterpret_cast<const char *>(infPtr), (payload_len - 2));
        if (!file)
        {
            std::cerr << "Errore durante la scrittura del file.\n";
        }
#endif

        countBytes += (payload_len - 2); // 2 for checksum
        infPtr += (payload_len - 2);

    } while (packageID != 0x08);

    *totalLen = countBytes;
#if 0    
    file.close();
#endif

    return SENS_OK;
}

SENS_StatusTypeDef R558::SENS_UART_Transmit(uint8_t *cmd, uint16_t cmd_len)
{
    FlushFileBuffers(serialHandle);

#if LOG_UART_READ_WRITE
    printArray("SENS_UART_Transmit: ", cmd, cmd_len);
#endif

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
        DB_LOG("Error Receiving data: errNo == %d", err);
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

        DB_LOG("Error Receiving data - numByteRead == 0");
        return SENS_TIMEOUT;
    }

#if LOG_UART_READ_WRITE
    DB_LOG("Expected data len: %d", resp_len);
    printArray("SENS_UART_Receive: ", response, numByteRead);
#endif

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
    //    DB_LOG("%d - 0x%X; ", i, response[i]);

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
        // Get length field (this length already includes checksum)
        // for (int i = 9; i < 9 + payload_len; i++)
        //    DB_LOG("%d - 0x%X; ", i, response[i]);
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
            /* Update Notepad data */
            memset(IDs_Table, 0x00, sizeof(IDs_Table));
            R558_WriteNotepad(1, IDs_Table);
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

            DB_LOG("Updating IDs table...");
            for (uint8_t i = 0; i < count; i++)
            {
                UpdateNextIndex(R558_UPDATE_RESET, page_id + i);
            }
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
        uint16_t cmd_len = R558_BuildCommand(cmd, R558_INS_GETIMG, NULL, 0);

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
        uint16_t cmd_len = R558_BuildCommand(cmd, R558_INS_GETIMG, NULL, 0);

        // Send capture finger command
        send_ret = R558_SendCommand(cmd, cmd_len, resp, sizeof(resp));

        if (send_ret == SENS_OK)
        {
            // for (int i = 0; i < 12; i++)
            //     DB_LOG("%d - 0x%X; ", i, resp[i]);

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

SENS_StatusTypeDef R558::FindNextIndex(uint8_t *idx)
{
    uint8_t selectedByte = 0;
    uint8_t i = 0;

    *idx = 0;

    /* Reading 32 notepad user data */
    if (!isNotepadDataValid)
    {
        if (R558_ReadNotepad(1, IDs_Table) != SENS_OK)
        {
            return SENS_ERROR;
        }
        isNotepadDataValid = true;
    }

    /* first 13(R558_MAX_BYTE_FOR_100_FP) bytes are used to store IDs free */
    while (IDs_Table[i] == 0xFF)
    {
        i++;
        if (i > R558_MAX_BYTE_FOR_100_FP)
            break; // to count 100 (=R558_MAX_FINGERPRINT) fingerprints I need 13(R558_MAX_BYTE_FOR_100_FP) byte.
    }

    if (i < R558_MAX_BYTE_FOR_100_FP)
    {
        selectedByte = IDs_Table[i];

        *idx = *idx + 1;

        while ((selectedByte & 0x80) != 0)
        {
            selectedByte = selectedByte << 1;
            *idx = *idx + 1;
            if (*idx == 8)
                break;
        }

        *idx = *idx + (i * 8);
        if (*idx > R558_MAX_FINGERPRINT)
        {
            return SENS_ERROR;
        }
        return SENS_OK;
    }

    return SENS_ERROR;
}

SENS_StatusTypeDef R558::UpdateNextIndex(Update_ID choice, uint8_t idx)
{
    uint8_t byteNo = (idx - 1) / 8;
    uint8_t bitNo = (idx - 1) % 8;

    if (byteNo < R558_MAX_BYTE_FOR_100_FP)
    {
        if (choice == R558_UPDATE_SET)
        {
            IDs_Table[byteNo] |= (0x80 >> bitNo);
        }
        else
        {
            // choice == R558_UPDATE_RESET
            IDs_Table[byteNo] &= ~(0x80 >> bitNo);
        }

        printArray("UpdateNextIndex", IDs_Table, 13);

        return R558_WriteNotepad(1, IDs_Table);
    }

    return SENS_ERROR;
}

// GenImg - Capture fingerprint image (no params / no toggles). The R558 returns codes:
// 0x00 success, other values indicate errors (e.g., no finger, movement, etc.)
SENS_StatusTypeDef R558::R558_CaptureFinger(void)
{
    uint8_t cmd[16];
    uint16_t cmd_len = R558_BuildCommand(cmd, R558_INS_GETIMG, NULL, 0);

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

// Img2Tz - Convert image to characteristic file (buffer 1 to 4)
SENS_StatusTypeDef R558::R558_Image2Tz(uint8_t buffer_id)
{
    if (buffer_id < 1 || buffer_id > 4)
    {
        DB_LOG("Invalid buffer id %d (must be from 1 to 4)", buffer_id);
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
            return SENS_NO_MATCH;
        }
        else
        {
            DB_LOG("Search failed (code=0x%02X)", resp[9]);
            return SENS_ERROR;
        }
    }
    return SENS_ERROR;
}

SENS_StatusTypeDef R558::R558_LoadChar(uint8_t buffer_id, uint16_t page_id)
{
    SENS_StatusTypeDef ret = SENS_ERROR;
    uint8_t cmd[16];
    uint16_t cmd_len = 0;
    uint8_t resp[32] = {0};
    uint8_t params[3]; // = {BufferID, PageID (high), PageID(low)}

    params[0] = buffer_id; // BufferID
    params[1] = (page_id >> 8) & 0XFF;
    params[2] = page_id & 0XFF;

    cmd_len = R558_BuildCommand(cmd, R558_INS_LOAD_CHAR, params, 3);

    DB_LOG("Loading template from flash...   (BufferID = %d - PageID = %d)", buffer_id, page_id);
    if (R558_SendCommand(cmd, cmd_len, resp, sizeof(resp)) == SENS_OK)
    {
        if (resp[9] == R558_CONFIRM_OK)
        {
            DB_LOG("Loading template from flash OK");
            ret = SENS_OK;
        }
        else
        {
            DB_LOG("Loading template from flash failed (code=0x%02X)", resp[9]);
            ret = SENS_ERROR;
        }
    }
    return ret;
}

/* Check whether the module works properly */
SENS_StatusTypeDef R558::R558_HandShake(void)
{
    uint8_t cmd[16];
    uint8_t resp[32] = {0};
    uint16_t cmd_len = R558_BuildCommand(cmd, R558_INS_HANDSHAKE, NULL, 0);

    FP_LOG("HandShake command...");

    if (R558_SendCommand(cmd, cmd_len, resp, sizeof(resp)) == SENS_OK)
    {
        if (resp[9] == R558_CONFIRM_OK)
        {
            FP_LOG("HandShake command OK. The module works properly.");
            return SENS_OK;
        }
        else
        {
            FP_LOG("HandShake command failed!");
            return SENS_ERROR;
        }
    }
    return SENS_ERROR;
}

/* Check whether the sensor works properly */
SENS_StatusTypeDef R558::R558_CheckSensor(void)
{
    uint8_t cmd[16];
    uint8_t resp[32] = {0};
    uint16_t cmd_len = R558_BuildCommand(cmd, R558_INS_CHECKSENSOR, NULL, 0);

    FP_LOG("CheckSensor command...");

    if (R558_SendCommand(cmd, cmd_len, resp, sizeof(resp)) == SENS_OK)
    {
        if (resp[9] == R558_CONFIRM_OK)
        {
            FP_LOG("CheckSensor command OK. The sensor works normally");
            return SENS_OK;
        }
        else
        {
            FP_LOG("CheckSensor command failed! Confirmation code: 0x%X", resp[9]);
            return SENS_ERROR;
        }
    }
    return SENS_ERROR;
}

/* Read the current valid template number of the module */
SENS_StatusTypeDef R558::R558_GetTemplateNum(uint16_t *out_temp_num)
{
    uint8_t cmd[16];
    uint8_t resp[32] = {0};
    uint16_t cmd_len = R558_BuildCommand(cmd, R558_INS_TEMPNUM, NULL, 0);

    FP_LOG("Reading current valid template number...");

    if (R558_SendCommand(cmd, cmd_len, resp, sizeof(resp)) == SENS_OK)
    {
        if (resp[9] == R558_CONFIRM_OK)
        {
            FP_LOG("Read template number OK.");
            *out_temp_num = ((uint16_t)resp[10] << 8) | resp[11];
            return SENS_OK;
        }
        else
        {
            FP_LOG("Read template number failed!");
            return SENS_ERROR;
        }
    }
    return SENS_ERROR;
}

// SENS_StatusTypeDef R558::R558_ManageLED()
SENS_StatusTypeDef R558::R558_ManageLED(const uint8_t *params)
{
    uint8_t cmd[16];
#if 0
#if 1
    uint8_t params[4] = {
        R558_FC_FLASH_LIGHT,       // R558_FC_NORMAL_BREATHING, // Function number
        R558_START_COLOR_GREEN_ON, // Starting color
        R558_END_COLOR_GREEN_ON,   // Ending color
        3                          // Cycle times
    };
#else
    uint8_t params[8] = {
        R558_FC_COLORFUL_PROGRAMMED_BREATHING, // Function number
        20,                                    // Time bit
        0xC9,                                  // Color1 H + L : valid+Red | valid+Blue
        0xA9,                                  // Color2 H + L : valid+green | valid+Blue
        0x90,                                  // Color3 H + L : valid+blue | No_valid
        0x00,                                  // Color4 H + L : No_valid | No_valid
        0x00,                                  // Color5 H + L : No_valid | No_valid
        1                                      // Cycle times
    };
#endif
#endif
    uint16_t cmd_len = R558_BuildCommand(cmd, R558_INS_LED_CONTROL, params, sizeof(params));

    uint8_t resp[32] = {0};
    DB_LOG("Setting LED control...");
    if (R558_SendCommand(cmd, cmd_len, resp, sizeof(resp)) == SENS_OK)
    {
        if (resp[9] == R558_CONFIRM_OK)
        {
            DB_LOG("LED control set!");
            return SENS_OK;
        }
        else
        {
            DB_LOG("LED control setting failed! Confirmation code: 0x%X", resp[9]);
            return SENS_ERROR;
        }
    }
    return SENS_ERROR;
}

SENS_StatusTypeDef R558::R558_Sleep()
{
    uint8_t cmd[16];
    uint8_t resp[32] = {0};

    uint16_t cmd_len = R558_BuildCommand(cmd, R558_INS_SLEEP, NULL, 0);

    DB_LOG("Sending sleep command...");
    if (R558_SendCommand(cmd, cmd_len, resp, sizeof(resp)) == SENS_OK)
    {
        if (resp[9] == R558_CONFIRM_OK)
        {
            DB_LOG("Sending sleep command Confirmed!");
            return SENS_OK;
        }
        else
        {
            DB_LOG("Sending sleep command failed! Confirmation code: 0x%X", resp[9]);
            return SENS_ERROR;
        }
    }
    return SENS_ERROR;
}

/* Write module registers */
SENS_StatusTypeDef R558::R558_WriteReg()
{
    uint8_t cmd[16];
    uint8_t params[2] = {
        0x11, // Register number
        0x01  // Contents
    };

    uint16_t cmd_len = R558_BuildCommand(cmd, R558_INS_WRITE_REG, params, sizeof(params));

    uint8_t resp[32] = {0};
    DB_LOG("Setting System Register...");
    if (R558_SendCommand(cmd, cmd_len, resp, sizeof(resp)) == SENS_OK)
    {
        if (resp[9] == R558_CONFIRM_OK)
        {
            DB_LOG("Setting System Register OK!");
            return SENS_OK;
        }
        else
        {
            DB_LOG("Setting System Register failed! Confirmation code: 0x%X", resp[9]);
            return SENS_ERROR;
        }
    }
    return SENS_ERROR;
}

/* Read Basic parameter list (16bytes) */
SENS_StatusTypeDef R558::R558_ReadSystemParameters(void)
{
    uint8_t cmd[16];
    uint16_t cmd_len = R558_BuildCommand(cmd, R558_INS_READ_SYS_PARA, NULL, 0);

    uint8_t resp[32] = {0};
    FP_LOG("Reading System parameters...");
    if (R558_SendCommand(cmd, cmd_len, resp, sizeof(resp)) == SENS_OK)
    {
        if (resp[9] == R558_CONFIRM_OK)
        {
            FP_LOG("Read complete!");

            uint8_t parametersArray[R558_SYSTEM_PARAM_SIZE];
            memcpy(&parametersArray[0], &resp[10], 16);

            SystemParameters.EnrollTimes = TO_UINT16_BE(&parametersArray[0]);
            SystemParameters.FP_Template_Size = TO_UINT16_BE(&parametersArray[2]);
            SystemParameters.FP_Database_Size = TO_UINT16_BE(&parametersArray[4]);
            SystemParameters.ScoreLevel = TO_UINT16_BE(&parametersArray[6]);
            SystemParameters.DeviceAddress = TO_UINT32_BE(&parametersArray[8]);
            SystemParameters.DataPacketSize = TO_UINT16_BE(&parametersArray[12]);
            SystemParameters.BaudSettings = TO_UINT16_BE(&parametersArray[14]);

            return SENS_OK;
        }
        else
        {
            FP_LOG("Reading error when receiving package");
            return SENS_ERROR;
        }
    }
    return SENS_ERROR;
}

/* Shows the system parameters of the module */
SENS_StatusTypeDef R558::R558_ShowSystemParameters(void)
{
    // to be review....

    // to call R558_ReadSystemParameters if it is private

    FP_LOG("Show System Parameters: ");

    std::cout << SystemParametersName[0] << SystemParameters.EnrollTimes << std::endl;
    std::cout << SystemParametersName[1] << SystemParameters.FP_Template_Size << std::endl;
    std::cout << SystemParametersName[2] << SystemParameters.FP_Database_Size << std::endl;
    std::cout << SystemParametersName[3] << SystemParameters.ScoreLevel << std::endl;
    std::cout << SystemParametersName[4] << " 0x" << std::hex << std::uppercase
              << std::setw(2) << std::setfill('0')
              << SystemParameters.DeviceAddress << std::endl;
    std::cout << std::dec;
    std::cout << SystemParametersName[5] << SystemParameters.DataPacketSize << std::endl;
    std::cout << SystemParametersName[6] << SystemParameters.BaudSettings << std::endl;

    return SENS_OK;
}

/* Read informaton page (512bytes) */
SENS_StatusTypeDef R558::R558_ReadInformationPage(uint8_t *infOut, uint16_t infOutLen, uint16_t *totalLen)
{
    uint8_t cmd[16];
    uint16_t cmd_len = R558_BuildCommand(cmd, R558_INS_READ_INF_PAGE, NULL, 0);

    uint8_t resp[32] = {0};
    FP_LOG("Reading information page...");
    if (R558_SendCommand(cmd, cmd_len, resp, sizeof(resp)) == SENS_OK)
    {
        if (resp[9] == R558_CONFIRM_OK)
        {
            FP_LOG("Reading information page OK! Receiving next data...");

            return ReceivePackets(infOut, infOutLen, totalLen);
        }
        else
        {
            FP_LOG("Reading information page failed!");
            return SENS_ERROR;
        }
    }
    return SENS_ERROR;
}

/* Upload the image in Img_Buffer to upper computer */
SENS_StatusTypeDef R558::R558_UploadImage(uint16_t page_id, uint8_t *infOut, uint16_t infOutLen, uint16_t *totalLen)
{
    uint8_t cmd[16];
    uint8_t resp[32] = {0};

    uint16_t cmd_len = R558_BuildCommand(cmd, R558_INS_UPIMAGE, NULL, 0);

    FP_LOG("Sending UpImage command...");
    if (R558_SendCommand(cmd, cmd_len, resp, sizeof(resp)) == SENS_OK)
    {
        if (resp[9] == R558_CONFIRM_OK)
        {
            FP_LOG("UpImage command sent OK!");
            return ReceivePackets(infOut, infOutLen, totalLen);
        }
        else
        {
            FP_LOG("Sending UpImage command failed!");
            return SENS_ERROR;
        }
    }

    return SENS_ERROR;
}

/* Write user's data into FLASH */
SENS_StatusTypeDef R558::R558_WriteNotepad(uint8_t page_id, uint8_t *userData)
{
    if (userData == nullptr)
    {
        DB_LOG("userData is NULL!");
        return SENS_ERROR;
    }

    if (page_id > R558_PAGE_NO_MAX)
    {
        DB_LOG("page_id out of range");
        return SENS_ERROR;
    }

    uint8_t cmd[50];
    uint8_t params[33];               // {pageID + userData(32byte)}
    params[0] = page_id;              // Page number
    memcpy(&params[1], userData, 32); // User information

    uint16_t cmd_len = R558_BuildCommand(cmd, R558_INS_WRITE_NOTEPD, params, sizeof(params));

    uint8_t resp[32] = {0};
    DB_LOG("Writing notepad...");
    if (R558_SendCommand(cmd, cmd_len, resp, sizeof(resp)) == SENS_OK)
    {
        if (resp[9] == R558_CONFIRM_OK)
        {
            DB_LOG("Writing notepad OK!");
            return SENS_OK;
        }
        else
        {
            DB_LOG("Writing notepad failed! Confirmation code: 0x%X", resp[9]);
            return SENS_ERROR;
        }
    }
    return SENS_ERROR;
}

/* Read user's data into FLASH */
SENS_StatusTypeDef R558::R558_ReadNotepad(uint8_t page_id, uint8_t *userData)
{
    uint8_t cmd[50];
    uint8_t params[1] = {page_id}; // Page number

    uint16_t cmd_len = R558_BuildCommand(cmd, R558_INS_READ_NOTEPD, params, sizeof(params));

    uint8_t resp[50] = {0};
    DB_LOG("Reading notepad...");
    if (R558_SendCommand(cmd, cmd_len, resp, sizeof(resp)) == SENS_OK)
    {
        if (resp[9] == R558_CONFIRM_OK)
        {
            DB_LOG("Reading notepad OK! ");
            /* To be decided what to do... */
            if (userData == nullptr)
            {
                printArray("Notepad contents:", &resp[10], 32);
            }
            else
            {
                memcpy(userData, &resp[10], 32);
            }
            return SENS_OK;
        }
        else
        {
            DB_LOG("Reading notepad failed! Confirmation code: 0x%X", resp[9]);
            return SENS_ERROR;
        }
    }
    return SENS_ERROR;
    ;
}
///////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////    HELPER FUNCTIONS   ////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////

static void printArray(const char *name, uint8_t *buf, uint16_t bufLen)
{
    using namespace std;
    {
        std::stringstream ss;
        string s(name);

        s.append("(len=" + to_string(bufLen) + ")");

        for (int i = 0; i < bufLen; i++)
            ss << " 0x" << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << static_cast<int>(buf[i]);

        s.append(" " + ss.str());

        DB_LOG("%s", s.c_str());
    } // namespace std
}

static uint32_t getTick()
{
    using namespace std::chrono;
    return static_cast<uint32_t>(duration_cast<milliseconds>(
                                     steady_clock::now().time_since_epoch())
                                     .count());
}
