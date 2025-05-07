# Hydro Communication Backup System (Firmware)

## Adding new sensor reading task
### Defining the task

1) Define sensor in PacketTypes enum

**(./firmware/Core/Inc/Lib/packet.h)[./firmware/Core/Inc/Lib/packet.h]**
```c++
enum class PacketTypes {
  MSG = 0,    // MSG to be spoken
  ENCRYP,     // Send counter for AES over plain text

  ACK,  // Acknowledge a whole message
  NACK, // Not Acknowledge, data should be list of missing fragments

  SENSOR_xxx, // ! Define here and on receiving side !
};
```

2) Define sensor reading task

**firmware/Tasks/xxxTask.h**
```c++
#ifndef XXX_H_
#define XXX_H_

#include "shared_context.h"

#define XXX_TASK_NAME "XXX_t"

void XXXTask(void *pvParameters);

#endif /* XXX_H_ */
```

**firmware/Tasks/xxxTask.cpp**
```c++
#include "XXXTask.h"

void XXXTask(void* pvParameters) {
  // Get the shared context in this task
  auto* ctx = static_cast<SharedContext_t*>(pvParameters);

  while (true) {
    
    // Example fake sensor data (This should be read using I2C, SPI, UART, ..., etc.)
    struct {
      float engine_temperature;
      float XXXX;
    } sensorData {
      .engine_temperature = 90.4f,
      .XXXX = 846.3f,
    };

    // Send the data as raw bytes (Data will be encrypted and sent over LoRa)
    ctx->packetHandler->send(
      reinterpret_cast<const uint8_t*>(&sensorData),
      sizeof(sensorData),
      PacketTypes::SENSOR_xxx
    );

    // Delay for 25s or more
    vTaskDelay(pdMS_TO_TICKS(25000));
  }
}
```

3) Create the task

**(./firmware/Core/Src/context.cpp)[./firmware/Core/Src/context.cpp]**
```c++
// ...

// Include your task
#include <XXXTask.h>

// ...

void schedule_tasks(void) {

  /** Initialization **/

  // Begin sensor reading tasks

  xTaskCreate(
    XXXTask,
    XXX_TASK_NAME,
    256, // Should be based on how much RAM you need in task
    &ctx,
    PRIORITY_LOW,
    NULL);

  // End sensor reading tasks
}
```
