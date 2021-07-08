
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
    ServoValveState state;
} ValveServoSetting;

class ServoValve
{
public:
    /**
     * Initialize the AnalogRead library.
     * @param pin The pin to be used for analog input.
     */
    ServoValve(uint8_t pin,ValveServoSetting *inServoSettings)
    {
        servoSettings = inServoSettings;
        _pin = pin;
        _servo.setPeriodHertz(50); // Standard 50hz servo
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

    void togleValve()
    {
        Serial.println("Opening valve");
        switch (servoSettings->state)
        {
        case OPEN:
        case OPENING:
            closeValve();
            break;

        case CLOSE:
        case CLOSING:
            openValve();
            break;

        default:
            break;
        }
    }

    void tick()
    {
        switch (servoSettings->state)
        {
        case OPENING:
        if (millis() - _stop_time > servoSettings->actionDuration)
            {
                _servo.detach();
                servoSettings->state = OPEN;
                Serial.println("Open valve");
                
            }
            break;

        case CLOSING:
        if (millis() - _stop_time > servoSettings->actionDuration)
            {
                _servo.detach();
                servoSettings->state = CLOSE;
                Serial.println("Close valve");
            }
            break;

            
        default:
            break;
        }
    }

protected:
    uint8_t _pin; // hardware pin number.
    ValveServoSetting* servoSettings;
    Servo _servo;
    unsigned long _stop_time = 0;
};

#endif