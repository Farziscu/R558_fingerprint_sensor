
#include <iostream>
#include <string.h>
#include <windows.h>
#include <sstream>
#include <iomanip>
// #include <string>

#include "FingerPrintSensor.h"

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

    if (mySensor.isSensorConnected() != SENS_OK)
    {
        return 0;
    }

    char choice;
    do
    {
        std::cout << "   Inserisci un carattere: " << std::endl;
        std::cout << "      P-Check password;" << std::endl;
        std::cout << "      Y-sYstem parameters;" << std::endl;
        std::cout << "      S-Sleep;" << std::endl;
        std::cout << "      L-LED; " << std::endl;
        std::cout << "      E-Enroll; " << std::endl;
        std::cout << "      V-Verify; " << std::endl;
        std::cout << "      A-Accurate Match; " << std::endl;
        std::cout << "      X-Exit" << std::endl;
        std::cin >> choice;

        FP_LOG("==================================================================");

        switch (choice)
        {
        case 'P': // verify password
        case 'p':
        {
            FP_LOG("Verify password test");

            if (mySensor.R558_VerifyPassword(0x00000000) != SENS_OK)
            {
                FP_LOG("Verify password failed!");
            }
            else
            {
                FP_LOG("Verify password OK!");
            }
            break;
        }

        case 'Y': // System parameters
        case 'y':
        {
            FP_LOG("Read System parameters");

            if (mySensor.R558_ReadSystemParameters() != SENS_OK)
            {
                FP_LOG("Read System parameters failed!");
            }
            else
            {
                FP_LOG("Read System parameters OK!");
            }

            mySensor.R558_ShowSystemParameters();
            break;
        }

        case 'L': // LED CONTROL
        case 'l':
        {
            FP_LOG("LED Control test");

            if (mySensor.R558_ManageLED() != SENS_OK)
            {
                FP_LOG("LED Control failed!");
            }
            else
            {
                FP_LOG("LED Control OK!");
            }

            break;
        }

        case 'V': // verify finger
        case 'v':
        {
            FP_LOG("Verify finger test");

            uint16_t out_page_id;
            uint16_t out_score;

            if (mySensor.R558_Verify(&out_page_id, &out_score) != SENS_OK)
            {
                std::cout << "Error verifying!" << std::endl;
            }
            else
            {
                std::cout << "Verification OK!" << std::endl;
                std::cout << "out_page_id:  " << out_page_id << std::endl;
                std::cout << "out_score:    " << out_score << std::endl;
            }
            break;
        }

        case 'A': // Accurate Match
        case 'a':
        {
            FP_LOG("Accurate Match test");

            uint16_t out_page_id;
            uint16_t out_score;

            if (mySensor.R558_Verify(&out_page_id, &out_score) != SENS_OK)
            {
                std::cout << "Error verifying!" << std::endl;
            }
            else
            {
                std::cout << "Verification OK!" << std::endl;
                std::cout << "out_page_id:  " << out_page_id << std::endl;
                std::cout << "out_score:    " << out_score << std::endl;

                FP_LOG("Next step...");
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
            FP_LOG("Sleep test test");

            if (mySensor.R558_Sleep() != SENS_OK)
            {
                std::cout << "Sleep test FAILED!" << std::endl;
            }
            else
            {
                std::cout << "Sleep test OK!" << std::endl;
            }
            break;
        }

        case 'R': // enroll
        case 'r':
        {
            FP_LOG("Enroll test");
            mySensor.R558_Enroll(4);
            break;
        }

        case 'T': // get template number
        case 't':
        {
            FP_LOG("Get Template test");

            uint16_t temp_numb = 1000;

            FP_LOG("Getting Template numb....");

            if (mySensor.R558_GetTemplateNum(&temp_numb) != SENS_OK)
            {
                FP_LOG("Error getting template number!");
            }
            else
            {
                FP_LOG("Template numb = %d", temp_numb);
            }
            break;
        }

        default:
            break;
        }
        std::cout << std::endl;
    } while ((choice != 'X') && (choice != 'x'));

#if 0 // test accuracy
    FP_LOG("==================================================================");
    FP_LOG("Accuracy match test");
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
    FP_LOG("==================================================================");
    FP_LOG("Read information page test");
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

            FP_LOG("%s", s.c_str());
        } // namespace std
    }
#endif

#if 0 // Sleep test
    FP_LOG("==================================================================");
    FP_LOG("Sleep test test");

    if (mySensor.R558_Sleep() != SENS_OK)
    {
        std::cout << "Sleep test FAILED!" << std::endl;
    }
    else
    {
        std::cout << "Sleep test OK!" << std::endl;
    }
#endif

    // to be removed
    //  mySensor.SendHello();
    //  mySensor.GetHello();
    FP_LOG("==================================================================");
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

    FP_LOG("%s", s.c_str());

    return 0;
}
#endif
