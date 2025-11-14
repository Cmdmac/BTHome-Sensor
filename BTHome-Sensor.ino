#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEAdvertising.h>
#include "BTHomeBuilder.h"

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
  Serial.println("ESP32-C3 快速BTHOME设备启动中...");
  
  BLEDevice::init("BTHome");
  startFastAdvertising();
  
  uint32_t targetFreq = 80;  // 目标频率：80MHz（省功耗，足够传感器+BLE 场景）
  setCpuFrequencyMhz(targetFreq);

  // 验证频率是否设置成功（可选，串口打印反馈）
  uint32_t currentFreq = getCpuFrequencyMhz();
  if (currentFreq == targetFreq) {
    Serial.printf("CPU 频率已成功设置为 %lu MHz\n", currentFreq);
  } else {
    Serial.printf("CPU 频率设置失败，当前频率：%lu MHz\n", currentFreq);
  }

  Serial.println("快速BTHOME设备开始广播");
  Serial.println("广播间隔: 100-200ms");
  Serial.println("数据更新: 立即响应");

    // 1. 配置唤醒源：RTC 定时器唤醒
  // esp_sleep_enable_timer_wakeup(10000);
  // esp_deep_sleep_start();
  // esp_light_sleep_start();

}

void loop() {
  // // 快速更新传感器数据（模拟）
  updateSensorDataFast();
  
  // // 立即更新广播数据，不等待
  updateAdvertisingFast();

  esp_sleep_enable_timer_wakeup(10 * 1000 * 1000); // 睡眠30秒
  esp_deep_sleep_start();
  // 禁用所有不需要的外设
  // esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_OFF);
  // esp_sleep_pd_config(ESP_PD_DOMAIN_XTAL, ESP_PD_OPTION_OFF);
  
  // 非常短的延迟，保持系统稳定
  // delay(10000); // 50ms延迟，实现约20次/秒的更新频率
}

void startFastAdvertising() {
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  
  BLEAdvertisementData advertisementData;
  BLEAdvertisementData scanResponseData;
  
  buildBTHomeAdvertisementData(advertisementData);
  scanResponseData.setName("BTHome");
  
  pAdvertising->setAdvertisementData(advertisementData);
  pAdvertising->setScanResponseData(scanResponseData);
  // pAdvertising->setAdvertisementType(ADV_TYPE_NONCONN_IND);
  
  // 关键：设置非常短的广播间隔
  pAdvertising->setMinInterval(0x0320);  // 32 * 0.625ms = 20ms
  pAdvertising->setMaxInterval(0x0640);  // 64 * 0.625ms = 40ms
    // esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_ADV, ESP_PWR_LVL_N12);  // -12dBm 低功耗

  // 4. 可选：设置广播类型为“非扫描响应广播”（进一步省功耗）
  // 无需手机主动扫描响应，仅广播核心数据，减少射频交互
  // pAdvertising->setAdvertisementType(0x02);
  
  pAdvertising->start();
  
}

/**
 * @brief 构建 BTHome v2 未加密广播数据（修正原AD长度错误，合规性提升）
 * @param advertisementData 输出：BLE广播数据对象
 */
void buildBTHomeAdvertisementData(BLEAdvertisementData &advertisementData) {
  BTHomeBuilder builder = BTHomeBuilder();
  String dataStr = builder.append(BTHOME_TYPE_TEMPERATURE, sensorData.temperature * 100).append(BTHOME_TYPE_HUMIDITY, sensorData.humidity * 100).append(BTHOME_TYPE_BATTERY, sensorData.battery).build();
  // buildBTHomeAdvertisementData(advertisementData);
  advertisementData.addData(dataStr);

  // 最终：将合规的广播数据添加到BLE对象
  // advertisementData.addData(reinterpret_cast<char*>(advBuffer), advIdx);
}

void updateSensorDataFast() {
  // 快速模拟数据变化
  unsigned long currentTime = millis();
  
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

void updateAdvertisingFast() {
  static unsigned long lastAdvUpdate = 0;
  unsigned long currentTime = millis();
  
  // 只有在需要更新数据时才重新配置广播
  if (currentTime - lastAdvUpdate >= 10000) { // 每50ms更新一次
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    
    // 注意：频繁停止/启动广播可能影响稳定性
    // 这里采用条件更新策略
    BLEAdvertisementData advertisementData, scanResponseData;
    // BTHomeBuilder builder = BTHomeBuilder();
    // String dataStr = builder.append(sensorData.temperature * 100).append(sensorData.humidity * 100).append(sensorData.battery).build();
    buildBTHomeAdvertisementData(advertisementData);
    // advertisementData.addData(dataStr);
    pAdvertising->setAdvertisementData(advertisementData);
    scanResponseData.setName("BTHome");
    pAdvertising->setScanResponseData(scanResponseData);
    lastAdvUpdate = currentTime;
    
    // 每100个包打印一次状态，避免串口阻塞
    if (sensorData.packetId % 100 == 0) {
      Serial.printf("包ID: %d, 温度: %.1f°C, 湿度: %.1f%%, 电池: %d%%\n", 
                   sensorData.packetId, sensorData.temperature, 
                   sensorData.humidity, sensorData.battery);
    }
  }
}