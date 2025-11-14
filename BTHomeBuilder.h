#include <sys/_stdint.h>
#ifndef _BTHOME_BUILDER_H
#define _BTHOME_BUILDER_H

// -------------------------- 协议常量宏定义（统一管理，见名知意）--------------------------
// 1. BLE AD单元核心定义（遵循BLE核心规范）
#define AD_TYPE_FLAGS                0x01  // AD类型：广播标志
#define AD_TYPE_COMPLETE_16BIT_UUID  0x03  // AD类型：完整16位服务UUID列表
#define AD_TYPE_SERVICE_DATA_16BIT   0x16  // AD类型：16位UUID关联的服务数据

// 2. BTHome v2 协议定义（未加密模式）
#define BTHOME_UUID_16               0xFCD2  // BTHome标准16位UUID
#define BTHOME_FLAG_UNENCRYPTED      0x40    // 未加密 payload 标志（协议固定值）

// #define BTHOME_TYPE_BATTERY          0x01    // 电池电压数据类型ID（协议定义）
// #define BTHOME_TYPE_TEMPERATURE      0x02    // 温度数据类型ID（协议定义）
// #define BTHOME_TYPE_HUMIDITY         0x03    // 湿度数据类型ID（协议定义）

// 3. BLE 广播配置（兼容所有手机）
#define BLE_FLAGS_GENERAL_BLE_ONLY   0x06    // 广播标志：通用可发现+仅支持BLE

enum BTHomeType : uint8_t {
  BATTERY = 0x01,
  TEMPERATURE = 0x02,
  HUMIDITY = 0x03
};

class BTHomeBuilder {
  private:
    uint8_t buffer[25] = {0};  // 广播缓冲区（初始化清空，避免脏数据）
    uint8_t index = 0; 
    uint8_t LEN_INDEX = 3;

    void buildHeader() {
      // -------------------------- AD单元1：广播标志（必需，告知手机设备能力）--------------------------
      // 格式：AD长度(1字节) + AD类型(1字节) + 标志值(1字节)
      buffer[index++] = 0x02;                  // AD长度：后续数据占2字节（AD_TYPE_FLAGS + BLE_FLAGS）
      buffer[index++] = AD_TYPE_FLAGS;         // AD类型：广播标志
      buffer[index++] = BLE_FLAGS_GENERAL_BLE_ONLY;  // 标志值：通用可发现+仅BLE

      // 格式：AD长度(1字节) + AD类型(1字节) + UUID(2字节) + 协议标志(1字节) + 传感器数据(N字节)
      buffer[index++] = 0x0C;                          // AD长度：后续数据占12字节（UUID2 + 标志1 + 温3 + 湿3 + 电2）
      buffer[index++] = AD_TYPE_SERVICE_DATA_16BIT;     // AD类型：16位UUID关联的服务数据
      buffer[index++] = (BTHOME_UUID_16 & 0xFF);        // 关联BTHome UUID（低字节）
      buffer[index++] = (BTHOME_UUID_16 >> 8) & 0xFF;   // 关联BTHome UUID（高字节）
      buffer[index++] = BTHOME_FLAG_UNENCRYPTED;        // 协议标志：未加密（BTHome v2必需）
    }
  public:
    BTHomeBuilder& append(BTHomeType type, int16_t data) {
      // 子部分1：温度数据（BTHome格式：类型ID + 16位原始值（小端序，0.01℃））
      buffer[index++] = type;        // 数据类型ID：温度
      // int16_t tempRaw = static_cast<int16_t>(sensorData.temperature * 100);  // ℃→0.01℃（放大100倍）
      buffer[index++] = data & 0xFF;                 // 低字节（小端序优先）
      buffer[index++] = (data >> 8) & 0xFF;          // 高字节
      return *this;
    }

    BTHomeBuilder& append(BTHomeType type, float data) {
      int16_t data_cast = static_cast<int16_t>(data);
      return append(type, data_cast);
    }

    BTHomeBuilder& append(BTHomeType type, uint8_t data) {
      buffer[index++] = type;
      buffer[index++] = data;
      return *this;
    }

    String build() {
      buffer[LEN_INDEX] = index - LEN_INDEX - 1;
      return String(buffer, index);
    }
};

#endif