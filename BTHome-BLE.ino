#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEAdvertising.h>

// -------------------------- 协议常量宏定义（统一管理，见名知意）--------------------------
// 1. BLE AD单元核心定义（遵循BLE核心规范）
#define AD_TYPE_FLAGS                0x01  // AD类型：广播标志
#define AD_TYPE_COMPLETE_16BIT_UUID  0x03  // AD类型：完整16位服务UUID列表
#define AD_TYPE_SERVICE_DATA_16BIT   0x16  // AD类型：16位UUID关联的服务数据

// 2. BTHome v2 协议定义（未加密模式）
#define BTHOME_UUID_16               0xFCD2  // BTHome标准16位UUID
#define BTHOME_FLAG_UNENCRYPTED      0x40    // 未加密 payload 标志（协议固定值）
#define BTHOME_TYPE_TEMPERATURE      0x02    // 温度数据类型ID（协议定义）
#define BTHOME_TYPE_HUMIDITY         0x03    // 湿度数据类型ID（协议定义）
#define BTHOME_TYPE_BATTERY          0x01    // 电池电压数据类型ID（协议定义）

// 3. BLE 广播配置（兼容所有手机）
#define BLE_FLAGS_GENERAL_BLE_ONLY   0x06    // 广播标志：通用可发现+仅支持BLE


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

  // esp_sleep_enable_timer_wakeup(10 * 1000 * 1000); // 睡眠30秒
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
  pAdvertising->setAdvertisementType(0x02);
  
  pAdvertising->start();
  
}

// void buildFastAdvertisementData(BLEAdvertisementData &advertisementData) {
//   uint8_t beaconData[25];
//   int index = 0;
  
//   // 标志数据
//   beaconData[index++] = 0x02;
//   beaconData[index++] = AD_TYPE_FLAGS;//0x01;  
//   beaconData[index++] = BLE_FLAGS_GENERAL_BLE_ONLY;//0x06;
  
//   // UUID列表
//   beaconData[index++] = 0x03;
//   beaconData[index++] = 0x03;
//   beaconData[index++] = 0xD2;
//   beaconData[index++] = BTHOME_UUID_16;//0xFC;
  
//   // BTHOME服务数据
//   beaconData[index++] = 0x0C;
//   beaconData[index++] = 0x16;
//   beaconData[index++] = 0xD2;
//   beaconData[index++] = 0xFC;
//   beaconData[index++] = 0x40;
  
//   // 温度数据
//   beaconData[index++] = 0x02;
//   int16_t temp = sensorData.temperature * 100;
//   beaconData[index++] = temp & 0xFF;
//   beaconData[index++] = (temp >> 8) & 0xFF;
  
//   // 湿度数据
//   beaconData[index++] = 0x03;
//   int16_t hum = sensorData.humidity * 100;
//   beaconData[index++] = hum & 0xFF;
//   beaconData[index++] = (hum >> 8) & 0xFF;
  
//   // 电池数据 + 包ID（用于跟踪更新）
//   beaconData[index++] = 0x01;
//   beaconData[index++] = sensorData.battery;
  
//   advertisementData.addData((char*)beaconData, index);
// }

/**
 * @brief 构建 BTHome v2 未加密广播数据（修正原AD长度错误，合规性提升）
 * @param advertisementData 输出：BLE广播数据对象
 */
void buildBTHomeAdvertisementData(BLEAdvertisementData &advertisementData) {
  uint8_t advBuffer[25] = {0};  // 广播缓冲区（初始化清空，避免脏数据）
  uint8_t advIdx = 0;           // 缓冲区索引（命名更明确：adv=advertisement）

  // -------------------------- AD单元1：广播标志（必需，告知手机设备能力）--------------------------
  // 格式：AD长度(1字节) + AD类型(1字节) + 标志值(1字节)
  advBuffer[advIdx++] = 0x02;                  // AD长度：后续数据占2字节（AD_TYPE_FLAGS + BLE_FLAGS）
  advBuffer[advIdx++] = AD_TYPE_FLAGS;         // AD类型：广播标志
  advBuffer[advIdx++] = BLE_FLAGS_GENERAL_BLE_ONLY;  // 标志值：通用可发现+仅BLE

  // -------------------------- AD单元2：BTHome服务UUID（必需，让手机识别BTHome设备）--------------------------
  // 格式：AD长度(1字节) + AD类型(1字节) + UUID(2字节，小端序)
  // advBuffer[advIdx++] = 0x03;                          // AD长度：后续数据占2字节（仅UUID）→ 修正原0x03错误
  // advBuffer[advIdx++] = AD_TYPE_COMPLETE_16BIT_UUID;    // AD类型：完整16位UUID列表
  // advBuffer[advIdx++] = (BTHOME_UUID_16 & 0xFF);        // BTHome UUID低字节（0xFC）→ 小端序
  // advBuffer[advIdx++] = (BTHOME_UUID_16 >> 8) & 0xFF;   // BTHome UUID高字节（0xD2）→ 小端序

  // -------------------------- AD单元3：BTHome核心数据（温湿度+电池，未加密）--------------------------
  // 格式：AD长度(1字节) + AD类型(1字节) + UUID(2字节) + 协议标志(1字节) + 传感器数据(N字节)
  advBuffer[advIdx++] = 0x0C;                          // AD长度：后续数据占12字节（UUID2 + 标志1 + 温3 + 湿3 + 电2）
  advBuffer[advIdx++] = AD_TYPE_SERVICE_DATA_16BIT;     // AD类型：16位UUID关联的服务数据
  advBuffer[advIdx++] = (BTHOME_UUID_16 & 0xFF);        // 关联BTHome UUID（低字节）
  advBuffer[advIdx++] = (BTHOME_UUID_16 >> 8) & 0xFF;   // 关联BTHome UUID（高字节）
  advBuffer[advIdx++] = BTHOME_FLAG_UNENCRYPTED;        // 协议标志：未加密（BTHome v2必需）

  // 子部分1：温度数据（BTHome格式：类型ID + 16位原始值（小端序，0.01℃））
  advBuffer[advIdx++] = BTHOME_TYPE_TEMPERATURE;        // 数据类型ID：温度
  int16_t tempRaw = static_cast<int16_t>(sensorData.temperature * 100);  // ℃→0.01℃（放大100倍）
  advBuffer[advIdx++] = tempRaw & 0xFF;                 // 温度低字节（小端序优先）
  advBuffer[advIdx++] = (tempRaw >> 8) & 0xFF;          // 温度高字节

  // 子部分2：湿度数据（BTHome格式：类型ID + 16位原始值（小端序，0.01%RH））
  advBuffer[advIdx++] = BTHOME_TYPE_HUMIDITY;           // 数据类型ID：湿度
  int16_t humiRaw = static_cast<int16_t>(sensorData.humidity * 100);  // %RH→0.01%RH
  advBuffer[advIdx++] = humiRaw & 0xFF;                 // 湿度低字节
  advBuffer[advIdx++] = (humiRaw >> 8) & 0xFF;          // 湿度高字节

  // 子部分3：电池数据（BTHome格式：类型ID + 8位原始值（0.01V））
  advBuffer[advIdx++] = BTHOME_TYPE_BATTERY;            // 数据类型ID：电池电压（修正原“包ID”注释错误）
  advBuffer[advIdx++] = sensorData.battery;              // 电池原始值（如330→3.3V）

  // 最终：将合规的广播数据添加到BLE对象
  advertisementData.addData(reinterpret_cast<char*>(advBuffer), advIdx);
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
    buildBTHomeAdvertisementData(advertisementData);
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