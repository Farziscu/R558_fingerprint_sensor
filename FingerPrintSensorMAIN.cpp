
#include <iostream>
#include <string.h>
#include <windows.h>

#include "FingerPrintSensor.h"

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

#if 1 // verify password

    if (mySensor.R558_VerifyPassword(0x00000000) != SENS_OK)
    {
        FP_LOG("Verify password failed!");
    }
    else
    {
        FP_LOG("Verify password OK!");
    }

#endif

#if 1 // LED CONTROL

    if (mySensor.R558_ManageLED() != SENS_OK)
    {
        FP_LOG("LED Control failed!");
    }
    else
    {
        FP_LOG("LED Control OK!");
    }

    Sleep(2000);

#endif

#if 0 // enroll
    mySensor.R558_Enroll(1);
#endif

#if 0 // get template number
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
#endif

#if 0 // verify finger

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
#endif

    // to be removed
    //  mySensor.SendHello();
    //  mySensor.GetHello();
    return 0;
}