#include "sensor_manager.h"
#include "config.h"
#include <DHT.h>
#include <math.h>

static DHT dht(DHT_PIN, DHT_TYPE);

static float temperatureBuffer[DHT_MOVING_AVERAGE_WINDOW] = {};
static float humidityBuffer[DHT_MOVING_AVERAGE_WINDOW] = {};
static uint8_t dhtBufferIndex = 0;
static uint8_t dhtBufferCount = 0;
static float previousTemperature = NAN;
static float previousHumidity = NAN;
static uint8_t dhtErrorCount = 0;

static uint8_t soilErrorCount = 0;

static float roundToOneDecimal(float value) {
  return roundf(value * 10.0f) / 10.0f;
}

static const char* getDhtInvalidReason(float temperature, float humidity) {
  if (isnan(temperature) || isnan(humidity)) {
    return "raw read returned NaN";
  }

  if (temperature < DHT_TEMP_MIN_C || temperature > DHT_TEMP_MAX_C) {
    return "temperature out of range";
  }

  if (humidity < DHT_RH_MIN_PERCENT || humidity > DHT_RH_MAX_PERCENT) {
    return "humidity out of range";
  }

  if (!isnan(previousTemperature) &&
      fabsf(temperature - previousTemperature) > DHT_MAX_TEMP_STEP_C) {
    return "temperature step too large";
  }

  if (!isnan(previousHumidity) &&
      fabsf(humidity - previousHumidity) > DHT_MAX_RH_STEP_PERCENT) {
    return "humidity step too large";
  }

  return nullptr;
}

static void pushDhtSample(float temperature, float humidity) {
  temperatureBuffer[dhtBufferIndex] = temperature;
  humidityBuffer[dhtBufferIndex] = humidity;
  dhtBufferIndex = (dhtBufferIndex + 1) % DHT_MOVING_AVERAGE_WINDOW;

  if (dhtBufferCount < DHT_MOVING_AVERAGE_WINDOW) {
    ++dhtBufferCount;
  }
}

static float averageBuffer(const float* buffer, uint8_t count) {
  if (count == 0) {
    return NAN;
  }

  float sum = 0.0f;
  for (uint8_t i = 0; i < count; ++i) {
    sum += buffer[i];
  }

  return sum / static_cast<float>(count);
}

static bool isAdcValid(int adcRaw) {
  return adcRaw > ADC_MIN_VALID && adcRaw < ADC_MAX_VALID;
}

static void sortSamples(int* values, uint8_t count) {
  for (uint8_t i = 0; i < count; ++i) {
    for (uint8_t j = i + 1; j < count; ++j) {
      if (values[j] < values[i]) {
        const int temp = values[i];
        values[i] = values[j];
        values[j] = temp;
      }
    }
  }
}

static int medianFilterAdc(const int* samples, uint8_t count) {
  int sorted[SOIL_SAMPLE_COUNT] = {};

  for (uint8_t i = 0; i < count; ++i) {
    sorted[i] = samples[i];
  }

  sortSamples(sorted, count);

  if (count % 2 == 1) {
    return sorted[count / 2];
  }

  return (sorted[(count / 2) - 1] + sorted[count / 2]) / 2;
}

void sensorBegin() {
  dht.begin();
  analogReadResolution(12);
  analogSetPinAttenuation(SOIL_ADC_PIN, ADC_11db);
  delay(1000);
}

float calculateSoilPercent(int adcRaw) {
  float percent = 0.0f;

  if (ADC_DRY == ADC_WET) {
    return 0.0f;
  }

  if (ADC_DRY > ADC_WET) {
    percent = (static_cast<float>(ADC_DRY - adcRaw) * 100.0f) /
              static_cast<float>(ADC_DRY - ADC_WET);
  } else {
    percent = (static_cast<float>(adcRaw - ADC_DRY) * 100.0f) /
              static_cast<float>(ADC_WET - ADC_DRY);
  }

  return constrain(percent, 0.0f, 100.0f);
}

SensorData readSensors() {
  SensorData data{};
  data.timestamp = millis();

  float temperature = dht.readTemperature() + TEMP_OFFSET_C;
  float humidity = dht.readHumidity() + RH_OFFSET_PERCENT;

  const char* dhtInvalidReason = getDhtInvalidReason(temperature, humidity);
  data.dhtValid = dhtInvalidReason == nullptr;
  if (data.dhtValid) {
    dhtErrorCount = 0;
    humidity = constrain(humidity, 0.0f, 100.0f);
    pushDhtSample(temperature, humidity);
    data.temperature = roundToOneDecimal(averageBuffer(temperatureBuffer, dhtBufferCount));
    data.humidity = roundToOneDecimal(averageBuffer(humidityBuffer, dhtBufferCount));
    previousTemperature = data.temperature;
    previousHumidity = data.humidity;
  } else {
    ++dhtErrorCount;
    data.temperature = previousTemperature;
    data.humidity = previousHumidity;
    Serial.print(F("[SENSOR] DHT22 invalid: "));
    Serial.print(dhtInvalidReason);
    Serial.print(F(", rawTemp="));
    Serial.print(temperature);
    Serial.print(F(", rawRH="));
    Serial.println(humidity);

    if (!isnan(previousTemperature) &&
        !isnan(previousHumidity) &&
        dhtErrorCount <= DHT_MAX_ERROR_COUNT) {
      data.dhtValid = true;
      Serial.println(F("[SENSOR] DHT22 using last valid sample"));
    }
  }

  data.soilAdcRaw = analogRead(SOIL_ADC_PIN);
  data.soilValid = isAdcValid(data.soilAdcRaw);

  if (!data.soilValid) {
    ++soilErrorCount;

    if (soilErrorCount > SOIL_MAX_ERROR_COUNT) {
      Serial.println(F("[SENSOR] Soil ADC invalid more than 3 times"));
    }

    delay(300);
  } else {
    soilErrorCount = 0;

    for (uint8_t i = 0; i < SOIL_DISCARD_COUNT; ++i) {
      analogRead(SOIL_ADC_PIN);
      delay(5);
    }

    int samples[SOIL_SAMPLE_COUNT] = {};
    for (uint8_t i = 0; i < SOIL_SAMPLE_COUNT; ++i) {
      samples[i] = analogRead(SOIL_ADC_PIN);
      delay(5);
    }

    data.soilAdcFiltered = medianFilterAdc(samples, SOIL_SAMPLE_COUNT);
    data.soilValid = isAdcValid(data.soilAdcFiltered);
    data.soilPercent = calculateSoilPercent(data.soilAdcFiltered);
  }

  if (!data.soilValid) {
    data.soilAdcFiltered = data.soilAdcRaw;
    data.soilPercent = 0.0f;
    Serial.println(F("[SENSOR] Soil sensor read failed"));
  }

  data.errorFlag = !data.dhtValid || !data.soilValid;

  return data;
}

const char* sensorStatusText(bool valid) {
  return valid ? "OK" : "ERROR";
}
