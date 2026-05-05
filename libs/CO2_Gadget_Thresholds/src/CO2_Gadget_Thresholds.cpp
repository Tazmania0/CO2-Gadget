#include "CO2_Gadget_Thresholds.h"

#ifndef RTC_DATA_ATTR
#define RTC_DATA_ATTR
#endif

extern uint64_t getReliableUptimeSeconds();

struct ThresholdRuntimeState {
    uint16_t previousCO2Value;
    float previousTemperatureValue;
    float previousHumidityValue;
    uint64_t lastUpdateSeconds;
};

static const uint32_t THRESHOLD_RUNTIME_MAGIC = 0xC02A7E51;
static RTC_DATA_ATTR uint32_t thresholdRuntimeMagic = 0;
static RTC_DATA_ATTR ThresholdRuntimeState thresholdRuntime[NUM_OUTPUTS];

static uint64_t thresholdNowSeconds() {
    return getReliableUptimeSeconds();
}

static bool combineThresholdResult(bool absoluteEnabled, bool absoluteExceeded, bool percentageEnabled, bool percentageExceeded, bool combineWithAnd) {
    if (!absoluteEnabled && !percentageEnabled) return false;
    if (absoluteEnabled && percentageEnabled) {
        return combineWithAnd ? (absoluteExceeded && percentageExceeded) : (absoluteExceeded || percentageExceeded);
    }
    return absoluteEnabled ? absoluteExceeded : percentageExceeded;
}

static const char* thresholdOutputName(OutputType outputType) {
    switch (outputType) {
        case DISPLAY_SHOW:
            return "DISPLAY";
        case BLE_SEND:
            return "BLE";
        case MQTT_SEND:
            return "MQTT";
        case ESPNOW_SEND:
            return "ESPNOW";
        default:
            return "UNKNOWN";
    }
}

static void copyConfigToRuntime(OutputType outputType, ThresholdConfig& config) {
    thresholdRuntime[outputType].previousCO2Value = config.previousCO2Value;
    thresholdRuntime[outputType].previousTemperatureValue = config.previousTemperatureValue;
    thresholdRuntime[outputType].previousHumidityValue = config.previousHumidityValue;
    thresholdRuntime[outputType].lastUpdateSeconds = config.lastUpdateSeconds;
    thresholdRuntimeMagic = THRESHOLD_RUNTIME_MAGIC;
}

static void copyRuntimeToConfig(OutputType outputType, ThresholdConfig& config) {
    config.previousCO2Value = thresholdRuntime[outputType].previousCO2Value;
    config.previousTemperatureValue = thresholdRuntime[outputType].previousTemperatureValue;
    config.previousHumidityValue = thresholdRuntime[outputType].previousHumidityValue;
    config.lastUpdateSeconds = thresholdRuntime[outputType].lastUpdateSeconds;
}

// Class to manage sensor thresholds
ThresholdManager::ThresholdManager() {
    loadThresholdsFromNVR();
#ifdef DEBUG_THRESHOLDS
    Serial.print("-->[TRESH] Thresholds loaded from NVRAM\t: ");
    printThresholdsFromNVR();
#endif
}

void ThresholdManager::setThresholds(OutputType outputType, bool enabled, bool useOnlyInLowPower, uint16_t keepAlive, uint16_t co2ThresholdAbsolute, float tempThresholdAbsolute, float humThresholdAbsolute, uint16_t co2ThresholdPercentage, float tempThresholdPercentage, float humThresholdPercentage, bool co2CombineWithAnd, bool tempCombineWithAnd, bool humCombineWithAnd) {
    thresholds[outputType].enabled = enabled;
    thresholds[outputType].useOnlyInLowPower = useOnlyInLowPower;
    thresholds[outputType].keepAlive = keepAlive;
    thresholds[outputType].co2ThresholdAbsolute = co2ThresholdAbsolute;
    thresholds[outputType].tempThresholdAbsolute = tempThresholdAbsolute;
    thresholds[outputType].humThresholdAbsolute = humThresholdAbsolute;
    thresholds[outputType].co2ThresholdPercentage = co2ThresholdPercentage;
    thresholds[outputType].tempThresholdPercentage = tempThresholdPercentage;
    thresholds[outputType].humThresholdPercentage = humThresholdPercentage;
    thresholds[outputType].previousCO2Value = 0;
    thresholds[outputType].previousTemperatureValue = 0.0f;
    thresholds[outputType].previousHumidityValue = 0.0f;
    thresholds[outputType].lastUpdateSeconds = 0;
    thresholds[outputType].co2CombineWithAnd = co2CombineWithAnd;
    thresholds[outputType].tempCombineWithAnd = tempCombineWithAnd;
    thresholds[outputType].humCombineWithAnd = humCombineWithAnd;
    copyConfigToRuntime(outputType, thresholds[outputType]);
#ifdef DEBUG_THRESHOLDS
    Serial.println("-->[TRESH] Thresholds set for output type: " + String(outputType));
    Serial.println("-->[TRESH] Setting enabled: " + String(enabled));
    Serial.println("-->[TRESH] Setting useOnlyInLowPower: " + String(useOnlyInLowPower));
    Serial.println("-->[TRESH] Setting keepAlive: " + String(keepAlive));
    Serial.println("-->[TRESH] Setting co2ThresholdAbsolute: " + String(co2ThresholdAbsolute));
    Serial.println("-->[TRESH] Setting tempThresholdAbsolute: " + String(tempThresholdAbsolute));
    Serial.println("-->[TRESH] Setting humThresholdAbsolute: " + String(humThresholdAbsolute));
    Serial.println("-->[TRESH] Setting co2ThresholdPercentage: " + String(co2ThresholdPercentage));
    Serial.println("-->[TRESH] Setting tempThresholdPercentage: " + String(tempThresholdPercentage));
    Serial.println("-->[TRESH] Setting humThresholdPercentage: " + String(humThresholdPercentage));
    Serial.println("-->[TRESH] Setting previousCO2Value: " + String(thresholds[outputType].previousCO2Value));
    Serial.println("-->[TRESH] Setting previousTemperatureValue: " + String(thresholds[outputType].previousTemperatureValue));
    Serial.println("-->[TRESH] Setting previousHumidityValue: " + String(thresholds[outputType].previousHumidityValue));
    Serial.println("-->[TRESH] Setting co2CombineWithAnd: " + String(thresholds[outputType].co2CombineWithAnd));
    Serial.println("-->[TRESH] Setting tempCombineWithAnd: " + String(thresholds[outputType].tempCombineWithAnd));
    Serial.println("-->[TRESH] Setting humCombineWithAnd: " + String(thresholds[outputType].humCombineWithAnd));
#endif
}

