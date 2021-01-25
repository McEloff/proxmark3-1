//-----------------------------------------------------------------------------
// Mikhail Elov, 2019
//
// This code is licensed to you under the terms of the GNU GPL, version 2 or,
// at your option, any later version. See the LICENSE.txt file for the text of
// the license.
//-----------------------------------------------------------------------------
// main code for HF Mifare, HF Ultralight, HF EV1/NTAG
//-----------------------------------------------------------------------------
#include "hf_eloff.h"

uint32_t eloff_lf_bits = 0;

void StandaloneReplyStatus(void) {
    if (eloff_lf_bits != 0) {
        reply_ng(CMD_GET_STANDALONE_DONE_STATUS, STANDALONE_ELOFF_LF_SUCCESS, (uint8_t *) &eloff_lf_bits, sizeof(eloff_lf_bits));
    } else {
        reply_ng(CMD_GET_STANDALONE_DONE_STATUS, PM3_EUNDEF, NULL, 0);
    }
}

void ModInfo(void) {
    DbpString("   HF: Mifare/Ultralight/EV1 simulation; LF: scanner");
}

uint32_t DemodLF(int trigger_threshold) {
    // Configure to go in 125Khz listen mode
    LFSetupFPGAForADC(95, true);

    // clear read buffer
    BigBuf_Clear_ext(false);

    // read samples
    uint32_t ret = DoAcquisition_default(trigger_threshold, true);

    FpgaWriteConfWord(FPGA_MAJOR_MODE_OFF);
    return ret;
}

void WorkWithLF(void) {
    FpgaDownloadAndGo(FPGA_BITSTREAM_LF);

    int step = 0;
    
    LED_B_ON();
    LED_C_ON();
    for (;;) {
        WDT_HIT();
        // wait for button to be released
        while (BUTTON_PRESS())
            WDT_HIT();
        
        if (step == 0) {
            LED_B_ON();
            LED_C_ON();
            while (!BUTTON_PRESS() && !data_available())
                WDT_HIT();
            LEDsoff();
            step++;
        } else if (step == 1) {
            LED_B_ON();
            LED_D_ON();
            // read LF samples
            eloff_lf_bits = DemodLF(NOISE_AMPLITUDE_THRESHOLD * 6);
            LEDsoff();
            step= 0;
        }
        
        // exit, send a usbcommand.
        if (data_available()) break;
    }
 }

void WorkWithHF(void) {
    FpgaDownloadAndGo(FPGA_BITSTREAM_HF);

    int step = 0;
    uint8_t data[20] = {0};

    DBGLEVEL = DBG_NONE;
    for (;;) {
        WDT_HIT();
        // wait for button to be released
        while (BUTTON_PRESS())
            WDT_HIT();

        if (step == 0 || step == 1) {
            // simulation clear 4K Mifare Classic
            uint8_t block0_4uid[] = { 0x11, 0x22, 0x33, 0x44, 0x44, 0x18, 0x04, 0x00, 0x01, 0xFA, 0x33, 0xF5, 0xCB, 0x2D, 0x02, 0x1D };
            uint8_t block0_7uid[] = { 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x18, 0x44, 0x00, 0x12, 0x01, 0x11, 0x00, 0x37, 0x13 };
            uint8_t block1[16] = { 0 };
            uint8_t block3[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x07, 0x80, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
            emlSetMem(step == 0 ? block0_4uid : block0_7uid, 0, 1);
            for (int b = 1; b <= 255; b++) {
                if ((b < 128 && b % 4 == 3) || (b > 128 && (b - 128) % 16 == 15))
                    emlSetMem(block3, b, 1);
                else
                    emlSetMem(block1, b, 1);
            }
            // start simulation
            Mifare1ksim(FLAG_UID_IN_EMUL, 0, 0, data, 0, 0);
        } else if (step == 2) {
            // simulate EV1/NTAG 41 pages
            mfu_dump_t ntag = {
                .version = { 0x00, 0x04, 0x03, 0x03, 0x01, 0x00, 0x0e, 0x03 }, // NXP EV1 version 41 pages
                .pages = 40,
                .signature = {
                    0x7d, 0xc0, 0x80, 0x1d, 0x96, 0xd7, 0xbb, 0x33, 0x0d, 0x06, 0x21, 0x1d, 0x44, 0x68, 0xe1, 0x91,
                    0xb9, 0x78, 0x8a, 0x97, 0x3c, 0x2e, 0x34, 0xa4, 0x03, 0xfa, 0xb4, 0x21, 0xf1, 0xb0, 0x39, 0x5f
                },
                .counter_tearing[0][3] = 0xbd,
                .counter_tearing[1][3] = 0xbd,
                .counter_tearing[2][3] = 0xbd,
                .data = { 0x04, 0x11, 0x22, 0x37, 0x33, 0x44, 0x55, 0x66, 0x44, 0x48, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }
            };
            // 36, 37, 38, 39 pages contains config
            uint8_t suffix[] = { 0x00, 0x00, 0x00, 0xbd, 0x00, 0x00, 0x00, 0xff, 0x00, 0x05, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff };
            memcpy(&ntag.data[36 * 4], suffix, sizeof(suffix));
            uint8_t *ptr = (uint8_t *) &ntag;
            for (int b = 0; b <= 11 + MFU_DUMP_PREFIX_LENGTH / 16; b++) {
                emlSetMem(ptr, b, 1);
                ptr += 16;
            }
            // start simulation
            SimulateIso14443aTag(7, FLAG_UID_IN_EMUL, data, 0);
        } else if (step == 3) {
            // start sniffer triggered by card answer
            // no interrupts by usb ready, need button click only
            SniffIso14443a(0x1);
        }

        // exit, send a usbcommand.
        if (data_available()) break;
        
        // internal state to check exit by usb ready
        while (BUTTON_PRESS())
            WDT_HIT();
        LED_A_ON();
        LED_B_ON();
        LED_C_ON();
        LED_D_ON();
        bool isDataReady = false;
        while (!BUTTON_PRESS() && !isDataReady) {
            WDT_HIT();
            isDataReady = data_available();
        }
        LEDsoff();

        if (isDataReady) break;
        
        // Was our button held down or pressed?
        if (BUTTON_HELD(1000) > 0) {
            step++;
            if (step > 3) {
                step = 0;
            }
            SpinDown(50);
        }
    }
}

void RunMod(void) {
    StandAloneMode();

    if (BUTTON_HELD(5000) > 0) {
        WorkWithLF();
    } else {
        WorkWithHF();
    }
}
