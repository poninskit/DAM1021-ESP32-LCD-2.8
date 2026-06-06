idf_component_register(
                        SRCS  "main.c"
                              "EXIO/TCA9554PWR.c"
                              "Buzzer/Buzzer.c"
                              "LCD_Driver/ST7701S.c"
                              "Touch_Driver/GT911.c"
                              "Touch_Driver/esp_lcd_touch/esp_lcd_touch.c"
                              "LVGL_Driver/LVGL_Driver.c"
                              "I2C_Driver/I2C_Driver.c"

                        INCLUDE_DIRS "./EXIO"
                                     "./Buzzer"
                                     "./LCD_Driver"
                                     "./Touch_Driver"
                                     "./Touch_Driver/esp_lcd_touch"
                                     "./LVGL_Driver"
                                     "./I2C_Driver"
                                     "."
                        )
