
#include "esp_display_panel.hpp"

using namespace esp_panel;

extern "C"  void app_main(void)
{
    auto panel = new board::Board();
    panel->init();
    panel->begin();

    panel->getBacklight()->on();

    auto touch = panel->getTouch();

    ESP_LOGI("MAIN", "Panel ready");
}


