// HEADLESS: yes
// ESP-IDF entry of benchmarkcontainers: the same cells as the host binary, with
// the ESP32's own libstdc++ and heap, on the datapack's files and items
// (generated into flash by the host) and the per-player FIFO pattern. Prints
// the same `BENCH <key>=<value>` lines, between BENCH_BEGIN and BENCH_END.
// The files dataset is cut to what the heap holds: the source copy plus the
// container under test, each element a FileEntry plus its path string.

#include "ContainerBench.hpp"

#include <cstring>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

extern const char * const cc_files_path[];
extern const uint32_t cc_files_size[];
extern const size_t cc_files_count;
extern const uint32_t cc_items_price[];
extern const uint8_t cc_items_consume[];
extern const size_t cc_items_count;

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

    //files: per element its FileEntry + the path block + a list node, three
    //copies alive at worst (source, container, vector growth): keep half the heap free
    size_t bytes=0;
    size_t index=0;
    while(index<cc_files_count)
    {
        bytes+=sizeof(FileEntry)+strlen(cc_files_path[index])+1+32;
        index++;
    }
    const size_t perFile=cc_files_count==0?1:bytes/cc_files_count;
    size_t count=freeHeap()/2/(3*perFile);
    if(count>cc_files_count)
        count=cc_files_count;
    print("files.available",static_cast<double>(cc_files_count));
    {
        std::vector<FileEntry> files;
        files.reserve(count);
        index=0;
        while(index<count)
        {
            FileEntry entry;
            entry.path=cc_files_path[index];
            entry.size=cc_files_size[index];
            files.push_back(entry);
            index++;
        }
        measureAll("files",files);
    }
    vTaskDelay(pdMS_TO_TICKS(50));
    {
        std::vector<ItemEntry> items;
        items.reserve(cc_items_count);
        index=0;
        while(index<cc_items_count)
        {
            ItemEntry entry;
            entry.price=cc_items_price[index];
            entry.consumeAtUse=cc_items_consume[index]!=0;
            items.push_back(entry);
            index++;
        }
        measureAll("items",items);
    }
    vTaskDelay(pdMS_TO_TICKS(50));
    measureFifos();
    print("done",1);
    printf("BENCH_END free_heap=%u\n",static_cast<unsigned int>(freeHeap()));
    fflush(stdout);
    // Idle without rebooting: a reboot would truncate the serial capture.
    while(true)
        vTaskDelay(pdMS_TO_TICKS(1000));
}