ThresholdConfig ThresholdManager::getThresholds(OutputType outputType) {
    return thresholds[outputType];
}

void ThresholdManager::printThresholdsDifferences(OutputType outputType) {
#ifdef DEBUG_THRESHOLDS
    // Function to print the differences of the thresholds
    Serial.println("-->[TRESH] Thresholds differences:");
    Serial.println("-->[TRESH] CO2: " + String(thresholds[outputType].co2ThresholdAbsolute) + " ppm or " + String(thresholds[outputType].co2ThresholdPercentage) + "%");
    Serial.println("-->[TRESH] Temperature: " + String(thresholds[outputType].tempThresholdAbsolute) + "°C or " + String(thresholds[outputType].tempThresholdPercentage) + "%");
    Serial.println("-->[TRESH] Humidity: " + String(thresholds[outputType].humThresholdAbsolute) + "%RH or " + String(thresholds[outputType].humThresholdPercentage) + "%");
#else
    (void)outputType;
#endif
}

void ThresholdManager::printThresholdEvaluation(OutputType outputType, uint16_t co2, float temp, float hum, bool isLowPowerMode, bool forceUpdate) {
#ifdef DEBUG_THRESHOLDS
    ThresholdConfig& config = thresholds[outputType];
    uint64_t nowSeconds = thresholdNowSeconds();
    uint64_t ageSeconds = (config.lastUpdateSeconds > 0) ? (nowSeconds - config.lastUpdateSeconds) : 0;
    bool activeForMode = config.enabled && (!config.useOnlyInLowPower || isLowPowerMode);

    uint16_t co2Delta = (co2 >= config.previousCO2Value) ? (co2 - config.previousCO2Value) : (config.previousCO2Value - co2);
    float tempDelta = (temp >= config.previousTemperatureValue) ? (temp - config.previousTemperatureValue) : (config.previousTemperatureValue - temp);
    float humDelta = (hum >= config.previousHumidityValue) ? (hum - config.previousHumidityValue) : (config.previousHumidityValue - hum);

    float co2DeltaPct = (config.previousCO2Value == 0) ? 0.0f : (static_cast<float>(co2Delta) * 100.0f / static_cast<float>(config.previousCO2Value));
    float tempDeltaPct = (config.previousTemperatureValue == 0.0f) ? 0.0f : (tempDelta * 100.0f / config.previousTemperatureValue);
    float humDeltaPct = (config.previousHumidityValue == 0.0f) ? 0.0f : (humDelta * 100.0f / config.previousHumidityValue);

    bool co2AbsoluteEnabled = config.co2ThresholdAbsolute > 0;
    bool tempAbsoluteEnabled = config.tempThresholdAbsolute > 0.0f;
    bool humAbsoluteEnabled = config.humThresholdAbsolute > 0.0f;
    bool co2PercentageEnabled = config.co2ThresholdPercentage > 0;
    bool tempPercentageEnabled = config.tempThresholdPercentage > 0.0f;
    bool humPercentageEnabled = config.humThresholdPercentage > 0.0f;

    bool co2AbsPass = co2AbsoluteEnabled && (co2Delta >= config.co2ThresholdAbsolute);
    bool tempAbsPass = tempAbsoluteEnabled && (tempDelta >= config.tempThresholdAbsolute);
    bool humAbsPass = humAbsoluteEnabled && (humDelta >= config.humThresholdAbsolute);
    bool co2PctPass = co2PercentageEnabled && (co2DeltaPct >= config.co2ThresholdPercentage);
    bool tempPctPass = tempPercentageEnabled && (tempDeltaPct >= config.tempThresholdPercentage);
    bool humPctPass = humPercentageEnabled && (humDeltaPct >= config.humThresholdPercentage);

    bool co2Pass = combineThresholdResult(co2AbsoluteEnabled, co2AbsPass, co2PercentageEnabled, co2PctPass, config.co2CombineWithAnd);
    bool tempPass = combineThresholdResult(tempAbsoluteEnabled, tempAbsPass, tempPercentageEnabled, tempPctPass, config.tempCombineWithAnd);
    bool humPass = combineThresholdResult(humAbsoluteEnabled, humAbsPass, humPercentageEnabled, humPctPass, config.humCombineWithAnd);
    bool firstSample = (config.previousCO2Value == 0) || (config.lastUpdateSeconds == 0);
    bool keepAlivePass = (config.keepAlive > 0) && (config.lastUpdateSeconds > 0) && (ageSeconds >= config.keepAlive);
    bool valuePass = co2Pass || tempPass || humPass;
    bool decision = forceUpdate || !activeForMode || firstSample || keepAlivePass || valuePass;

    Serial.println("-->[TRESH][" + String(thresholdOutputName(outputType)) + "] enabled=" + String(config.enabled) +
                   " onlyLowPower=" + String(config.useOnlyInLowPower) +
                   " inLowPower=" + String(isLowPowerMode) +
                   " activeForMode=" + String(activeForMode) +
                   " force=" + String(forceUpdate));
    Serial.println("-->[TRESH][" + String(thresholdOutputName(outputType)) + "] keepAlive=" + String(config.keepAlive) +
                   "s lastUpdate=" + String(static_cast<unsigned long>(config.lastUpdateSeconds)) +
                   "s age=" + String(static_cast<unsigned long>(ageSeconds)) +
                   "s pass=" + String(keepAlivePass));
    Serial.println("-->[TRESH][" + String(thresholdOutputName(outputType)) + "] CO2 prev=" + String(config.previousCO2Value) +
                   " now=" + String(co2) +
                   " delta=" + String(co2Delta) +
                   " absThr=" + String(config.co2ThresholdAbsolute) +
                   " absPass=" + String(co2AbsPass) +
                   " pct=" + String(co2DeltaPct, 1) +
                   "% pctThr=" + String(config.co2ThresholdPercentage) +
                   "% pctPass=" + String(co2PctPass) +
                   " combine=" + String(config.co2CombineWithAnd ? "AND" : "OR") +
                   " pass=" + String(co2Pass));
    Serial.println("-->[TRESH][" + String(thresholdOutputName(outputType)) + "] TEMP prev=" + String(config.previousTemperatureValue, 2) +
                   " now=" + String(temp, 2) +
                   " delta=" + String(tempDelta, 2) +
                   " absThr=" + String(config.tempThresholdAbsolute, 2) +
                   " absPass=" + String(tempAbsPass) +
                   " pct=" + String(tempDeltaPct, 1) +
                   "% pctThr=" + String(config.tempThresholdPercentage, 1) +
                   "% pctPass=" + String(tempPctPass) +
                   " combine=" + String(config.tempCombineWithAnd ? "AND" : "OR") +
                   " pass=" + String(tempPass));
    Serial.println("-->[TRESH][" + String(thresholdOutputName(outputType)) + "] HUM prev=" + String(config.previousHumidityValue, 2) +
                   " now=" + String(hum, 2) +
                   " delta=" + String(humDelta, 2) +
                   " absThr=" + String(config.humThresholdAbsolute, 2) +
                   " absPass=" + String(humAbsPass) +
                   " pct=" + String(humDeltaPct, 1) +
                   "% pctThr=" + String(config.humThresholdPercentage, 1) +
                   "% pctPass=" + String(humPctPass) +
                   " combine=" + String(config.humCombineWithAnd ? "AND" : "OR") +
                   " pass=" + String(humPass));
    Serial.println("-->[TRESH][" + String(thresholdOutputName(outputType)) + "] decision=" + String(decision ? "PASS" : "SKIP") +
                   " firstSample=" + String(firstSample) +
                   " keepAlivePass=" + String(keepAlivePass) +
                   " valuePass=" + String(valuePass));
#else
    (void)outputType;
    (void)co2;
    (void)temp;
    (void)hum;
    (void)isLowPowerMode;
    (void)forceUpdate;
#endif
}

