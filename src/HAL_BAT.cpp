#if 0
#include "Adafruit_MAX1704X.h"

Adafruit_MAX17048 maxlipo;

void HAL::MAX_Init(void)
{
    while (!maxlipo.begin())
    {
        Serial.println(F("Couldnt find Adafruit MAX17048?\nMake sure a battery is plugged in!"));
        delay(2000);
    }
    Serial.print(F("Found MAX17048"));
    Serial.print(F(" with Chip ID: 0x"));
    Serial.println(maxlipo.getChipID(), HEX);
}

HAL::Power_Info_t HAL::MAXGetVoltage(void)
{
    Power_Info_t power_info = {0};
    power_info.voltage = maxlipo.cellVoltage();
    power_info.usage = maxlipo.cellPercent();

    if (isnan(power_info.voltage))
    {
        Serial.println("Failed to read cell voltage, check battery is connected!");
        delay(2000);
        return;
    }
    Serial.print(F("Batt Voltage: "));
    Serial.print(power_info.voltage, 3);
    Serial.println(" V");
    Serial.print(F("Batt Percent: "));
    Serial.print(power_info.usage, 1);
    Serial.println(" %");
    Serial.println();

    return power_info;
}
#endif