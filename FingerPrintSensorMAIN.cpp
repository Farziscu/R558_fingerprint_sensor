
#include <iostream>
#include <string.h>
#include <windows.h>
#include <sstream>
#include <iomanip>

#include "FingerPrintSensor.h"

#define MAIN_LOG(fmt, ...) printf("[MAIN] " fmt "\n", ##__VA_ARGS__)

void printBuffer(uint8_t *buf, uint16_t bufLen)
{
    using namespace std;
    {
        std::stringstream ss;
        string s("buffer : ");
        for (int i = 0; i < bufLen; i++)
            ss << " 0x" << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << static_cast<int>(buf[i]);

        s.append(" " + ss.str());

        MAIN_LOG("%s", s.c_str());
    } // namespace std
}

void manageLED_Colour()
{
    // yellow: ready
    // Flashing (blue|green): reading finger
    // Red: Fail verify | after 3sec goes to ready
    // Green: OK verify | after 3sec goes to ready
}

#if 0
using namespace std;
int main(void)
{
    char choice;
    do
    {
        std::cout << "Inserisci un carattere: " << std::endl;
        std::cout << " P-Check password;" << std::endl;
        std::cout << " S-Sleep;" << std::endl;
        std::cout << " L-LED; " << std::endl;
        std::cout << " V-Verify; " << std::endl;
        std::cout << " E-Exit" << std::endl;
        std::cin >> choice;
        std::cout << "Hai inserito: " << choice << std::endl;

        switch (choice)
        {
        case 'P':
        case 'p':
            std::cout << "SCELTA P " << std::endl;
            break;

        case 'S':
        case 's':
            std::cout << "SCELTA S " << std::endl;
            break;

        case 'E':
        case 'e':
            std::cout << "SCELTA E - Uscita " << std::endl;
            break;
        default:
            break;
        }
    } while ((choice != 'E') || (choice != 'e'));

    return 0;
}