void ThresholdManager::updatePreviousValues(OutputType outputType, uint16_t co2, float temp, float hum) {
    // Update the last measured values
    thresholds[outputType].previousCO2Value = co2;
    thresholds[outputType].previousTemperatureValue = temp;
    thresholds[outputType].previousHumidityValue = hum;
    thresholds[outputType].lastUpdateSeconds = thresholdNowSeconds();
    copyConfigToRuntime(outputType, thresholds[outputType]);
}

/**
 * Checks if the given CO2, temperature, and humidity values exceed the thresholds
 * for the specified output type. If the thresholds are exceeded, the function updates
 * the previous values and returns true. Otherwise, it returns false.
 *
 * @param outputType The output type to check against the thresholds.
 * @param co2 The CO2 value to check against the thresholds.
 * @param temp The temperature value to check against the thresholds.
 * @param hum The humidity value to check against the thresholds.
 *
 * @return True if the thresholds are exceeded, false otherwise.
 */
bool ThresholdManager::checkAndMaybeUpdateThresholds(OutputType outputType, uint16_t co2, float temp, float hum, bool updateOnPass) {
    ThresholdConfig& config = thresholds[outputType];

    if (!config.enabled) {
        // Thresholds are not enabled for this output type
        return false;
    }

    uint64_t nowSeconds = thresholdNowSeconds();

    if ((config.keepAlive > 0) && (config.lastUpdateSeconds > 0) && ((nowSeconds - config.lastUpdateSeconds) >= config.keepAlive)) {
        if (updateOnPass) updatePreviousValues(outputType, co2, temp, hum);
        return true;
    }

    if ((config.previousCO2Value == 0) || (config.lastUpdateSeconds == 0)) {
        if (updateOnPass) updatePreviousValues(outputType, co2, temp, hum);
        return true;
    }

    // Check absolute thresholds
    bool co2AbsoluteEnabled = config.co2ThresholdAbsolute > 0;
    bool tempAbsoluteEnabled = config.tempThresholdAbsolute > 0.0f;
    bool humAbsoluteEnabled = config.humThresholdAbsolute > 0.0f;
    bool co2ExceedsAbsolute = co2AbsoluteEnabled && (abs(co2 - config.previousCO2Value) >= config.co2ThresholdAbsolute);
    bool tempExceedsAbsolute = tempAbsoluteEnabled && (abs(temp - config.previousTemperatureValue) >= config.tempThresholdAbsolute);
    bool humExceedsAbsolute = humAbsoluteEnabled && (abs(hum - config.previousHumidityValue) >= config.humThresholdAbsolute);
#ifdef DEBUG_THRESHOLDS
    if (co2ExceedsAbsolute) {
        Serial.println("-->[TRESH] CO2 exceeds absolute threshold. Old value: " + String(config.previousCO2Value) + ", New value: " + String(co2) + ", Difference: " + String(abs(co2 - config.previousCO2Value)));
    }
#endif

    // Check percentage thresholds (avoid division by zero)
    bool co2PercentageEnabled = config.co2ThresholdPercentage > 0;
    bool tempPercentageEnabled = config.tempThresholdPercentage > 0.0f;
    bool humPercentageEnabled = config.humThresholdPercentage > 0.0f;
    bool co2ExceedsPercentage = co2PercentageEnabled && (abs(co2 - config.previousCO2Value) >= (config.previousCO2Value * config.co2ThresholdPercentage / 100));
    bool tempExceedsPercentage = tempPercentageEnabled && (abs(temp - config.previousTemperatureValue) >= (config.previousTemperatureValue * config.tempThresholdPercentage / 100));
    bool humExceedsPercentage = humPercentageEnabled && (abs(hum - config.previousHumidityValue) >= (config.previousHumidityValue * config.humThresholdPercentage / 100));
#ifdef DEBUG_THRESHOLDS
    if (co2ExceedsPercentage) {
        Serial.println("-->[TRESH] CO2 exceeds percentage threshold. Old value: " + String(config.previousCO2Value) + ", New value: " + String(co2) + ", Difference (%): " + String(config.previousCO2Value != 0 ? (abs(co2 - config.previousCO2Value) * 100 / config.previousCO2Value) : 0));

    }
#endif

    // Check if both absolute and percentage thresholds are exceeded (AND or OR combination)
    bool co2ThresholdsExceeded = combineThresholdResult(co2AbsoluteEnabled, co2ExceedsAbsolute, co2PercentageEnabled, co2ExceedsPercentage, config.co2CombineWithAnd);
        bool tempThresholdsExceeded = combineThresholdResult(tempAbsoluteEnabled, tempExceedsAbsolute, tempPercentageEnabled, tempExceedsPercentage, config.tempCombineWithAnd);
        bool humThresholdsExceeded = combineThresholdResult(humAbsoluteEnabled, humExceedsAbsolute, humPercentageEnabled, humExceedsPercentage, config.humCombineWithAnd);
    #ifdef DEBUG_THRESHOLDS
        if (co2ThresholdsExceeded) {
            // If co2CombineWithAnd = true append AND otherwise append OR
            Serial.println("-->[TRESH] CO2 threshold combination meeted evaluating with: " + String(config.co2CombineWithAnd ? "AND" : "OR"));
        } else {
            if (co2ExceedsAbsolute) Serial.println("-->[TRESH] CO2 threshold combination not meeted evaluating with: " + String(config.co2CombineWithAnd ? "AND" : "OR"));
        }
    #endif

        // Check if any threshold is exceeded
        if (co2ThresholdsExceeded || tempThresholdsExceeded || humThresholdsExceeded) {
            // Update previous values
            if (updateOnPass) updatePreviousValues(outputType, co2, temp, hum);
            return true;  // Thresholds exceeded
        }

    return false;  // Thresholds not exceeded
}

