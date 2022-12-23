#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "../test/get_weather.h"
#include "../test/get_change.h"
#include "../test/dht11.h"
#include <cstring>
#include <string>
#include <ctime>
#include "../test/keep.h"
#include <IRremoteESP8266.h> //红外
#include <IRsend.h>
#include <ir_Kelvinator.h>
#include <SD.h>
#include <FS.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <JPEGDecoder.h>
#include <climits>
#define SD_MISO 21
#define SD_MOSI 19
#define SD_SCLK 22
#define SD_CS 18
void drawSdJpeg(const char *filename, int xpos, int ypos);
void jpegRender(int xpos, int ypos);
void update_wind(int speed);                   // 显示风速
void update_mode(int work);                    // 工作模式
void update_user(int usermode);                // 用户偏好
void update_tempset(int set);                  // 显示设置温度
void update_tempnow(int tempnow);              // 当前温度
void update_date(int date);                    // 显示日期
void update_day(int day);                      // 显示星期几
void update_weather(int weather);              // 显示天气
void update_today_temp(int tempmin, int tempmax); // 显示当日气温
void update_time(int hour, int min);           // 显示时间
void lcd_update_outRoom_temperature(){};
void lcd_update_conditioner_setting(){};
void lcd_update_inRoom_temperature(){};
void lcd_update_time(){};
void lcd_setup(){};
void connect_WIFI();
void humid_condition(uint8_t mode);
void hong_wai(uint8_t temp, uint8_t mode, uint8_t speed, int begin_set = 1, uint8_t light = 1, uint8_t sleep = 1);
void correct_time();
void closeAirCondition();
void set_air_conditioner(/*bool man_open*/);
const char *sid = "gongdao";            // wifi name
const char *password = "12345678";      // wifi password
const char *ntpServer = "pool.ntp.org"; // 网络时间服务器
const long gmtOffset_sec = 8 * 3600;    // 时区设置函数，东八区（UTC/GMT+8:00）写成8*3600
const int daylightOffset_sec = 0;       // 夏令时填写3600，否则填0
bool open_flag = false;                 // 判断空调循环是否为第一次
int SetWindSpeed;
int Setmode;
int switches;
int personal_mode = 3;
int wind_speed;
int conditionMode;
struct tm time_info;
const uint16_t kIrLed = 15; // ESP8266 GPIO pin to use. Recommended: 4 (D2).
IRKelvinatorAC ac(kIrLed);    // Set the GPIO to be used for sending messages.
uint8_t temp, begin;
bool flag_tm_min10 = true;
bool flag_tm_min2 = true;
int if_change, windSpeed, needTemperature, modeChange, personalMode, on_off;
char condition0[20];
char conditionId0[20];
char out_real_feel[20];
int out_real_feel0;
char iconDay0[20];
char iconNight0[20];
String r;
String s;
String t;
char WH_S[20][20] = {"/WHS/WH_S0.jpg", "/WHS/WH_S1.jpg", "/WHS/WH_S2.jpg", "/WHS/WH_S3.jpg", "/WHS/WH_S4.jpg", "/WHS/WH_S5.jpg", "/WHS/WH_S6.jpg", "/WHS/WH_S7.jpg", "/WHS/WH_S8.jpg", "/WHS/WH_S9.jpg", "/WHS/WH_Sfu.jpg"};
char WH_L[20][20] = {"/WHL/WH_L0.jpg", "/WHL/WH_L1.jpg", "/WHL/WH_L2.jpg", "/WHL/WH_L3.jpg", "/WHL/WH_L4.jpg", "/WHL/WH_L5.jpg", "/WHL/WH_L6.jpg", "/WHL/WH_L7.jpg", "/WHL/WH_L8.jpg", "/WHL/WH_L9.jpg"};
char BL_S[20][20] = {"/BLS/BL_S0.jpg", "/BLS/BL_S1.jpg", "/BLS/BL_S2.jpg", "/BLS/BL_S3.jpg", "/BLS/BL_S4.jpg", "/BLS/BL_S5.jpg", "/BLS/BL_S6.jpg", "/BLS/BL_S7.jpg", "/BLS/BL_S8.jpg", "/BLS/BL_S9.jpg"};
char BL_L[20][20] = {"/BLL/BL_L0.jpg", "/BLL/BL_L1.jpg", "/BLL/BL_L2.jpg", "/BLL/BL_L3.jpg", "/BLL/BL_L4.jpg", "/BLL/BL_L5.jpg", "/BLL/BL_L6.jpg", "/BLL/BL_L7.jpg", "/BLL/BL_L8.jpg", "/BLL/BL_L9.jpg"};
char day[10][20] = {"/DAY/WH_d1.jpg", "/DAY/WH_d2.jpg", "/DAY/WH_d3.jpg", "/DAY/WH_d4.jpg", "/DAY/WH_d5.jpg", "/DAY/WH_d6.jpg", "/DAY/WH_d7.jpg"};
char speed[5][20] = {"/WIND/speed1.jpg", "/WIND/speed2.jpg", "/WIND/speed3.jpg", "/WIND/speed4.jpg"};
char mode[5][20] = {"/MODE/0AU.jpg", "/MODE/1DR.jpg", "/MODE/2CO.jpg", "/MODE/3FA.jpg", "/MODE/4HE.jpg"};
char user[5][20] = {    "/USER/user0.jpg",    "/USER/user1.jpg",    "/USER/user2.jpg",    "/USER/user3.jpg",    "/USER/user4.jpg",};
char weather[20][30] = {"/WEATHER/sunny.jpg", "/WEATHER/duoyun.jpg", "/WEATHER/yin.jpg", "/WEATHER/xiaoyu.jpg", "/WEATHER/zhongyu.jpg", "/WEATHER/dayu.jpg", "/WEATHER/zhenyu.jpg", "/WEATHER/leizhenyu.jpg", "/WEATHER/yujiaxue.jpg", "/WEATHER/xiaoxue.jpg", "/WEATHER/zhongxue.jpg", "/WEATHER/daxue.jpg"};
TFT_eSPI tft = TFT_eSPI(320, 240);
SPIClass sdSPI(VSPI);
using namespace std;
void setup()
{
  connect_WIFI();
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  correct_time();
  ac.begin();
  pinMode(12, OUTPUT);
  digitalWrite(12, HIGH);
  tft.init();
  tft.setRotation(2);
  tft.setSwapBytes(true);
  sdSPI.begin(SD_SCLK, SD_MISO, SD_MOSI, SD_CS);
  if (!SD.begin(SD_CS, sdSPI))
  {
    return;
  }
  drawSdJpeg("/ditu.jpg", 0, 0);

  delay(2000);
}

