
#ifndef _ServoValve_h
#define _ServoValve_h

#include <ESP32PWM.h>
#include <ESP32Servo.h>

typedef enum
{
    CLOSE = 1,
    OPEN = 2,
    OPENING = 3,
    CLOSING = 4
} ServoValveState;

typedef struct
{
    int minPulseWidth;
    int maxPulseWidth;
    int openAngle;
    int closeAngle;
    int actionDuration;
    int humidityLogicMin;
    int humidityLogicMax;
    ServoValveState state;
} ValveServoSetting;

class ServoValve
{
public:
    /**
     * Initialize the AnalogRead library.
     * @param pin The pin to be used for analog input.
     */
    ServoValve(uint8_t pin, uint8_t pinPNP, uint8_t pinHumid, ValveServoSetting *inServoSettings)
    {

        servoSettings = inServoSettings;
        _pin = pin;
        _pinPNP = pinPNP;
        _pinHumid = pinHumid;
        _servo.setPeriodHertz(50); // Standard 50hz servo
        pinMode(_pinPNP, OUTPUT);
        pinMode(_pinHumid, INPUT);
    }

    void setupPwm()
    {
        //Servo Setup
        ESP32PWM::allocateTimer(0);
        ESP32PWM::allocateTimer(1);
        ESP32PWM::allocateTimer(2);
        ESP32PWM::allocateTimer(3);
    }

    void openValve()
    {
        switch (servoSettings->state)
        {
        case OPEN: //this way we can force to open again if necessary
        case CLOSE:
            digitalWrite(_pinPNP, LOW); //Enable pnp
            _servo.attach(_pin, servoSettings->minPulseWidth, servoSettings->maxPulseWidth);
        case CLOSING:
            _servo.write(servoSettings->openAngle);
            servoSettings->state = OPENING;
            _stop_time = millis();

        default:
            break;
        }
    }
    void closeValve()
    {
        Serial.println("Closing valve");
        switch (servoSettings->state)
        {
        case CLOSE: //this way we can force to close again if necessary
        case OPEN:
            digitalWrite(_pinPNP, LOW); //Enable pnp
            _servo.attach(_pin, servoSettings->minPulseWidth, servoSettings->maxPulseWidth);
        case OPENING:
            _servo.write(servoSettings->closeAngle);
            servoSettings->state = CLOSING;
            _stop_time = millis();
            break;

        default:
            break;
        }
    }

    boolean togleValve()
    {
        Serial.println("Opening valve");
        switch (servoSettings->state)
        {
        case OPEN:
        case OPENING:
            closeValve();
            return false;

        case CLOSE:
        case CLOSING:
            openValve();
            return true;

        default:
            break;
        }
        return false;
    }

    boolean tick()
    {
        switch (servoSettings->state)
        {
        case OPENING:
            if (millis() - _stop_time > servoSettings->actionDuration)
            {

                digitalWrite(_pinPNP, HIGH); //Disable pnp
                _servo.detach();
                servoSettings->state = OPEN;
                #ifdef SERIAL_DEBUG
                    Serial.println("Open valve");
                #endif
            }
            return false;

        case CLOSING:
            if (millis() - _stop_time > servoSettings->actionDuration)
            {
                digitalWrite(_pinPNP, HIGH); //Disable pnp
                _servo.detach();
                servoSettings->state = CLOSE;
                Serial.println("Close valve");
            }
            return false;

        default:
            return true;
        }
    }
    int readHumidity()
    {

        long x = analogRead(_pinHumid);

        long in_min = servoSettings->humidityLogicMin;
        long in_max = servoSettings->humidityLogicMax;
        const long out_min = 100;
        const long out_max = 0;
        const long dividend = out_max - out_min;
        const long divisor = in_max - in_min;
        long x_limit = x;
        if (x_limit < in_min)
        {
            x_limit = in_min;
        }
        else if (x_limit > in_max)
        {
            x_limit = in_max;
        }
        const long delta = x_limit - in_min;
        int converted = (delta * dividend + (divisor / 2)) / divisor + out_min;
        #ifdef SERIAL_DEBUG
            Serial.printf("Humidity x=%d converted=%d\n", x, converted);
        #endif
        return converted;
    }

protected:
    uint8_t _pin;      // hardware pin number.
    uint8_t _pinPNP;   //pnp enable pin
    uint8_t _pinHumid; //pnp enable pin
    ValveServoSetting *servoSettings;
    Servo _servo;
    unsigned long _stop_time = 0;
};

#endif