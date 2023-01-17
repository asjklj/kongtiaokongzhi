#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "cstring"

//获取当前用户是否通过网络对空调设置进行更改
//若更改，获取：1.开关状态 2.用户个性化模式怕冷怕热标准这种 3.风速 4.用户想要的室内体感温度 5.用户想要的空调模式制冷制热这种）
//返回一个json类型的字符串
String getChange()
{
    HTTPClient http;
    http.begin("http://81.68.216.118:9094/api/get/foresp32/airset/"); // HTTP begin
    int httpCode = http.GET();

    if (httpCode > 0)
    {
        // httpCode will be negative on error
        // Serial.printf("HTTP Get Code: %d\r\n", httpCode);

        if (httpCode == HTTP_CODE_OK) // 收到正确的内容
        {
            String change = http.getString();
            http.end();
            return change;
        }
    }
    else
    {
        http.end();
        return "0";
    }

    http.end();
}

//获取当前是否为用户设置的空调开启时间（1 开启；0 关闭）
//返回一个json类型的字符串
String if_auto_open(){
    // HTTPClient http;
    // http.begin("http://81.68.216.118:8021/zlh2/"); // HTTP begin
   
    // int httpCode = http.GET();

    // if (httpCode > 0)
    // {
    //     // httpCode will be negative on error
    //     // Serial.printf("HTTP Get Code: %d\r\n", httpCode);

    //     if (httpCode == HTTP_CODE_OK) // 收到正确的内容
    //     {
    //         String if_auto_open = http.getString();
    //         http.end();
    //         // return if_auto_open;
    //         return "1";

    //     }
    // }
    // else
    // {
    //     http.end();
    //     return "0";
    // }
    return "0";
}