/**
 * Evaluates if the sensor readings pass the thresholds for the given output type.
 *
 * @param outputType The output type to check against the thresholds.
 * @param co2 The CO2 value to check against the thresholds.
 * @param temp The temperature value to check against the thresholds.
 * @param hum The humidity value to check against the thresholds.
 *
 * @return True if the sensor readings pass the thresholds, false otherwise.
 */
bool ThresholdManager::evaluateThresholds(OutputType outputType, uint16_t co2, float temp, float hum, bool isLowPowerMode, bool updateOnPass) {
    // If threshold is not enabled for outputType, return true
    if (!thresholds[outputType].enabled) return true;
    if (thresholds[outputType].useOnlyInLowPower && !isLowPowerMode) return true;
    // Evaluate if the sensor readings pass the thresholds
    return checkAndMaybeUpdateThresholds(outputType, co2, temp, hum, updateOnPass);
}

/**
 * Returns the threshold data for the specified output type as a JSON string.
 *
 * @param outputType The output type for which to retrieve the threshold data.
 *
 * @return The threshold data as a JSON string.
 *
 * @throws None
 */
String ThresholdManager::getThresholdsAsJson(OutputType outputType) {
    const size_t capacity = JSON_ARRAY_SIZE(NUM_OUTPUTS) + NUM_OUTPUTS * JSON_OBJECT_SIZE(16);
    DynamicJsonDocument doc(capacity);

    doc["enabled"] = thresholds[outputType].enabled;
    doc["thrOnlyInLowP"] = thresholds[outputType].useOnlyInLowPower;
    doc["thrKeepAlive"] = thresholds[outputType].keepAlive;
    doc["thrCo2Abs"] = thresholds[outputType].co2ThresholdAbsolute;
    doc["thrTempAbs"] = thresholds[outputType].tempThresholdAbsolute;
    doc["thrHumAbs"] = thresholds[outputType].humThresholdAbsolute;
    doc["thrCo2Per"] = thresholds[outputType].co2ThresholdPercentage;
    doc["thrTempPer"] = thresholds[outputType].tempThresholdPercentage;
    doc["thrHumPer"] = thresholds[outputType].humThresholdPercentage;
    doc["prevCO2"] = thresholds[outputType].previousCO2Value;
    doc["prevTemp"] = thresholds[outputType].previousTemperatureValue;
    doc["prevHum"] = thresholds[outputType].previousHumidityValue;
    doc["lastUpdate"] = thresholds[outputType].lastUpdateSeconds;
    doc["thrCo2CombAnd"] = thresholds[outputType].co2CombineWithAnd;
    doc["thrTempCombAnd"] = thresholds[outputType].tempCombineWithAnd;
    doc["thrHumCombAnd"] = thresholds[outputType].humCombineWithAnd;

    String json;
    serializeJson(doc, json);
#ifdef DEBUG_THRESHOLDS
    Serial.print("-->[TRESH] Thresholds as JSON: ");
    Serial.println(json);
#endif
    return json;
}

