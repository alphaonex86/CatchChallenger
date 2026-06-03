// app_main.cpp — ESP-IDF entry for the CatchChallenger all-in-one game server.
//
// On ESP32 there is no filesystem and very little RAM, so the server is built
// with the datapack-cpp profile: the datapack AND the server settings are
// compiled into flash (.rodata, read in place) and the account/character DB is
// the in-memory backend (CATCHCHALLENGER_DB_INTERNAL_VARS). All this code has
// to do is bring the network up, then hand control to the normal server loop.
//
//   1. init NVS (the wifi driver stores calibration there)
//   2. bring up wifi STA + the DHCP client (esp_netif), block until we get an IP
//   3. call the server entry (the CLI main(), compiled here as
//      catchchallenger_cli_main — see main/CMakeLists.txt). It binds the listen
//      socket via lwIP's BSD sockets + select(2) and runs forever.
//
// The server itself, once it binds, periodically emits the LAN-announce UDP
// broadcast (server-properties <broadcastName>, baked into the flash cache) so
// LAN clients running the normal client auto-discover this ESP32 server.
//
// SSID / password come from menuconfig (Kconfig.projbuild). See test/ESP32.md.

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_system.h"
#include "sdkconfig.h"

static const char *TAG = "catchchallenger";
static EventGroupHandle_t s_wifi_eg;
static const int WIFI_CONNECTED_BIT = BIT0;

// Provided by the server CLI (server/cli/main-unix.cpp), compiled with
// -Dmain=catchchallenger_cli_main so its int main(argc,argv) is reachable here.
// NOT extern "C": the renamed function keeps C++ linkage, so this declaration
// must match (same mangled name).
int catchchallenger_cli_main(int argc, char **argv);

static void cc_wifi_event_handler(void *, esp_event_base_t base,
                                  int32_t id, void *event_data)
{
    if(base==WIFI_EVENT && id==WIFI_EVENT_STA_START)
        esp_wifi_connect();
    else if(base==WIFI_EVENT && id==WIFI_EVENT_STA_DISCONNECTED)
    {
        ESP_LOGW(TAG, "wifi disconnected, retrying");
        esp_wifi_connect();
    }
    else if(base==IP_EVENT && id==IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *e=(ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "DHCP got IP " IPSTR, IP2STR(&e->ip_info.ip));
        xEventGroupSetBits(s_wifi_eg, WIFI_CONNECTED_BIT);
    }
}

static void cc_wifi_init_sta(void)
{
    s_wifi_eg = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    // The default STA netif owns the DHCP client; it requests an address as soon
    // as the link is up — no static config needed.
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID, &cc_wifi_event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT, IP_EVENT_STA_GOT_IP, &cc_wifi_event_handler, NULL, NULL));

    wifi_config_t wc;
    memset(&wc, 0, sizeof(wc));
    strncpy((char *)wc.sta.ssid, CONFIG_CATCHCHALLENGER_WIFI_SSID,
            sizeof(wc.sta.ssid) - 1);
    strncpy((char *)wc.sta.password, CONFIG_CATCHCHALLENGER_WIFI_PASSWORD,
            sizeof(wc.sta.password) - 1);

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wc));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi STA connecting to \"%s\" (DHCP)...",
             CONFIG_CATCHCHALLENGER_WIFI_SSID);
    xEventGroupWaitBits(s_wifi_eg, WIFI_CONNECTED_BIT, pdFALSE, pdTRUE,
                        portMAX_DELAY);
}

extern "C" void app_main(void)
{
    esp_err_t err = nvs_flash_init();
    if(err==ESP_ERR_NVS_NO_FREE_PAGES || err==ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }
    cc_wifi_init_sta();   // blocks until DHCP gives us an address

    ESP_LOGI(TAG, "network up — starting CatchChallenger server "
                  "(datapack + settings in flash, no filesystem)");
    char arg0[] = "catchchallenger";
    char *argv[] = { arg0, NULL };
    catchchallenger_cli_main(1, argv);   // binds + runs the event loop forever

    ESP_LOGE(TAG, "server loop returned unexpectedly — restarting");
    esp_restart();
}
