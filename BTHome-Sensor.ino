#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEAdvertising.h>
#include "BTHomeBuilder.h"

#define DEVICE_NAME "BTHome"

struct SensorData {
  float temperature = 23.5;
  float humidity = 45.2;
  uint8_t battery = 85;
  uint16_t packetId = 0;
  unsigned long lastUpdate = 0;
};

SensorData sensorData;

void setup() {
  Serial.begin(115200);
  Serial.println("setup");

  //初始化BLE
  BLEDevice::init(DEVICE_NAME);
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  //设置广播间隔
  pAdvertising->setMinInterval(160);  // 32 * 0.625ms = 20ms
  pAdvertising->setMaxInterval(240);  // 64 * 0.625ms = 40ms
  // 初始广播数据
  BLEAdvertisementData advertisementData, scanResponseData;
  buildBTHomeAdvertisementData(advertisementData);
  // advertisementData.addData(dataStr);
  pAdvertising->setAdvertisementData(advertisementData);
  // 设置设备名称
  scanResponseData.setName(DEVICE_NAME);
  pAdvertising->setScanResponseData(scanResponseData);
  pAdvertising->start();
  
}

void loop() {
  //更新数据
  updateSensorData();
  //发送广播
  broardcastData();
}

/**
 * @brief 构建 BTHome v2 未加密广播数据（修正原AD长度错误，合规性提升）
 * @param advertisementData 输出：BLE广播数据对象
 */
void buildBTHomeAdvertisementData(BLEAdvertisementData &advertisementData) {
  BTHomeBuilder builder = BTHomeBuilder();
  String dataStr = builder.append(BTHomeType::TEMPERATURE, sensorData.temperature * 100).append(BTHomeType::HUMIDITY, sensorData.humidity * 100).append(BTHomeType::BATTERY, sensorData.battery).build();
  // buildBTHomeAdvertisementData(advertisementData);
  advertisementData.addData(dataStr);
}

void updateSensorData() {
  // 快速模拟数据变化
  unsigned long currentTime = esp_timer_get_time() / 1000;

  // 每100ms微调数据，实现平滑变化
  if (currentTime - sensorData.lastUpdate >= 10000) {
    sensorData.temperature = (sin(currentTime * 0.001) * 5.0) + (rand() % 20) * 0.1;
    sensorData.humidity = (cos(currentTime * 0.0007) * 10.0) + (rand() % 30) * 0.1;
    sensorData.packetId++;
    sensorData.lastUpdate = currentTime;

    // 每100个包减少1%电量
    if (sensorData.packetId % 100 == 0) {
      sensorData.battery = max(0, sensorData.battery - 1);
    }
  }
}

void broardcastData() {

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  BLEAdvertisementData advertisementData;
  // BTHomeBuilder builder = BTHomeBuilder();
  // String dataStr = builder.append(sensorData.temperature * 100).append(sensorData.humidity * 100).append(sensorData.battery).build();
  buildBTHomeAdvertisementData(advertisementData);
  // advertisementData.addData(dataStr);
  pAdvertising->setAdvertisementData(advertisementData);

  pAdvertising->start();

}