/**
 * Method to return all threshold data as a JSON string.
 *
 * @param None
 *
 * @return The threshold data as a JSON string.
 *
 * @throws None
 */
String ThresholdManager::getAllThresholdsAsJson() {
    const size_t capacity = JSON_ARRAY_SIZE(NUM_OUTPUTS) + NUM_OUTPUTS * JSON_OBJECT_SIZE(16);
    DynamicJsonDocument doc(capacity);

    for (int i = 0; i < NUM_OUTPUTS; ++i) {
        JsonObject obj = doc.createNestedObject();
        obj["enabled"] = thresholds[i].enabled;
        obj["thrOnlyInLowP"] = thresholds[i].useOnlyInLowPower;
        obj["thrKeepAlive"] = thresholds[i].keepAlive;
        obj["thrCo2Abs"] = thresholds[i].co2ThresholdAbsolute;
        obj["thrTempAbs"] = thresholds[i].tempThresholdAbsolute;
        obj["thrHumAbs"] = thresholds[i].humThresholdAbsolute;
        obj["thrCo2Per"] = thresholds[i].co2ThresholdPercentage;
        obj["thrTempPer"] = thresholds[i].tempThresholdPercentage;
        obj["thrHumPer"] = thresholds[i].humThresholdPercentage;
        obj["prevCO2"] = thresholds[i].previousCO2Value;
        obj["prevTemp"] = thresholds[i].previousTemperatureValue;
        obj["prevHum"] = thresholds[i].previousHumidityValue;
        obj["lastUpdate"] = thresholds[i].lastUpdateSeconds;
        obj["thrCo2CombAnd"] = thresholds[i].co2CombineWithAnd;
        obj["thrTempCombAnd"] = thresholds[i].tempCombineWithAnd;
        obj["thrHumCombAnd"] = thresholds[i].humCombineWithAnd;
    }

    String json;
    serializeJson(doc, json);
#ifdef DEBUG_THRESHOLDS
    Serial.print("-->[TRESH] All thresholds as JSON: ");
    Serial.println(json);
#endif
    return json;
}

/**
 * Saves the thresholds to non-volatile memory (NVRAM).
 *
 * This function opens the NVRAM with the key "thresholds" and saves the thresholds for each output.
 * The thresholds are stored as key-value pairs in the NVRAM. The keys are constructed by appending
 * "_enabled", "_OnlyInLowP", "_KeepAlive", "_Co2Abs", "_TempAbs", "_HumAbs", "_Co2Per", "_TempPer",
 * "_HumPer", "_Co2CombAnd", "_TempCombAnd", and "_HumCombAnd" to
 * the output index.
 *
 * @throws None
 */
