#ifndef CO2_Gadget_Thresholds_h
#define CO2_Gadget_Thresholds_h

#include <Preferences.h>
#include <ArduinoJson.h>

// Enum for output types
enum OutputType {
    DISPLAY_SHOW,
    BLE_SEND,
    MQTT_SEND,
    ESPNOW_SEND,
    NUM_OUTPUTS  // Number of output types
};

// Structure to hold configuration settings for each output type
struct ThresholdConfig {
    bool enabled;
    bool useOnlyInLowPower;
    uint16_t keepAlive;
    uint16_t co2ThresholdAbsolute;
    float tempThresholdAbsolute;
    float humThresholdAbsolute;
    uint16_t co2ThresholdPercentage;
    float tempThresholdPercentage;
    float humThresholdPercentage;
    uint16_t previousCO2Value;
    float previousTemperatureValue;
    float previousHumidityValue;
    uint64_t lastUpdateSeconds;
    bool co2CombineWithAnd;
    bool tempCombineWithAnd;
    bool humCombineWithAnd;
};

void printThresholdsFromNVR();

class ThresholdManager {
   private:
    ThresholdConfig thresholds[NUM_OUTPUTS];

   public:
    ThresholdManager();
    void setThresholds(OutputType outputType, bool enabled, bool useOnlyInLowPower, uint16_t keepAlive, uint16_t co2ThresholdAbsolute, float tempThresholdAbsolute, float humThresholdAbsolute, uint16_t co2ThresholdPercentage, float tempThresholdPercentage, float humThresholdPercentage, bool co2CombineWithAnd, bool tempCombineWithAnd, bool humCombineWithAnd);
    ThresholdConfig getThresholds(OutputType outputType);
    void printThresholdsDifferences(OutputType outputType);
    void printThresholdEvaluation(OutputType outputType, uint16_t co2, float temp, float hum, bool isLowPowerMode = false, bool forceUpdate = false);
    void updatePreviousValues(OutputType outputType, uint16_t co2, float temp, float hum);
    bool checkAndMaybeUpdateThresholds(OutputType outputType, uint16_t co2, float temp, float hum, bool updateOnPass = true);
    bool evaluateThresholds(OutputType outputType, uint16_t co2, float temp, float hum, bool isLowPowerMode = false, bool updateOnPass = true);
    String getThresholdsAsJson(OutputType outputType);
    String getAllThresholdsAsJson();
    bool saveThresholdsToNVR();
    bool setThresholdsFromJSON(String response);
    bool setThresholdsFromJSON(JsonVariant json);
    void loadThresholdsFromNVR();
};

#endif // CO2_Gadget_Thresholds_h
