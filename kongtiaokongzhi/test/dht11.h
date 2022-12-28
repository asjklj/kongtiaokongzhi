#ifndef dht11_mine
#define dht11_mine

#include "DHTesp.h"

#define DHTPIN 26

DHTesp dht;
//传感器初始化
void dht_init()
{
    dht.setup(DHTPIN, DHTesp::DHT11);
    return;
}
//传感器获取室内实时体感温度
float get_temperature()
{
    delay(dht.getMinimumSamplingPeriod());
    return dht.getTemperature();
}
//传感器获取室内实时湿度
float get_humidity()
{
    delay(dht.getMinimumSamplingPeriod());
    return dht.getHumidity();
}

#endif