bool ThresholdManager::saveThresholdsToNVR() {
    Preferences thresholdsPrefs;
    if (!thresholdsPrefs.begin("thresholds", false)) {  // Open NVRAM with THRESHOLD_PREF_KEY key
#ifdef DEBUG_THRESHOLDS
        Serial.println("-->[TRESH] Failed to open thresholds NVRAM namespace for writing");
#endif
        return false;
    }

    bool success = true;

    for (int i = 0; i < NUM_OUTPUTS; ++i) {
        String key = String(i);
        success &= thresholdsPrefs.putBool((key + "_enabled").c_str(), thresholds[i].enabled) > 0;
        success &= thresholdsPrefs.putBool((key + "_OnlyInLowP").c_str(), thresholds[i].useOnlyInLowPower) > 0;
        success &= thresholdsPrefs.putUShort((key + "_KeepAlive").c_str(), thresholds[i].keepAlive) > 0;
        success &= thresholdsPrefs.putUShort((key + "_Co2Abs").c_str(), thresholds[i].co2ThresholdAbsolute) > 0;
        success &= thresholdsPrefs.putFloat((key + "_TempAbs").c_str(), thresholds[i].tempThresholdAbsolute) > 0;
        success &= thresholdsPrefs.putFloat((key + "_HumAbs").c_str(), thresholds[i].humThresholdAbsolute) > 0;
        success &= thresholdsPrefs.putUShort((key + "_Co2Per").c_str(), thresholds[i].co2ThresholdPercentage) > 0;
        success &= thresholdsPrefs.putFloat((key + "_TempPer").c_str(), thresholds[i].tempThresholdPercentage) > 0;
        success &= thresholdsPrefs.putFloat((key + "_HumPer").c_str(), thresholds[i].humThresholdPercentage) > 0;
        success &= thresholdsPrefs.putBool((key + "_Co2CombAnd").c_str(), thresholds[i].co2CombineWithAnd) > 0;
        success &= thresholdsPrefs.putBool((key + "_TempCombAnd").c_str(), thresholds[i].tempCombineWithAnd) > 0;
        success &= thresholdsPrefs.putBool((key + "_HumCombAnd").c_str(), thresholds[i].humCombineWithAnd) > 0;
    }
    thresholdsPrefs.end();  // Finish using NVRAM
    return success;
}

/**
 * Sets the thresholds for the ThresholdManager object from a JSON string.
 *
 * @param response The JSON string containing the thresholds.
 * @returns None
 * @example {"thresholds":[{"enabled":"on","thrOnlyInLowP":false,"thrKeepAlive":"5","thrCo2Abs":"20","thrTempAbs":"0.5","thrHumAbs":"1","thrCo2Per":"4","thrTempPer":"1","thrHumPer":"1","thrCo2CombAnd":true,"thrTempCombAnd":true,"thrHumCombAnd":true},{"enabled":"on","thrOnlyInLowP":false,"thrKeepAlive":"0","thrCo2Abs":"20","thrTempAbs":"0.5","thrHumAbs":"1","thrCo2Per":"5","thrTempPer":"1","thrHumPer":"1","thrCo2CombAnd":false,"thrTempCombAnd":false,"thrHumCombAnd":false},{"enabled":"on","thrOnlyInLowP":false,"thrKeepAlive":"0","thrCo2Abs":"20","thrTempAbs":"0.5","thrHumAbs":"1","thrCo2Per":"5","thrTempPer":"1","thrHumPer":"1","thrCo2CombAnd":false,"thrTempCombAnd":false,"thrHumCombAnd":false},{"enabled":false,"thrOnlyInLowP":false,"thrKeepAlive":"0","thrCo2Abs":"20","thrTempAbs":"0.5","thrHumAbs":"1","thrCo2Per":"5","thrTempPer":"1","thrHumPer":"1","thrCo2CombAnd":false,"thrTempCombAnd":false,"thrHumCombAnd":false}]}
 *
 * @throws None
 */
bool ThresholdManager::setThresholdsFromJSON(String response) {
    const size_t capacity = 4096;
    DynamicJsonDocument doc(capacity);
    DeserializationError error = deserializeJson(doc, response);
    if (error) {
#ifdef DEBUG_THRESHOLDS
        Serial.println("-->[TRESH] Failed to parse thresholds JSON: " + String(error.c_str()));
#endif
        return false;
    }
    return setThresholdsFromJSON(doc.as<JsonVariant>());
}

