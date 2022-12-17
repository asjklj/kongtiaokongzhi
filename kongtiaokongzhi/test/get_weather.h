#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "cstring"

// void WiFi_Connect(char *name, char *password)
// {
//     WiFi.begin(name, password);
//     Serial.print("connecting.");
//     while (WiFi.status() != WL_CONNECTED)
//     { //这里是阻塞程序，直到连接成功
//         delay(300);
//         Serial.print(".");
//     }
// }

//获取当前室外气象状况
String getWeather(int citiID)
{
    HTTPClient http;
    http.begin("https://aliv18.data.moji.com/whapi/json/alicityweather/forecast24hours"); // HTTP begin
    http.addHeader("cityId", "50");
    http.addHeader("Authorization", "APPCODE 2492c984aba14ee18708854e7db33b1b");
    int httpCode = http.POST("");

    if (httpCode > 0)
    {
        // httpCode will be negative on error
        // Serial.printf("HTTP Get Code: %d\r\n", httpCode);

        if (httpCode == HTTP_CODE_OK) // 收到正确的内容
        {
            String resBuff = http.getString();
            http.end();
            return resBuff;
        }
    }
    else
    {
        http.end();
        return "0";
    }

    http.end();
}