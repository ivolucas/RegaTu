
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
    ServoValve(uint8_t pin,uint8_t pinPNP,ValveServoSetting *inServoSettings)
    {
        
        servoSettings = inServoSettings;
        _pin = pin;
        _pinPNP = pinPNP;
        _servo.setPeriodHertz(50); // Standard 50hz servo
        pinMode(_pinPNP,OUTPUT);
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
            digitalWrite(_pinPNP,LOW);//Enable pnp
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
            digitalWrite(_pinPNP,LOW);//Enable pnp        
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
                
                digitalWrite(_pinPNP,HIGH);//Disable pnp
                _servo.detach();
                servoSettings->state = OPEN;
                Serial.println("Open valve");
                
            }
            return false;

        case CLOSING:
            if (millis() - _stop_time > servoSettings->actionDuration)
            {
                digitalWrite(_pinPNP,HIGH);//Disable pnp        
                _servo.detach();
                servoSettings->state = CLOSE;
                Serial.println("Close valve");
            }
            return false;

            
        default:
            return true;
        }
    }

    

protected:
    uint8_t _pin; // hardware pin number.
    uint8_t _pinPNP; //pnp enable pin
    ValveServoSetting* servoSettings;
    Servo _servo;
    unsigned long _stop_time = 0;
};

#endif