#pragma once
#include "Arduino.h"
#define SEL_D1 21
#define SEL_D2 19
#define SEL_D3 22
#define SEL_D4 23

#define SEL_A 2
#define SEL_B 0
#define SEL_C 4
#define SEL_D 5
#define SEL_E 18
#define SEL_F 15
#define SEL_G 16
#define SEL_DP 17

class SevSeg
{
public:
    SevSeg();
    void test();
};

SevSeg::SevSeg()
{
    pinMode(SEL_D1, OUTPUT);
    pinMode(SEL_D2, OUTPUT);
    pinMode(SEL_D3, OUTPUT);
    pinMode(SEL_D4, OUTPUT);
    pinMode(SEL_A, OUTPUT);
    pinMode(SEL_B, OUTPUT);
    pinMode(SEL_C, OUTPUT);
    pinMode(SEL_D, OUTPUT);
    pinMode(SEL_E, OUTPUT);
    pinMode(SEL_F, OUTPUT);
    pinMode(SEL_G, OUTPUT);
    pinMode(SEL_DP, OUTPUT);
    digitalWrite(SEL_D1, LOW);
    digitalWrite(SEL_D2, LOW);
    digitalWrite(SEL_D3, LOW);
    digitalWrite(SEL_D4, LOW);
    digitalWrite(SEL_A, HIGH);
    digitalWrite(SEL_B, HIGH);
    digitalWrite(SEL_C, HIGH);
    digitalWrite(SEL_D, HIGH);
    digitalWrite(SEL_E, HIGH);
    digitalWrite(SEL_F, HIGH);
    digitalWrite(SEL_G, HIGH);
    digitalWrite(SEL_DP, HIGH);
}

void SevSeg::test()
{
    uint8_t panels[4] = {SEL_D2, SEL_D3, SEL_D4};
    uint8_t segments[8] = {SEL_A, SEL_B, SEL_C, SEL_D, SEL_E, SEL_F, SEL_G, SEL_DP};

    for (int k = 0; k < 5000; k++)
        for (uint8_t s = 0; s < 8; s++)
            for (uint8_t i = 0; i < 4; i++)
            {
                digitalWrite(panels[i], HIGH);
                digitalWrite(segments[s], LOW);
                delayMicroseconds(100);
                digitalWrite(segments[s], HIGH);
                digitalWrite(panels[i], LOW);
            }

    for (uint8_t s = 0; s < 8; s++)
    {
        for (int k = 0; k < 200; k++)
            for (uint8_t i = 0; i < 4; i++)
            {
                digitalWrite(panels[i], HIGH);
                digitalWrite(segments[s], LOW);
                delay(1);
                digitalWrite(segments[s], HIGH);
                digitalWrite(panels[i], LOW);
            }
        delay(300);
    }
}