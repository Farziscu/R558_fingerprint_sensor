
#include <iostream>
#include <string.h>
#include <windows.h>

#include "FingerPrintSensor.h"

int main(void)
{
    std::cout << "Hello World!!!!!!!!!!" << std::endl;

    // R558 mySensor; // test error: (SER_P_COM2, SER_P_BAUDRATE_9600);
    R558 mySensor(SER_P_COM10, SER_P_BAUDRATE_57600);

    // mySensor.SendHello();
    // mySensor.GetHello();

#if 1 // verify password
    // while (1)
    {

        if (mySensor.R558_VerifyPassword(0x00000000) != SENS_OK)
        // if (mySensor.R558_VerifyPassword(0x01233210) != SENS_OK)
        {
            FP_LOG("Password verify failed!");
        }
        else
        {
            FP_LOG("Password verify OK OK OK OK OK OK OK OK OK OK OK OK OK OK OK OK OK OK OK OK OK OK OK OK !");
        }
    }
#endif

    FP_LOG("\n\n");

#if 0
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

#if 0 // enroll
    mySensor.R558_Enroll(1);
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

    return 0;
}