void loop()
{
  update_today_temp(-2,7);
  correct_time();
  Serial.print("min:");
  Serial.println(time_info.tm_min);
  Serial.print("sec:");
  Serial.println(time_info.tm_sec);

  // 解析用户输入修改
  t = getChange();
  StaticJsonDocument<1024> doc2;
  deserializeJson(doc2, t);
  DeserializationError err2 = deserializeJson(doc2, t);
  Serial.println(t);
  // 读取用户输入修改的参数
  if_change = doc2["if_change"];
  windSpeed = doc2["set"]["windSpeed"];
  needTemperature = doc2["set"]["needTemperature"];
  modeChange = doc2["set"]["Mode"];
  personalMode = doc2["set"]["personalMode"];
  on_off = doc2["set"]["on_off"];
  // Serial.print("if_change:");
  // Serial.println(if_change);
  // Serial.print("windSpeed:");
  // Serial.println(windSpeed);
  // Serial.print("needTemperature:");
  // Serial.println(needTemperature);
  // Serial.print("modeChange:");
  // Serial.println(modeChange);
  // Serial.print("personalMode:");
  // Serial.println(personalMode);
  // Serial.print("on_off:");
  // Serial.println(on_off);
  // 解析当前空调应该处于的状态（开或关）
  r = if_auto_open();
  switches = (r == "0") ? 0 : 1;
  // StaticJsonDocument<1024> doc3;
  // deserializeJson(doc3, r);
  // DeserializationError err3 = deserializeJson(doc3, r);
  // int switches = doc3["if_auto_open"];
  Serial.println(r);

  if (if_change) // 如果用户通过网页进行输入且修改设置了
  {              // 更新空调状态
    switches = on_off;
    personal_mode = personalMode;
    update_user(personal_mode);
    wind_speed = windSpeed;
    update_wind(wind_speed);
    conditionMode = modeChange;
    update_mode(conditionMode);
  }
  if (!switches) // 判断当前空调是否应当开启
  {
    open_flag = false;
    closeAirCondition();
    return;
  }
  int if_on = true;   // json
  if (open_flag == 0) // 第一次
  {
    set_air_conditioner();
  }

  if (time_info.tm_min % 10 == 0) // per 10 mins
  {
    if (flag_tm_min10)
    {
      flag_tm_min10 = false;
        // 解析室外气象数据
  s = getWeather(1);
  StaticJsonDocument<1024> doc1;
  deserializeJson(doc1, s);
  DeserializationError err1 = deserializeJson(doc1, s);
  Serial.println(s);
  delay(1000);
  // 读取室外气象数据
  strcpy(condition0, doc1["data"]["hourly"][0]["condition"]);     // 天气
  strcpy(conditionId0, doc1["data"]["hourly"][0]["conditionId"]); // 天气编码
  // const char* humidity0=doc["data"]["hourly"][0]["humidity"];//室外湿度
  // const char* temp0=doc["data"]["hourly"][0]["temp"];//室外气温
  // const char* snow0=doc["data"]["hourly"][0]["snow"];//降雪
  strcpy(out_real_feel, doc1["data"]["hourly"][0]["realFeel"]); // 室外体感温度
  out_real_feel0 = std::stoi(out_real_feel);
  // const char* date0=doc["data"]["hourly"][0]["date"];//日期
  strcpy(iconDay0, doc1["data"]["hourly"][0]["iconDay"]);     // 白天天气图标
  strcpy(iconNight0, doc1["data"]["hourly"][0]["iconNight"]); // 夜晚天气图标
  // const char* qpf0=doc["data"]["hourly"][0]["qpf"];//定量降水预报
  // Serial.println(if_change);
  // Serial.println(windSpeed);
  // Serial.println(needTemperature);
  // Serial.println(mode_change);
  // 室内传感器获取数据：1.体感温度inrealfeel 2.湿度inhumidity
  // Serial.print("condition0:");
  // Serial.println(condition0);
  // Serial.print("conditionId0:");
  // Serial.println(conditionId0);
  // Serial.print("out_real_feel:");
  // Serial.println(out_real_feel);
      set_air_conditioner();
    }
  }
  else
    flag_tm_min10 = true;
  if (time_info.tm_min % 2 == 0) // per 2 mins
  {
    if (flag_tm_min2)
    {
      flag_tm_min2 = false;
      int tempNow=get_temperature();
      update_date(tempNow);
    }
  }
  else
    flag_tm_min2 = true;
  delay(5000);
  open_flag = true;
}
// 打开除湿模式
void set_air_conditioner(/*bool man_open*/)
{
  int TiGan = personalMode - 3; // 根据用户喜好进行修改温度用到的参数
  // 根据外界温度高低分类给出不同设置
  //   if (out_real_feel0 >= 35){
  // 根据当前的时间不同给出不同的设置
  //   if ((time_info.tm_hour >= 6 && time_info.tm_hour < 9) || (time_info.tm_hour > 15 && time_info.tm_hour <= 21))
  //   {
  static int setTemp;
  if (!open_flag) // 是循环的第一次时
  {
    setTemp = 25;
    SetWindSpeed = 2;
    Setmode = 2;
  }
  else if (if_change)
    keepInRealFeel(needTemperature, setTemp);
  else
    keepInRealFeel(27 + TiGan, setTemp);
  hong_wai(setTemp, Setmode, SetWindSpeed);
  //  }
  //    if (time_info.tm_hour > 21 || time_info.tm_hour < 6)
  //    {
  //      static int setTemp;
  //      if (!open_flag)
  //        setTemp = 26;
  //      else if (if_change)
  //        keepInRealFeel(needTemperature, setTemp);
  //      else
  //        keepInRealFeel(28 + TiGan, setTemp);
  //      hong_wai(setTemp, 2);
  //    }
  //    if (time_info.tm_hour > 9 || time_info.tm_hour < 15)
  //    {
  //      static int setTemp;
  //      if (!open_flag)
  //        setTemp = 24;
  //      else if (if_change)
  //        keepInRealFeel(needTemperature, setTemp);
  //      else
  //        keepInRealFeel(26 + TiGan, setTemp);
  //      hong_wai(setTemp, 2);
  //    }
  //  }

  // if (30 <= out_real_feel0 && out_real_feel0 < 35 && auto_open)
  // {
  //   if ((time_info.tm_hour >= 6 && time_info.tm_hour < 9) || (time_info.tm_hour > 15 && time_info.tm_hour <= 21))
  //   {
  //     static int setTemp;
  //     if (!open_flag)
  //       setTemp = 26;
  //     else if (if_change)
  //       keepInRealFeel(needTemperature, setTemp);
  //     else
  //       keepInRealFeel(26 + TiGan, setTemp);
  //     hong_wai(setTemp, 2);
  //   }
  //   if (time_info.tm_hour > 21 || time_info.tm_hour < 6)
  //   {
  //     static int setTemp;
  //     if (!open_flag)
  //       setTemp = 27;
  //     else if (if_change)
  //       keepInRealFeel(needTemperature, setTemp);
  //     else
  //       keepInRealFeel(27 + TiGan, setTemp);
  //     hong_wai(setTemp, 2);
  //   }
  //   if (time_info.tm_hour > 9 || time_info.tm_hour < 15)
  //   {
  //     static int setTemp;
  //     if (!open_flag)
  //       setTemp = 25;
  //     else if (if_change)
  //       keepInRealFeel(needTemperature, setTemp);
  //     else
  //       keepInRealFeel(25 + TiGan, setTemp);
  //     hong_wai(setTemp, 2);
  //   }
  // }

  // // 空调自动模式（温度范围20~30）
  // if (20 <= out_real_feel0 && out_real_feel0 < 30 && man_open)
  // {
  //   static int setTemp;
  //   if (!open_flag)
  //     setTemp = 25;
  //   else if (if_change)
  //     keepInRealFeel(needTemperature, setTemp);
  //   else
  //     keepInRealFeel(23 + TiGan, setTemp);
  //   hong_wai(setTemp, 0);
  // }

  // // 空调制热模式
  // if (15 <= out_real_feel0 && out_real_feel0 < 20 && auto_open)
  // {
  //   if ((time_info.tm_hour >= 6 && time_info.tm_hour < 9) || (time_info.tm_hour > 15 && time_info.tm_hour <= 21))
  //   {
  //     static int setTemp;
  //     if (!open_flag)
  //       setTemp = 20;
  //     else if (if_change)
  //       keepInRealFeel(needTemperature, setTemp);
  //     else
  //       keepInRealFeel(19 + TiGan, setTemp);
  //     hong_wai(setTemp, 4);
  //   }
  //   if (time_info.tm_hour > 21 || time_info.tm_hour < 6)
  //   {
  //     static int setTemp;
  //     if (!open_flag)
  //       setTemp = 22;
  //     else if (if_change)
  //       keepInRealFeel(needTemperature, setTemp);
  //     else
  //       keepInRealFeel(20 + TiGan, setTemp);
  //     hong_wai(setTemp, 4);
  //   }
  //   if (time_info.tm_hour > 9 || time_info.tm_hour < 15)
  //   {
  //     static int setTemp;
  //     if (!open_flag)
  //       setTemp = 19;
  //     else if (if_change)
  //       keepInRealFeel(needTemperature, setTemp);
  //     else
  //       keepInRealFeel(18 + TiGan, setTemp);
  //     hong_wai(setTemp, 4);
  //   }
  // }

  // if (10 <= out_real_feel0 && out_real_feel0 < 15 && auto_open)
  // {
  //   if ((time_info.tm_hour >= 6 && time_info.tm_hour < 9) || (time_info.tm_hour > 15 && time_info.tm_hour <= 21))
  //   {
  //     static int setTemp;
  //     if (!open_flag)
  //       setTemp = 20;
  //     else if (if_change)
  //       keepInRealFeel(needTemperature, setTemp);
  //     else
  //       keepInRealFeel(18 + TiGan, setTemp);
  //     hong_wai(setTemp, 4);
  //   }
  //   if (time_info.tm_hour > 21 || time_info.tm_hour < 6)
  //   {
  //     static int setTemp;
  //     if (!open_flag)
  //       setTemp = 21;
  //     else if (if_change)
  //       keepInRealFeel(needTemperature, setTemp);
  //     else
  //       keepInRealFeel(19 + TiGan, setTemp);
  //     hong_wai(setTemp, 4);
  //   }
  //   if (time_info.tm_hour > 9 || time_info.tm_hour < 15)
  //   {
  //     static int setTemp;
  //     if (!open_flag)
  //       setTemp = 19;
  //     else if (if_change)
  //       keepInRealFeel(needTemperature, setTemp);
  //     else
  //       keepInRealFeel(17 + TiGan, setTemp);
  //     hong_wai(setTemp, 4);
  //   }
  // }
  // if (0 <= out_real_feel0 && out_real_feel0 < 10 && auto_open)
  // {
  //   if ((time_info.tm_hour >= 6 && time_info.tm_hour < 9) || (time_info.tm_hour > 15 && time_info.tm_hour <= 21))
  //   {
  //     static int setTemp;
  //     if (!open_flag)
  //       setTemp = 24;
  //     else if (if_change)
  //       keepInRealFeel(needTemperature, setTemp);
  //     else
  //       keepInRealFeel(17 + TiGan, setTemp);
  //     hong_wai(setTemp, 4);
  //   }
  //   if (time_info.tm_hour > 21 || time_info.tm_hour < 6)
  //   {
  //     static int setTemp;
  //     if (!open_flag)
  //       setTemp = 24;
  //     else if (if_change)
  //       keepInRealFeel(needTemperature, setTemp);
  //     else
  //       keepInRealFeel(18 + TiGan, setTemp);
  //     hong_wai(setTemp, 4);
  //   }
  //   if (time_info.tm_hour > 9 || time_info.tm_hour < 15)
  //   {
  //     static int setTemp;
  //     if (!open_flag)
  //       setTemp = 20;
  //     else if (if_change)
  //       keepInRealFeel(needTemperature, setTemp);
  //     else
  //       keepInRealFeel(16 + TiGan, setTemp);
  //     hong_wai(setTemp, 4);
  //   }
  // }

  // if (out_real_feel0 < 0 && auto_open)
  // {
  //   if ((time_info.tm_hour >= 6 && time_info.tm_hour < 9) || (time_info.tm_hour > 15 && time_info.tm_hour <= 21))
  //   {
  //     static int setTemp;
  //     if (!open_flag)
  //       setTemp = 24;
  //     else if (if_change)
  //       keepInRealFeel(needTemperature, setTemp);
  //     else
  //       keepInRealFeel(17 + TiGan, setTemp);
  //     hong_wai(setTemp, 4);
  //   }
  //   if (time_info.tm_hour > 21 || time_info.tm_hour < 6)
  //   {
  //     static int setTemp;
  //     if (!open_flag)
  //       setTemp = 25;
  //     else if (if_change)
  //       keepInRealFeel(needTemperature, setTemp);
  //     else
  //       keepInRealFeel(18 + TiGan, setTemp);
  //     hong_wai(setTemp, 4);
  //   }
  //   if (time_info.tm_hour > 9 || time_info.tm_hour < 15)
  //   {
  //     static int setTemp;
  //     if (!open_flag)
  //       setTemp = 20;
  //     else if (if_change)
  //       keepInRealFeel(needTemperature, setTemp);
  //     else
  //       keepInRealFeel(16 + TiGan, setTemp);
  //     hong_wai(setTemp, 4);
  //   }
  // }

  // // 除湿模式
  // if (get_humidity()> 75 && auto_open)
  // {
  //   humid_condition(1);
  // }

  // break;
  // switch (mode)

  // { // choose "mode:非常怕冷1 比较怕冷2 正常3 比较怕热4 非常怕热5";
  //   // case(非常怕冷1)：{
  //   // if (out_real_feel>35 && auto_open) 制冷模式from 7:00-10:00 & 14:30-21:00 setTemp=25;from 21:00-7:00 setTemp=26;from 10:00-14:30 setTemp=24;keepInRealFeel(28);
  //   // if (30<out_real_feel<35 && auto_open) 制冷模式from 7:00-10:00 & 14:30-21:00 setTemp=26;from 21:00-7:00 setTemp=27;from 10:00-14:30 setTemp=25;keepInRealFeel(28);
  //   // if（20<out_real_feel<30&& man_open) 自动模式 keepInRealFeel(27);
  //   // if (15<out_real_feel<20 && auto_open) 制热模式from 7:00-10:00 & 14:30-21:00 setTemp=20;from 21:00-7:00 setTemp=22;from 10:00-14:30 setTemp=18;keepInRealFeel(24);
  //   // if (10<out_real_feel<15 && auto_open) 制热模式from 7:00-10:00 & 14:30-21:00 setTemp=19;from 21:00-7:00 setTemp=21;from 10:00-14:30 setTemp=17;keepInRealFeel(22);
  //   // if (0<out_real_feel<10 && auto_open) 制热模式from 7:00-10:00 & 14:30-21:00 setTemp=22;from 21:00-7:00 setTemp=24;from 10:00-14:30 setTemp=20;keepInRealFeel(21);
  //   // if (out_real_feel<0 && auto_open) 制热模式from 7:00-10:00 & 14:30-21:00 setTemp=25;from 21:00-7:00 setTemp=26;from 10:00-14:30 setTemp=18;keepInRealFeel(21);
  //   // if (inhumity >75 && auto_open)  更改为除湿模式 keep 40<inhumidity<60 in summer and 50<inhumidity<70 in winter ;
  //   // if (realfeel0-realfeeli>10&&realfeel0<25)open air condition(强降温)}break;

  //   // case(比较怕冷2)：{
  //   // if (out_real_feel>35 && auto_open) 制冷模式from 6:30-9:00 & 15:00-21:00 setTemp=25;from 21:00-6:30 setTemp=26;from 9:00-15:00 setTemp=24;keepInRealFeel(27);
  //   // if (30<out_real_feel<35 && auto_open) 制冷模式from 6:30-9:00 & 15:00-21:00 setTemp=26;from 21:00-6:30 setTemp=27;from 9:00-15:00 setTemp=25;keepInRealFeel(27);
  //   // if（20<out_real_feel<30&& man_open) 自动模式 keepInRealFeel(26);
  //   // if (15<out_real_feel<20 && auto_open) 制热模式from 6:30-9:00 & 15:00-21:00 setTemp=20;from 21:00-6:30 setTemp=22;from 9:00-15:00 setTemp=18;keepInRealFeel(23);
  //   // if (10<out_real_feel<15 && auto_open) 制热模式from 6:30-9:00 & 15:00-21:00 setTemp=19;from 21:00-6:30 setTemp=21;from 9:00-15:00 setTemp=17;keepInRealFeel(21);
  //   // if (0<out_real_feel<10 && auto_open) 制热模式from 6:30-9:00 & 15:00-21:00 setTemp=22;from 21:00-6:30 setTemp=24;from 9:00-15:00 setTemp=20;keepInRealFeel(20);
  //   // if (out_real_feel<0 && auto_open) 制热模式from 6:30-9:00 & 15:00-21:00 setTemp=25;from 21:00-6:30 setTemp=26;from 9:00-15:00 setTemp=18;keepInRealFeel(20);
  //   // if (inhumity >75 && auto_open)  更改为除湿模式 keep 40<inhumidity<60 in summer and 50<inhumidity<70 in winter ;
  //   // if (realfeel0-realfeeli>10&&realfeel0<25)open air condition(强降温)}break;

  //   // case 3: //正常模式
  //   // 空调制冷模式
  //   // case(比较怕热4)：{
  //   // if (out_real_feel>35 && autoopen) 制冷模式from 6:30-9:00 & 15:00-21:00 setTemp=25;from 21:00-6:30 setTemp=26;from 9:00-15:00 setTemp=24;keepInRealFeel(25);
  //   // if (30<out_real_feel<35 && autoopen) 制冷模式from 6:30-9:00 & 15:00-21:00 setTemp=26;from 21:00-6:30 setTemp=27;from 9:00-15:00 setTemp=25;keepInRealFeel(25);
  //   // if(20<out_real_feel<30&& man_open) 自动模式 keepInRealFeel(23);
  //   // if (15<out_real_feel<20 && autoopen) 制热模式from 6:30-9:00 & 15:00-21:00 setTemp=20;from 21:00-6:30 setTemp=22;from 9:00-15:00 setTemp=18;keepInRealFeel(20);
  //   // if (10<out_real_feel<15 && autoopen) 制热模式from 6:30-9:00 & 15:00-21:00 setTemp=19;from 21:00-6:30 setTemp=21;from 9:00-15:00 setTemp=17;keepInRealFeel(19);
  //   // if (0<out_real_feel<10 && autoopen) 制热模式from 6:30-9:00 & 15:00-21:00 setTemp=22;from 21:00-6:30 setTemp=24;from 9:00-15:00 setTemp=20;keepInRealFeel(19);
  //   // if (out_real_feel<0 && autoopen) 制热模式from 6:30-9:00 & 15:00-21:00 setTemp=25;from 21:00-6:30 setTemp=26;from 9:00-15:00 setTemp=18;keepInRealFeel(19);
  //   // if (inhumity >75 && autoopen)  更改为除湿模式 keep 40<inhumidity<60 in summer and 50<inhumidity<70 in winter ;
  //   // if (realfeel0-realfeeli>10&&realfeel0<25)open air condition(强降温)}break;
  //   // 7.if (temp>32||temp<10) open;

  //   // case(非常怕热5)：{
  //   // if (out_real_feel>35 && autoopen) 制冷模式from 6:30-9:00 & 15:00-21:00 setTemp=25;from 21:00-6:30 setTemp=26;from 9:00-15:00 setTemp=24;keepInRealFeel(24);
  //   // if (30<out_real_feel<35 && autoopen) 制冷模式from 6:30-9:00 & 15:00-21:00 setTemp=26;from 21:00-6:30 setTemp=27;from 9:00-15:00 setTemp=25;keepInRealFeel(24);
  //   // if（20<out_real_feel<30&& man_open) 自动模式 keepInRealFeel(22);
  //   // if (15<out_real_feel<20 && autoopen) 制热模式from 6:30-9:00 & 15:00-21:00 setTemp=20;from 21:00-6:30 setTemp=22;from 9:00-15:00 setTemp=18;keepInRealFeel(19);
  //   // if (10<out_real_feel<15 && autoopen) 制热模式from 6:30-9:00 & 15:00-21:00 setTemp=19;from 21:00-6:30 setTemp=21;from 9:00-15:00 setTemp=17;keepInRealFeel(18);
  //   // if (0<out_real_feel<10 && autoopen) 制热模式from 6:30-9:00 & 15:00-21:00 setTemp=22;from 21:00-6:30 setTemp=24;from 9:00-15:00 setTemp=20;keepInRealFeel(18);
  //   // if (out_real_feel<0 && autoopen) 制热模式from 6:30-9:00 & 15:00-21:00 setTemp=25;from 21:00-6:30 setTemp=26;from 9:00-15:00 setTemp=18;keepInRealFeel1(8);
  //   // if (inhumity >75 && autoopen)  更改为除湿模式 keep 40<inhumidity<60 in summer and 50<inhumidity<70 in winter ;
  //   // if (realfeel0-realfeeli>10&&realfeel0<25)open air condition(强降温)}break;
  // }
  // // Serial.print("condition: ");
  // // Serial.println(condition0);
  // // Serial.print("conditionId: ");Serial.println(conditionId0);
  // // Serial.print("date: ");Serial.println(date0);
  // // Serial.print("hour: ");Serial.println(hour0);
  // // Serial.print("humidity: ");Serial.println(humidity0);
  // // Serial.print("iconDay: ");Serial.println(iconDay0);
  // // Serial.print("iconNight");Serial.println(iconNight0);
  // // Serial.print("pop: ");Serial.println(pop0);
  // // Serial.print("pressure: ");Serial.println(pressure0);
  // // Serial.print("qpf: ");Serial.println(qpf0);
  // // Serial.print("realFeel: ");Serial.println(realFeel0);
  // // Serial.print("snow: ");Serial.println(snow0);
  // // Serial.print("temp: ");Serial.println(temp0);
  // // Serial.print("updatetime:");Serial.println(updatetime0)
}
void humid_condition(uint8_t mode)
{
  if (1)
  {
    delay(200);
    ac.on();
    ac.setMode(mode);
    ac.setTurbo(false);

#if SEND_GREE
    ac.send();
#endif
    delay(1000);
  }
  else
  {
    ac.off();
    Serial.println("not running");
  }
}
// 关机
void closeAirCondition()
{
  ac.off();
  // Serial.println("not running");
}
// 红外模块
void hong_wai(uint8_t temp, uint8_t mode, uint8_t speed, int begin_set, uint8_t light, uint8_t sleep) // 请输入 是否打开(1打开0关闭) 温度 模式(0自动1除湿2制冷3通风4制热) 风速(123)  扫风 灯光 睡眠;
{
  if (begin_set)
  {
    delay(200);
    ac.on();
    ac.setFan(speed); //(123)
                      // kGreeAuto 0, kGreeDry 1, kGreeCool 2, kGreeFan 3, kGreeHeat 4
    ac.setMode(mode);
    ac.setTemp(temp); // 16
    ac.setXFan(false);
    ac.setLight(light);
    ac.setTurbo(false);
    update_wind(speed);
    update_mode(mode);
    update_tempset(temp);

#if SEND_GREE
    ac.send();
#endif
    delay(1000);
  }
  else
  {
    ac.off();
    Serial.println("not running");
  }
}
// WIFI连接
void connect_WIFI()
{
  Serial.begin(115200); // open the serial port at 115200 bps;
  delay(100);
  Serial.print("Connecting.. ");
  WiFi.begin(sid, password);
  while (WiFi.status() != WL_CONNECTED)
  { // 这里是阻塞程序，直到连接成功
    delay(300);
    Serial.print(".");
  }
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
}
// 校对当前时间
void correct_time()
{
  getLocalTime(&time_info);
  update_date(time_info.tm_mday);
  update_day(time_info.tm_wday);
  update_time(time_info.tm_hour,time_info.tm_min);
}
// 设置空调状态
void update_wind(int speedd)//显示风速
{
    int i = speedd;
    drawSdJpeg(speed[i], 257, 24);
}
void update_mode(int modee)//工作模式
{
    int i = modee;
    drawSdJpeg(mode[i], 257, 57);
}
void update_user(int userr)//用户偏好
{
    int i = userr;
    drawSdJpeg(user[i], 257, 90);

}
void update_tempset(int set)//显示设置温度
{
    int i = set / 10;
    int j = set % 10;
    drawSdJpeg(BL_S[i], 218, 128);
    drawSdJpeg(BL_S[j], 238, 128);
}
void update_tempnow(int tempnow)//当前温度
{
    int i = tempnow / 10;
    int j = tempnow % 10;
    drawSdJpeg(BL_L[i], 15, 50);
    drawSdJpeg(BL_L[j], 80, 50);
}
void update_date(int date)//显示日期
{
    int i = date;
    if (i < 10)
        // showImage(152, 42, 6, 7, Image_WH_S[i]);
        drawSdJpeg(WH_S[i], 162, 191);
    else {
        drawSdJpeg(WH_S[i / 10], 162, 191);
        drawSdJpeg(WH_S[i % 10], 169, 191);
    }
}
void update_day(int dayy)//显示星期几
{
    int i = dayy;
    drawSdJpeg(day[i], 180, 191);
}
void update_weather(int weatherr)//显示天气
{
    int i = weatherr;
    drawSdJpeg(weather[i], 235, 177);
}
void update_today_temp(int tempmin, int tempmax)//显示当日气温
{
    if (tempmin >= 10)
    {
        drawSdJpeg(WH_S[tempmin / 10], 162, 207);
        drawSdJpeg(WH_S[tempmin % 10], 169, 207);
    }
    else if (tempmin >= 0)
    {
        drawSdJpeg(WH_S[0], 162, 207);
        drawSdJpeg(WH_S[tempmin % 10], 169, 207);
    }
    else if (tempmin > (-10))
    {
        drawSdJpeg(WH_S[10], 162, 207);
        drawSdJpeg(WH_S[(-tempmin) % 10], 169, 207);
    }
    else;
    if (tempmax >= 10)
    {
        drawSdJpeg(WH_S[tempmax / 10], 191, 207);
        drawSdJpeg(WH_S[tempmax % 10], 199, 207);
    }
    else if (tempmax >= 0)
    {
        drawSdJpeg(WH_S[0], 191, 207);
        drawSdJpeg(WH_S[tempmax % 10], 199, 207);
    }
    else if (tempmax > (-10))
    {
        drawSdJpeg(WH_S[10], 191, 207);
        drawSdJpeg(WH_S[(-tempmax) % 10], 199, 207);
    }
    else;
}
void update_time(int hour, int min)//显示时间
{
    int i = hour / 10;
    int j = hour % 10;
    drawSdJpeg(WH_L[i], 30, 184);
    drawSdJpeg(WH_L[j], 50, 184);
    i = min / 10;
    j = min % 10;
    drawSdJpeg(WH_L[i], 93, 184);
    drawSdJpeg(WH_L[j], 113, 184);
}
void drawSdJpeg(const char* filename, int xpos, int ypos) {
    File jpegFile = SD.open(filename, FILE_READ);
    if (!jpegFile) {
        return;
    }
    boolean decoded = JpegDec.decodeSdFile(jpegFile);
    if (decoded) {
        jpegRender(xpos, ypos);
    }
}
void jpegRender(int xpos, int ypos) {
    uint32_t drawTime = millis();
    uint16_t* pImg;
    uint16_t mcu_w = JpegDec.MCUWidth;
    uint16_t mcu_h = JpegDec.MCUHeight;
    uint32_t max_x = JpegDec.width;
    uint32_t max_y = JpegDec.height;
    bool swapBytes = tft.getSwapBytes();
    tft.setSwapBytes(true);
    uint32_t min_w = (mcu_w < (max_x% mcu_w) ? mcu_w : (max_x % mcu_w));
    uint32_t min_h = (mcu_h < (max_y% mcu_h) ? mcu_h : (max_y % mcu_h));
    uint32_t win_w = mcu_w;
    uint32_t win_h = mcu_h;
    max_x += xpos;
    max_y += ypos;
    while (JpegDec.read()) {
        pImg = JpegDec.pImage;
        int mcu_x = JpegDec.MCUx * mcu_w + xpos;
        int mcu_y = JpegDec.MCUy * mcu_h + ypos;
        if (mcu_x + mcu_w <= max_x) win_w = mcu_w;
        else win_w = min_w;
        if (mcu_y + mcu_h <= max_y) win_h = mcu_h;
        else win_h = min_h;
        if (win_w != mcu_w)
        {
            uint16_t* cImg;
            int p = 0;
            cImg = pImg + win_w;
            for (int h = 1; h < win_h; h++)
            {
                p += mcu_w;
                for (int w = 0; w < win_w; w++)
                {
                    *cImg = *(pImg + w + p);
                    cImg++;
                }
            }
        }
        uint32_t mcu_pixels = win_w * win_h;
        if ((mcu_x + win_w) <= tft.width() && (mcu_y + win_h) <= tft.height())
            tft.pushImage(mcu_x, mcu_y, win_w, win_h, pImg);
        else if ((mcu_y + win_h) >= tft.height())
            JpegDec.abort();
    }
    tft.setSwapBytes(swapBytes);
}