bool ThresholdManager::setThresholdsFromJSON(JsonVariant json) {
    if (!json["thresholds"].is<JsonArray>()) {
#ifdef DEBUG_THRESHOLDS
        Serial.println("-->[TRESH] Invalid thresholds JSON: missing thresholds array");
#endif
        return false;
    }

    JsonArray thresholdDocs = json["thresholds"].as<JsonArray>();

    for (int i = 0; i < NUM_OUTPUTS; ++i) {
        if (i >= thresholdDocs.size()) continue;
        JsonObject thresholdDoc = thresholdDocs[i].as<JsonObject>();
        if (thresholdDoc.containsKey("enabled")) thresholds[i].enabled = thresholdDoc["enabled"].as<bool>();
        if (thresholdDoc.containsKey("thrOnlyInLowP")) thresholds[i].useOnlyInLowPower = thresholdDoc["thrOnlyInLowP"].as<bool>();
        if (thresholdDoc.containsKey("thrKeepAlive")) thresholds[i].keepAlive = thresholdDoc["thrKeepAlive"].as<uint16_t>();
        if (thresholdDoc.containsKey("thrCo2Abs")) thresholds[i].co2ThresholdAbsolute = thresholdDoc["thrCo2Abs"].as<uint16_t>();
        if (thresholdDoc.containsKey("thrTempAbs")) thresholds[i].tempThresholdAbsolute = thresholdDoc["thrTempAbs"].as<float>();
        if (thresholdDoc.containsKey("thrHumAbs")) thresholds[i].humThresholdAbsolute = thresholdDoc["thrHumAbs"].as<float>();
        if (thresholdDoc.containsKey("thrCo2Per")) thresholds[i].co2ThresholdPercentage = thresholdDoc["thrCo2Per"].as<uint16_t>();
        if (thresholdDoc.containsKey("thrTempPer")) thresholds[i].tempThresholdPercentage = thresholdDoc["thrTempPer"].as<float>();
        if (thresholdDoc.containsKey("thrHumPer")) thresholds[i].humThresholdPercentage = thresholdDoc["thrHumPer"].as<float>();
        if (thresholdDoc.containsKey("thrCo2CombAnd")) thresholds[i].co2CombineWithAnd = thresholdDoc["thrCo2CombAnd"].as<bool>();
        if (thresholdDoc.containsKey("thrTempCombAnd")) thresholds[i].tempCombineWithAnd = thresholdDoc["thrTempCombAnd"].as<bool>();
        if (thresholdDoc.containsKey("thrHumCombAnd")) thresholds[i].humCombineWithAnd = thresholdDoc["thrHumCombAnd"].as<bool>();
        thresholds[i].previousCO2Value = 0;
        thresholds[i].previousTemperatureValue = 0.0f;
        thresholds[i].previousHumidityValue = 0.0f;
        thresholds[i].lastUpdateSeconds = 0;
        copyConfigToRuntime(static_cast<OutputType>(i), thresholds[i]);
#ifdef DEBUG_THRESHOLDS
        Serial.println("-->[TRESH][" + String(thresholdOutputName(static_cast<OutputType>(i))) + "] saved enabled=" + String(thresholds[i].enabled) +
                       " onlyLowPower=" + String(thresholds[i].useOnlyInLowPower) +
                       " keepAlive=" + String(thresholds[i].keepAlive) +
                       " co2Abs=" + String(thresholds[i].co2ThresholdAbsolute) +
                       " co2Pct=" + String(thresholds[i].co2ThresholdPercentage) +
                       " tempAbs=" + String(thresholds[i].tempThresholdAbsolute, 2) +
                       " tempPct=" + String(thresholds[i].tempThresholdPercentage, 2) +
                       " humAbs=" + String(thresholds[i].humThresholdAbsolute, 2) +
                       " humPct=" + String(thresholds[i].humThresholdPercentage, 2));
#endif
    }
    return saveThresholdsToNVR();
}

/**
 * Loads the thresholds from the non-volatile memory (NVRAM) into the ThresholdManager object.
 *
 * @throws None
 */
void ThresholdManager::loadThresholdsFromNVR() {
    Preferences thresholdsPrefs;
    thresholdsPrefs.begin("thresholds", true);  // Open NVRAM with THRESHOLD_PREF_KEY key
    bool runtimeValid = thresholdRuntimeMagic == THRESHOLD_RUNTIME_MAGIC;
    uint64_t nowSeconds = thresholdNowSeconds();

    for (int i = 0; i < NUM_OUTPUTS; ++i) {
        String key = String(i);
        // Method to load thresholds from NVRAM
        thresholds[i].enabled = thresholdsPrefs.getBool((key + "_enabled").c_str(), false);
        thresholds[i].useOnlyInLowPower = thresholdsPrefs.getBool((key + "_OnlyInLowP").c_str(), true);
        thresholds[i].keepAlive = thresholdsPrefs.getUShort((key + "_KeepAlive").c_str(), 5);
        thresholds[i].co2ThresholdAbsolute = thresholdsPrefs.getUShort((key + "_Co2Abs").c_str(), 20);
        thresholds[i].tempThresholdAbsolute = thresholdsPrefs.getFloat((key + "_TempAbs").c_str(), 0.5f);
        thresholds[i].humThresholdAbsolute = thresholdsPrefs.getFloat((key + "_HumAbs").c_str(), 1.0f);
        thresholds[i].co2ThresholdPercentage = thresholdsPrefs.getUShort((key + "_Co2Per").c_str(), 5);
        thresholds[i].tempThresholdPercentage = thresholdsPrefs.getFloat((key + "_TempPer").c_str(), 1.0f);
        thresholds[i].humThresholdPercentage = thresholdsPrefs.getFloat((key + "_HumPer").c_str(), 1.0f);
        thresholds[i].previousCO2Value = 0;
        thresholds[i].previousTemperatureValue = 0.0f;
        thresholds[i].previousHumidityValue = 0.0f;
        thresholds[i].lastUpdateSeconds = 0;
        thresholds[i].co2CombineWithAnd = thresholdsPrefs.getBool((key + "_Co2CombAnd").c_str(), false);
        thresholds[i].tempCombineWithAnd = thresholdsPrefs.getBool((key + "_TempCombAnd").c_str(), false);
        thresholds[i].humCombineWithAnd = thresholdsPrefs.getBool((key + "_HumCombAnd").c_str(), false);
        if (runtimeValid && (thresholdRuntime[i].lastUpdateSeconds <= nowSeconds)) {
            copyRuntimeToConfig(static_cast<OutputType>(i), thresholds[i]);
        } else {
            thresholds[i].lastUpdateSeconds = 0;
            copyConfigToRuntime(static_cast<OutputType>(i), thresholds[i]);
        }
    }
    thresholdsPrefs.end();  // Finish using NVRAM
}

