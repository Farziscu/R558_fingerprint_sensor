
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

#if 1 // verify password
    FP_LOG("==================================================================");
    FP_LOG("Verify password test");

    if (mySensor.R558_VerifyPassword(0x00000000) != SENS_OK)
    {
        FP_LOG("Verify password failed!");
    }
    else
    {
        FP_LOG("Verify password OK!");
    }

#endif

#if 1 // System parameters
    FP_LOG("==================================================================");
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

#endif

#if 1 // LED CONTROL
    FP_LOG("==================================================================");
    FP_LOG("LED Control test");

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

#if 0 // get template number
    FP_LOG("==================================================================");
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
#endif

#if 0 // enroll
    FP_LOG("==================================================================");
    FP_LOG("Enroll test");
    mySensor.R558_Enroll(2);
#endif

#if 0 // get template number
    FP_LOG("==================================================================");
    FP_LOG("Get Template test");

    temp_numb = 1000;

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

#if 1 // verify finger
    FP_LOG("==================================================================");
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
#endif

    // to be removed
    //  mySensor.SendHello();
    //  mySensor.GetHello();
    FP_LOG("==================================================================");
    return 0;
}
#endif