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
#include <ir_Gree.h>
void lcd_update_outRoom_temperature(){};
void lcd_update_conditioner_setting(){};
void lcd_update_inRoom_temperature(){};
void lcd_update_time(){};
void lcd_setup(){};
void connect_WIFI();
void humid_condition(uint8_t mode);
void hong_wai(uint8_t temp, uint8_t mode, uint8_t speed, int begin_set = 1, uint8_t light = 1, uint8_t sleep = 1);
void correct_time();
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
IRGreeAC ac(kIrLed);        // Set the GPIO to be used for sending messages.
uint8_t temp, begin;
bool flag_tm_min10 = true;
bool flag_tm_min2 = true;

void setup()
{
  connect_WIFI();
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  correct_time();
  ac.begin();
  lcd_setup();
}

void loop()
{
  correct_time();
  Serial.print("min:");
  Serial.println(time_info.tm_min);
  Serial.print("sec:");
  Serial.println(time_info.tm_sec);
  // http
  int if_on = true; // json
  if (if_on && open_flag == 0)
  {
    set_air_conditioner();
    lcd_update_outRoom_temperature();
    lcd_update_conditioner_setting();
  }

  if (time_info.tm_min % 10 == 0) // per 10 mins
  {
    if (flag_tm_min10)
    {
      flag_tm_min10 = false;
      set_air_conditioner();
      lcd_update_outRoom_temperature();
      lcd_update_conditioner_setting();
    }
  }
  else
    flag_tm_min10 = true;
  if (time_info.tm_min % 2 == 0) // per 2 mins
  {
    if (flag_tm_min2)
    {
      flag_tm_min2 = false;
      lcd_update_inRoom_temperature();
    }
  }
  else
    flag_tm_min2 = true;

  lcd_update_time();
  // delay(60000);
  delay(5000);
  open_flag = true;
}
// 打开除湿模式
void humid_condition(uint8_t mode)
{
  if (begin)
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
    ac.setSleep(sleep);
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
}
// 设置空调状态
void set_air_conditioner(/*bool man_open*/)
{
  // 解析室外气象数据
  String s = getWeather(1);
  StaticJsonDocument<1024> doc1;
  deserializeJson(doc1, s);
  DeserializationError err1 = deserializeJson(doc1, s);
  Serial.println(s);
  delay(1000);
  // 读取室外气象数据
  const char *condition0 = doc1["data"]["hourly"][0]["condition"];     // 天气
  const char *conditionId0 = doc1["data"]["hourly"][0]["conditionId"]; // 天气编码
  // const char* humidity0=doc["data"]["hourly"][0]["humidity"];//室外湿度
  // const char* temp0=doc["data"]["hourly"][0]["temp"];//室外气温
  // const char* snow0=doc["data"]["hourly"][0]["snow"];//降雪
  const char *out_real_feel = doc1["data"]["hourly"][0]["realFeel"]; // 室外体感温度
  int out_real_feel0 = std::stoi(out_real_feel);
  // const char* date0=doc["data"]["hourly"][0]["date"];//日期
  const char *iconDay0 = doc1["data"]["hourly"][0]["iconDay"];     // 白天天气图标
  const char *iconNight0 = doc1["data"]["hourly"][0]["iconNight"]; // 夜晚天气图标
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
  // 解析用户输入修改
  String t = getChange();
  StaticJsonDocument<1024> doc2;
  deserializeJson(doc2, t);
  DeserializationError err2 = deserializeJson(doc2, t);
  Serial.println(t);
  // 读取用户输入修改的参数
  int if_change = doc2["if_change"];
  int windSpeed = doc2["set"]["windSpeed"];
  int needTemperature = doc2["set"]["needTemperature"];
  int modeChange = doc2["set"]["Mode"];
  int personalMode = doc2["set"]["personalMode"];
  int on_off = doc2["set"]["on_off"];
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
  String r = if_auto_open();
 	int switches = (r =="0") ? 0 : 1;
  // StaticJsonDocument<1024> doc3;
  // deserializeJson(doc3, r);
  // DeserializationError err3 = deserializeJson(doc3, r);
  // int switches = doc3["if_auto_open"];
  Serial.println(r);

  if (if_change) // 如果用户通过网页进行输入且修改设置了
  {              // 更新空调状态
    switches = on_off;
    personal_mode = personalMode;
    wind_speed = windSpeed;
    conditionMode = modeChange;
  }
  if (!switches) // 判断当前空调是否应当开启
  {
    open_flag = false;
    closeAirCondition();
    return;
  }
  int TiGan = personalMode - 3; // 根据用户喜好进行修改温度用到的参数
int open_flag;
  //根据外界温度高低分类给出不同设置
  //  if (out_real_feel0 >= 35)
   {//根据当前的时间不同给出不同的设置
    //  if ((time_info.tm_hour >= 6 && time_info.tm_hour < 9) || (time_info.tm_hour > 15 && time_info.tm_hour <= 21))
    //  {
       static int setTemp;
       if (!open_flag)//是循环的第一次时
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