#else
int main(void)
{
    std::cout << "Testing R558 fingerprint sensor." << std::endl;
    std::cout << "Serial port: COM10" << std::endl;
    std::cout << "Baudrate: 57600" << std::endl;
    std::cout << std::endl;
    R558 mySensor(SER_P_COM10, SER_P_BAUDRATE_57600);

    if (mySensor.R558_isConnected() != SENS_OK)
    {
        return 0;
    }

    char choice;
    do
    {
        std::cout << "   Inserisci un carattere: " << std::endl;
        std::cout << "      P-Check password;" << std::endl;
        std::cout << "      V-Verify; " << std::endl;
        std::cout << "      Y-sYstem parameters;" << std::endl;
        std::cout << "      R-Read information Page;" << std::endl;
        std::cout << "      C-Check sensor and module;" << std::endl;
        std::cout << "      T-Read Template number; " << std::endl;
        std::cout << "      N-Write Notepad; " << std::endl;
        std::cout << std::endl;
        std::cout << "      S-Sleep;" << std::endl;
        std::cout << "      W-Write Reg (only for led now);" << std::endl;
        std::cout << "      L-LED; " << std::endl;
        std::cout << "      E-Enroll; " << std::endl;
        std::cout << "      A-Accurate Match; " << std::endl;
        std::cout << "      U-Upload Image to Upper Computer; " << std::endl;
        std::cout << std::endl;
        std::cout << "      D-Delete template; " << std::endl;
        std::cout << "      F-delete All fingerprints; " << std::endl;
        std::cout << "      X-Exit" << std::endl;
        std::cin >> choice;

        MAIN_LOG("==================================================================");

        switch (choice)
        {
        case 'P': // verify password
        case 'p':
        {
            MAIN_LOG("Verify password test");

            if (mySensor.R558_VerifyPassword(0x00000000) != SENS_OK)
            {
                MAIN_LOG("Verify password failed!");
            }
            else
            {
                MAIN_LOG("Verify password OK!");
            }
            break;
        }

        case 'Y': // System parameters
        case 'y':
        {
            MAIN_LOG("Read System parameters");

            if (mySensor.R558_ReadSystemParameters() != SENS_OK)
            {
                MAIN_LOG("Read System parameters failed!");
            }
            else
            {
                MAIN_LOG("Read System parameters OK!");
            }

            mySensor.R558_ShowSystemParameters();
            break;
        }

        case 'R': // Read information Page
        case 'r':
        {

            MAIN_LOG("Read information page test");
            uint8_t inf[1024];
            uint16_t retSize = 0;

            if (mySensor.R558_ReadInformationPage(inf, sizeof(inf), &retSize) != SENS_OK)
            {
                MAIN_LOG("Read information page test failed!");
            }
            else
            {
                MAIN_LOG("Read information page test OK!");
                printBuffer(inf, retSize);
            }
            break;
        }

        case 'C': // Check sensor and module
        case 'c':
        {
            MAIN_LOG("Check sensor and module test");

            if (mySensor.R558_CheckSensor() != SENS_OK)
            {
                MAIN_LOG("Check sensor test failed!");
            }
            else
            {
                MAIN_LOG("Check sensor test OK!");
            }

            if (mySensor.R558_HandShake() != SENS_OK)
            {
                MAIN_LOG("Check module test failed!");
            }
            else
            {
                MAIN_LOG("Check module test OK!");
            }
            break;
        }

        case 'L': // LED CONTROL
        case 'l':
        {
            MAIN_LOG("LED Control test");

            mySensor.R558_ManageLED(LED_ReadyStateParams);

            Sleep(5000);

            if (mySensor.R558_ManageLED(LED_ReadingFingerParams) != SENS_OK)
            {
                MAIN_LOG("LED Control failed!");
            }
            else
            {
                MAIN_LOG("LED Control OK!");
            }

            Sleep(5000);

            mySensor.R558_ManageLED(LED_FailVerifyParams);

            break;
        }

        case 'V': // verify finger
        case 'v':
        {
            MAIN_LOG("Verify finger test");

            uint16_t out_page_id;
            uint16_t out_score;
            SENS_StatusTypeDef status = mySensor.R558_Verify(&out_page_id, &out_score);
            if (status == SENS_OK)
            {
                MAIN_LOG("Verify finger test OK!");
                MAIN_LOG("out_page_id: %d ", out_page_id);
                MAIN_LOG("out_score:   %d ", out_score);
            }
            else if (status == SENS_NO_MATCH)
            {
                MAIN_LOG("No match for this finger!");
            }
            else
            {
                MAIN_LOG("Verify finger test failed!");
            }
            break;
        }

        case 'A': // Accurate Match
        case 'a':
        {
            MAIN_LOG("Accurate Match test");

            uint16_t out_page_id;
            uint16_t out_score;

            if (mySensor.R558_Verify(&out_page_id, &out_score) != SENS_OK)
            {
                MAIN_LOG("Accurate Match test failed!");
            }
            else
            {
                MAIN_LOG("Accurate Match test OK!");
                MAIN_LOG("out_page_id: %d ", out_page_id);
                MAIN_LOG("out_score:   %d ", out_score);

                MAIN_LOG("Next step...");
                out_score = 999;

                if (mySensor.R558_CheckAccuracy(&out_score, out_page_id) != SENS_OK)
                {
                    std::cout << "Accuracy match test FAILED!" << std::endl;
                }
                else
                {
                    std::cout << "Accuracy match test OK!" << std::endl;
                    std::cout << "out_score:    " << out_score << std::endl;
                }
            }
            break;
        }

        case 'S': // Sleep test
        case 's':
        {
            MAIN_LOG("Sleep test");

            if (mySensor.R558_Sleep() != SENS_OK)
            {
                MAIN_LOG("Sleep test failed!");
            }
            else
            {
                MAIN_LOG("Sleep test OK!");
                mySensor.R558_ManageLED(LED_SleepStateParams);
            }
            break;
        }

        case 'W': // Write Reg
        case 'w':
        {
            MAIN_LOG("Write Reg test");

            if (mySensor.R558_WriteReg() != SENS_OK)
            {
                MAIN_LOG("Write Reg test failed!");
            }
            else
            {
                MAIN_LOG("Write Reg test OK!");
            }
            break;
        }

        case 'E': // enroll
        case 'e':
        {
            // mySensor.R558_Enroll(4);

            MAIN_LOG("Enroll test");

            if (mySensor.R558_EnrollNextIndex() != SENS_OK)
            {
                MAIN_LOG("Enroll test failed!");
            }
            else
            {
                MAIN_LOG("Enroll test OK!");
            }
            break;
        }

        case 'T': // get template number
        case 't':
        {
            MAIN_LOG("Get Template test");

            uint16_t temp_numb = 1000;

            if (mySensor.R558_GetTemplateNum(&temp_numb) != SENS_OK)
            {
                MAIN_LOG("Get Template test failed!");
            }
            else
            {
                MAIN_LOG("Get Template test OK!   Template numb = %d", temp_numb);
            }
            break;
        }

        case 'N': // Write Notepad or read.... change for test
        case 'n':
        {
            uint8_t pageID = 1;
            uint8_t UserInfo[32] = {0x00};
            memset(UserInfo, 0xFF, sizeof(UserInfo));
            // UserInfo[0] = 0xFF; //{1111 1111 }
            UserInfo[1] = 0xDF;  //{1101 1111}  //pos 11
            UserInfo[2] = 0xBE;  //{1011 1110}  //pos 18 + pos 24
            UserInfo[12] = 0xE0; //{1110 1111}  //pos 100
#if 0
            MAIN_LOG("Write Notepad test");

            if (mySensor.R558_WriteNotepad(pageID, UserInfo) != SENS_OK)
            {
                MAIN_LOG("Write Notepad test failed!");
            }
            else
            {
                MAIN_LOG("Write Notepad test OK!");
            }
#endif
            MAIN_LOG("Read Notepad test");
            memset(UserInfo, 0x11, sizeof(UserInfo));

            if (mySensor.R558_ReadNotepad(pageID, UserInfo) != SENS_OK)
            {
                MAIN_LOG("Read Notepad test failed!");
            }
            else
            {
                MAIN_LOG("Read Notepad test OK!");
                printBuffer(UserInfo, 13);
            }
#if 0
            uint8_t idx = 200;
            if (mySensor.FindNextIndex(&idx) != SENS_OK)
            {
                MAIN_LOG("CalculateNexIndex fails!");
            }
            else
            {
                MAIN_LOG("CalculateNexIndex OK!   idx == %d", idx);
            }

            if (mySensor.UpdateNextIndex(&idx) != SENS_OK)
            {
                MAIN_LOG("UpdateNextIndex fails!");
            }
            else
            {
                MAIN_LOG("UpdateNextIndex OK!");
            }
#endif
            break;
        }

        case 'U': // Upload Image to Upper Computer
        case 'u':
        {
            uint8_t img[16384] = {0};
            uint8_t pageId = 1;  // index of the fingerprint in flash
            uint16_t imgLen = 0; // index of the fingerprint in flash

            MAIN_LOG("Upload Image to Upper Computer test");

            std::cout << " select no. (1, 2, 3, 4) " << std::endl;
            std::cin >> choice;
            if (choice >= '0' && choice <= '9')
            {
                pageId = choice - '0'; // converte '5' in 5
                std::cout << "pageId = " << pageId << std::endl;
            }
            else
            {
                std::cout << "Non hai inserito una cifra!  default pageId = 1" << std::endl;
            }

            if (mySensor.R558_UploadImage(pageId, img, sizeof(img), &imgLen) != SENS_OK)
            {
                MAIN_LOG("Upload Image to Upper Computer test FAILED!");
            }
            else
            {
                MAIN_LOG("Upload Image to Upper Computer test OK");
                MAIN_LOG("Image size == %d", imgLen);
                // printBuffer(img, imgLen);
                printBuffer(img, 100);
            }
            break;
        }

        case 'D': // Delete template
        case 'd':
        {
            uint8_t fpIndex = 2; // index of the fingerprint in flash

            MAIN_LOG("Delete template test");

            if (mySensor.R558_DeleteFingerprints(fpIndex, 2) != SENS_OK)
            {
                MAIN_LOG("Delete template test FAILED!");
            }
            else
            {
                MAIN_LOG("Delete template test OK");
            }
            break;
        }

        case 'F': // Delete all fingeprint
        case 'f':
        {
            uint8_t pageId = 3; // index of the fingerprint in flash

            MAIN_LOG("Delete all fingeprint test");

            if (mySensor.R558_ClearAllFingerprints() != SENS_OK)
            {
                MAIN_LOG("Delete all fingeprint test FAILED!");
            }
            else
            {
                MAIN_LOG("Delete all fingeprint test OK");
            }
            break;
        }

        default:
            break;
        }
        std::cout << std::endl;
    } while ((choice != 'X') && (choice != 'x'));

#if 0 // test accuracy
    MAIN_LOG("==================================================================");
    MAIN_LOG("Accuracy match test");
    uint16_t out_score;

    if (mySensor.R558_CheckAccuracy(&out_score) != SENS_OK)
    {
        std::cout << "Accuracy match test FAILED!" << std::endl;
    }
    else
    {
        std::cout << "Accuracy match test OK!" << std::endl;
        std::cout << "out_score:    " << out_score << std::endl;
    }
#endif

#if 0 // Read information page
    MAIN_LOG("==================================================================");
    MAIN_LOG("Read information page test");
    uint8_t inf[1024];

    if (mySensor.R558_ReadInformationPage(inf) != SENS_OK)
    {
        std::cout << "Read information page test FAILED!" << std::endl;
    }
    else
    {
        std::cout << "Read information page test OK!" << std::endl;

        using namespace std;
        {
            std::stringstream ss;
            string s("inf buffer : ");
            for (int i = 0; i < 512; i++)
                ss << " 0x" << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << static_cast<int>(inf[i]);

            s.append(" " + ss.str());

            MAIN_LOG("%s", s.c_str());
        } // namespace std
    }
#endif

#if 0 // Sleep test
    MAIN_LOG("==================================================================");
    MAIN_LOG("Sleep test test");

    if (mySensor.R558_Sleep() != SENS_OK)
    {
        std::cout << "Sleep test FAILED!" << std::endl;
    }
    else
    {
        std::cout << "Sleep test OK!" << std::endl;
    }
#endif

    MAIN_LOG("==================================================================");
    return 0;
}
#endif

#if 0 
using namespace std;
int main(void)
{
    string s;
    uint8_t val = 0x55;
    s.append("ciaone! ");
    // s.push_back(val);
    s.append(" 0x" + to_string(val));
    cout << "hello world! " << s << endl;

    std::stringstream ss;
    ss << "0x" << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << static_cast<int>(val);
    s.append(" " + ss.str());

    MAIN_LOG("%s", s.c_str());

    return 0;
}
#endif