/**
 * Retrieves and prints thresholds stored in non-volatile memory.
 *
 * @param None
 *
 * @return None
 *
 * @throws None
 */
void printThresholdsFromNVR() {
#ifdef DEBUG_THRESHOLDS
    Preferences thresholdsPrefs;
    thresholdsPrefs.begin("thresholds", true);  // Open NVRAM with THRESHOLD_PREF_KEY key
    Serial.println("-->[TRESH] Thresholds in NVRAM");
    for (int i = 0; i < NUM_OUTPUTS; ++i) {
        String key = String(i);
        Serial.println("-->[TRESH] Threshold " + String(i) + " enabled: " + String(thresholdsPrefs.getBool((key + "_enabled").c_str(), false)));
        Serial.println("-->[TRESH] Threshold " + String(i) + " useOnlyInLowPower: " + String(thresholdsPrefs.getBool((key + "_OnlyInLowP").c_str(), true)));
        Serial.println("-->[TRESH] Threshold " + String(i) + " keepAlive: " + String(thresholdsPrefs.getUShort((key + "_KeepAlive").c_str(), 5)));
        Serial.println("-->[TRESH] Threshold " + String(i) + " co2ThresholdAbsolute: " + String(thresholdsPrefs.getUShort((key + "_Co2Abs").c_str(), 20)));
        Serial.println("-->[TRESH] Threshold " + String(i) + " tempThresholdAbsolute: " + String(thresholdsPrefs.getFloat((key + "_TempAbs").c_str(), 0.5f)));
        Serial.println("-->[TRESH] Threshold " + String(i) + " humThresholdAbsolute: " + String(thresholdsPrefs.getFloat((key + "_HumAbs").c_str(), 1.0f)));
        Serial.println("-->[TRESH] Threshold " + String(i) + " co2ThresholdPercentage: " + String(thresholdsPrefs.getUShort((key + "_Co2Per").c_str(), 5)));
        Serial.println("-->[TRESH] Threshold " + String(i) + " tempThresholdPercentage: " + String(thresholdsPrefs.getFloat((key + "_TempPer").c_str(), 1.0f)));
        Serial.println("-->[TRESH] Threshold " + String(i) + " humThresholdPercentage: " + String(thresholdsPrefs.getFloat((key + "_HumPer").c_str(), 1.0f)));
        Serial.println("-->[TRESH] Threshold " + String(i) + " previousCO2Value: " + String(thresholdRuntimeMagic == THRESHOLD_RUNTIME_MAGIC ? thresholdRuntime[i].previousCO2Value : 0));
        Serial.println("-->[TRESH] Threshold " + String(i) + " previousTemperatureValue: " + String(thresholdRuntimeMagic == THRESHOLD_RUNTIME_MAGIC ? thresholdRuntime[i].previousTemperatureValue : 0.0f));
        Serial.println("-->[TRESH] Threshold " + String(i) + " previousHumidityValue: " + String(thresholdRuntimeMagic == THRESHOLD_RUNTIME_MAGIC ? thresholdRuntime[i].previousHumidityValue : 0.0f));
        Serial.println("-->[TRESH] Threshold " + String(i) + " lastUpdateSeconds: " + String(thresholdRuntimeMagic == THRESHOLD_RUNTIME_MAGIC ? thresholdRuntime[i].lastUpdateSeconds : 0));
        Serial.println("-->[TRESH] Threshold " + String(i) + " co2CombineWithAnd: " + String(thresholdsPrefs.getBool((key + "_Co2CombAnd").c_str(), false)));
        Serial.println("-->[TRESH] Threshold " + String(i) + " tempCombineWithAnd: " + String(thresholdsPrefs.getBool((key + "_TempCombAnd").c_str(), false)));
        Serial.println("-->[TRESH] Threshold " + String(i) + " humCombineWithAnd: " + String(thresholdsPrefs.getBool((key + "_HumCombAnd").c_str(), false)));
    }
    thresholdsPrefs.end();  // Finish using NVRAM
#endif
}
