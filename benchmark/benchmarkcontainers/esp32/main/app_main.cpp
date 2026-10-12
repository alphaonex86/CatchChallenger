// HEADLESS: yes
// ESP-IDF entry of benchmarkcontainers: the embedded suite (EmbeddedSuite.hpp)
// with the ESP32's own libstdc++ and heap, data in flash. Prints the same
// `BENCH <key>=<value>` lines as the host binary, between BENCH_BEGIN and BENCH_END.

#include "ContainerBench.hpp"
#include "EmbeddedSuite.hpp"

#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static size_t freeHeap()
{
    return heap_caps_get_free_size(MALLOC_CAP_8BIT);
}

extern "C" void app_main(void)
{
#if CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG
    // A native-USB board re-enumerates on every reset: give the host time to
    // reopen the port, or the first BENCH lines go nowhere.
    vTaskDelay(pdMS_TO_TICKS(5000));
#endif
    printf("BENCH_BEGIN free_heap=%u\n",static_cast<unsigned int>(freeHeap()));
    runEmbeddedSuite(freeHeap());
    printf("BENCH_END free_heap=%u\n",static_cast<unsigned int>(freeHeap()));
    fflush(stdout);
    // Idle without rebooting: a reboot would truncate the serial capture.
    while(true)
        vTaskDelay(pdMS_TO_TICKS(1000));
}
