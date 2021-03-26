#include "Environment.h"

Environment::Environment() {
    lastIMURead = 0;
    imuIntegrationCount = 0;
    imuReady = false;
    temperature = distance1 = 0.0;
}

void Environment::begin() {
#if ENABLE_IMU == true
    Wire.begin();
    Wire.setClock(400000);
    bool initialized = false;
    while (!initialized) {
        imu.begin(Wire, 1);
        if (imu.status == ICM_20948_Stat_Ok) {
            Serial.println("LIMU ok");
            initialized = true;
        } else {
            Serial.println("LIMU non available, trying again...");
            delay(500);
        }
    }
#endif
    pinMode(DIST_SENS1_TRIG, OUTPUT);
    pinMode(DIST_SENS1_ECHO, INPUT);
}

bool Environment::readIMU() {
    // Adesso filtra i dati con una media di tre valori
    // TODO: provare altri filtri, come passa-basso e Kalman
#if ENABLE_IMU == true
    unsigned long t = millis();
    if ((t - lastIMURead >= IMU_READ_INTERVAL) && imu.dataReady()) {
        imu.getAGMT();
        imuMatrix[0][0] += (double)imu.accX();
        imuMatrix[0][1] += (double)imu.accY();
        imuMatrix[0][2] += (double)imu.accZ();
        imuMatrix[1][0] += (double)imu.gyrX();
        imuMatrix[1][1] += (double)imu.gyrY();
        imuMatrix[1][2] += (double)imu.gyrZ();
        imuMatrix[2][0] += (double)imu.magX();
        imuMatrix[2][1] += (double)imu.magY();
        imuMatrix[2][2] += (double)imu.magZ();
        temperature = imu.temp();
        imuIntegrationCount++;
        if (imuIntegrationCount == IMU_INTEGRATION_COUNT) {
            for (int i = 0; i < 3; i++) {
                for (int j = 0; j < 3; j++) {
                    imuFinalMatrix[i][j] =
                        imuMatrix[i][j] / IMU_INTEGRATION_COUNT;
                    imuMatrix[i][j] = 0;
                }
            }
            imuIntegrationCount = 0;
            imuReady = true;
        }
        lastIMURead = t;
        return true;
    }
#endif
    return false;
}

bool Environment::readDistances() {
    unsigned long t = millis();
    if (t - lastDistRead >= DIST_READ_INTERVAL) {
        digitalWrite(DIST_SENS1_TRIG, LOW);
        delayMicroseconds(2);
        digitalWrite(DIST_SENS1_TRIG, HIGH);
        delayMicroseconds(10);
        digitalWrite(DIST_SENS1_TRIG, LOW);
        distance1 = pulseIn(DIST_SENS1_ECHO, HIGH) * 0.017;
        lastDistRead = t;
        return true;
    }
    return false;
}

double Environment::getDistance1() { return distance1; }

double *Environment::getAccel() {
    double *result = new double[3];
    if (imuReady) {
        result[0] = imuFinalMatrix[0][0];
        result[1] = imuFinalMatrix[0][1];
        result[2] = imuFinalMatrix[0][2];
    } else {
        result[0] = INVALID_VALUE;
        result[1] = INVALID_VALUE;
        result[2] = INVALID_VALUE;
    }
    return result;
}

double *Environment::getGyro() {
    double *result = new double[3];
    if (imuReady) {
        result[0] = imuFinalMatrix[1][0];
        result[1] = imuFinalMatrix[1][1];
        result[2] = imuFinalMatrix[1][2];
    } else {
        result[0] = INVALID_VALUE;
        result[1] = INVALID_VALUE;
        result[2] = INVALID_VALUE;
    }
    return result;
}

double *Environment::getCompass() {
    double *result = new double[3];
    if (imuReady) {
        result[0] = imuFinalMatrix[2][0];
        result[1] = imuFinalMatrix[2][1];
        result[2] = imuFinalMatrix[2][2];
    } else {
        result[0] = INVALID_VALUE;
        result[1] = INVALID_VALUE;
        result[2] = INVALID_VALUE;
    }
    return result;
}

float Environment::getTemp() { return imuReady ? temperature : INVALID_VALUE; }

float Environment::readbattery() {
    unsigned int val = analogRead(BATTERY_PIN);
    return float(val) / 64;
}