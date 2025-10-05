"""Test Class sensor"""

import serial
import time
import struct
import keyboard
from serial.serialutil import SerialException

finger_on_sensor = False
simulation_on = True


Resp_HDR        = bytes([0xEF, 0x01])
Resp_ADDR       = bytes([0xFF, 0xFF, 0xFF, 0xFF])
Resp_PckID      = bytes([0x07])

Resp_PckLen     = bytes([0x00, 0x03])
Resp_ConfCode   = bytes([0x00])   #00 = OK;  01 = error when receiving package;  13 = wrong password;
calcSum = sum(Resp_PckID + Resp_PckLen + Resp_ConfCode)
Resp_CeckSum    = bytes([(calcSum >> 8) & 0xFF, calcSum & 0xFF])
Resp_psw_check = struct.pack('>2s4s1s2s1s2s', Resp_HDR, Resp_ADDR, Resp_PckID, Resp_PckLen, Resp_ConfCode, Resp_CeckSum)
Resp_finger_present = Resp_psw_check
Resp_img_convert_OK = Resp_psw_check
Resp_merge_buffer   = Resp_psw_check
Resp_store_template = Resp_psw_check


Resp_PckLen     = bytes([0x00, 0x03])
Resp_ConfCode   = bytes([0x02])
calcSum = sum(Resp_PckID + Resp_PckLen + Resp_ConfCode)
Resp_CeckSum    = bytes([(calcSum >> 8) & 0xFF, calcSum & 0xFF])
Resp_finger_removed = struct.pack('>2s4s1s2s1s2s', Resp_HDR, Resp_ADDR, Resp_PckID, Resp_PckLen, Resp_ConfCode, Resp_CeckSum)


Resp_PckLen     = bytes([0x00, 0x07])
Resp_ConfCode   = bytes([0x00])   #00 = OK;  01 = error when receiving package;  13 = wrong password;
Resp_PageID     = bytes([0x0A, 0x07])
Resp_MatchScore = bytes([0x99, 0x88])
calcSum = sum(Resp_PckID + Resp_PckLen + Resp_ConfCode + Resp_PageID + Resp_MatchScore)
Resp_CeckSum    = bytes([(calcSum >> 8) & 0xFF, calcSum & 0xFF])
Resp_Search = struct.pack('>2s4s1s2s1s2s2s2s', Resp_HDR, Resp_ADDR, Resp_PckID, Resp_PckLen, Resp_ConfCode, Resp_PageID, Resp_MatchScore, Resp_CeckSum)

Resp = Resp_psw_check



def verify_pwd():
    """Verify password"""
    print('Reply: ' + ' '.join(f'{byte:02X}' for byte in Resp_psw_check) )
    ser.write(Resp_psw_check)

def wait_finger_presence():
    """wait placement"""
    if finger_on_sensor:
        print('Reply PRESENCE OK: ' + ' '.join(f'{byte:02X}' for byte in Resp_finger_present) )
        ser.write(Resp_finger_present)
    else:
        print('Reply PRESENCE NOT OK: ' + ' '.join(f'{byte:02X}' for byte in Resp_finger_removed) )
        ser.write(Resp_finger_removed)

def reply_img_convert():
    """img_2_tz"""
    print('Reply img convert result: ' + ' '.join(f'{byte:02X}' for byte in Resp_img_convert_OK) )
    ser.write(Resp_img_convert_OK)

def reply_search_result():
    """R558_INS_SEARCH"""
    print('Reply with search result: ' + ' '.join(f'{byte:02X}' for byte in Resp_Search) )
    ser.write(Resp_Search)

def reply_merge_buffer():
    """R558_INS_REGMODEL"""
    print('Reply for merge buffer -> template: ' + ' '.join(f'{byte:02X}' for byte in Resp_merge_buffer) )
    ser.write(Resp_merge_buffer)

def reply_store_template():
    """R558_INS_STORE"""
    print('Reply for store template: ' + ' '.join(f'{byte:02X}' for byte in Resp_store_template) )
    ser.write(Resp_store_template)

def set_finger_presence():
    """set finger presence"""
    print("set_finger_presence.....")
    global finger_on_sensor
    finger_on_sensor = True

def reset_finger_presence():
    """reset finger presence"""
    print("reset_finger_presence.....")
    global finger_on_sensor
    finger_on_sensor = False

def close_simulation():
    """close_simulation"""
    print("closing simulation.....")
    global simulation_on
    simulation_on = False


keyboard.add_hotkey('P', set_finger_presence)
keyboard.add_hotkey('N', reset_finger_presence)
keyboard.add_hotkey('esc', close_simulation)

ser = serial.Serial(port="COM5", baudrate=57600, timeout=1)

try:
    while simulation_on:
        print('\nwaiting commands from host.......................................................')
        ser.flush()

        while 1:
            msg = ser.read(9)
            if msg:
                break
            if not simulation_on:
                break

        if simulation_on:
            #get last part.....
            remaining = sum(msg[8:])
            #print(remaining)
            while 1:
                tail = ser.read(remaining)
                if tail:
                    break

            msg = msg + tail
            print('Command received: ' + ' '.join(f'{byte:02X}' for byte in msg) )

            # Instruction code
            instructionCode = msg[9]
            #print(instructionCode)

            match instructionCode:
                case 0x13:  #R558_INS_VERIFYPWD
                    print("R558_INS_VERIFYPWD received!")
                    verify_pwd()

                case 0x01:  #R558_INS_GENIMG
                    print("R558_INS_GENIMG received! ??????????Metti o Rimuovi il dito????????????")
                    wait_finger_presence()

                case 0x02:  #R558_INS_IMG2TZ
                    print("R558_INS_IMG2TZ received!")
                    reply_img_convert()

                case 0x04:  #R558_INS_SEARCH
                    print("R558_INS_SEARCH received!")
                    reply_search_result()

                case 0x05:  #R558_INS_REGMODEL
                    print("R558_INS_REGMODEL received!")
                    reply_merge_buffer()

                case 0x06:  #R558_INS_STORE
                    print("R558_INS_STORE received!")
                    reply_store_template()

                case 0x0D:  #R558_INS_EMPTY
                    print("R558_INS_EMPTY received!")

            print('-----------------------------------------------------------------------------\n')

except SerialException as e:
    print("UART Read on COM5 failed:", e)

ser.close()
