
#ifndef _CronTask_h
#define _CronTask_h

#include <ServoValve.h>

typedef struct
{
    bool task1Active;
    int task1MinuteOfDayStart;
    int task1MinuteOfDayStop;
    short task1MaxHumidity;
    bool task2Active;
    int task2MinuteOfDayStart;
    int task2MinuteOfDayStop;
    short task2MaxHumidity;
    bool task3Active;
    int task3MinuteOfDayStart;
    int task3MinuteOfDayStop;
    short task3MaxHumidity;
    bool task4Active;
    int task4MinuteOfDayStart;
    int task4MinuteOfDayStop;
    short task4MaxHumidity;
    int lastMinuteOfDayCheck;
    int manualStartClose;
    int manualDuration;
} CronTaskSettings;

class CronTaskManager
{
public:
    /**
     * Initialize the AnalogRead library.
     * @param pin The pin to be used for analog input.
     */
    CronTaskManager(CronTaskSettings *inCronTaskSettings)
    {
        cronTaskSettings = inCronTaskSettings;
    };

    void scheduleManualClose(struct tm *timeinfo){
        cronTaskSettings->manualStartClose =  ((timeinfo->tm_hour * 60) + timeinfo->tm_min + cronTaskSettings->manualDuration +1) % 1440;
    }

    void resetManualClose(){
        cronTaskSettings->manualStartClose =  -1;
    }

    void checkAction(ServoValve* servoValve, struct tm *timeinfo, short humidity)
    {
        int minuteOfDay = (timeinfo->tm_hour * 60) + timeinfo->tm_min;

        if (minuteOfDay == cronTaskSettings->lastMinuteOfDayCheck)
            return;
        else
            Serial.printf("minuteOfDay %d\n", minuteOfDay);

        cronTaskSettings->lastMinuteOfDayCheck = minuteOfDay;

        if (cronTaskSettings->task1Active)
        {            
            if (minuteOfDay == cronTaskSettings->task1MinuteOfDayStart && humidity < cronTaskSettings->task1MaxHumidity)
            {
                Serial.println("Task1 open");
                servoValve->openValve();
            }
            if (minuteOfDay == cronTaskSettings->task1MinuteOfDayStop)
            {
                Serial.println("Task1 close");
                servoValve->closeValve();
            }
        }
        if (cronTaskSettings->task2Active)
        {
            if (minuteOfDay == cronTaskSettings->task2MinuteOfDayStart && humidity < cronTaskSettings->task2MaxHumidity)
            {
                Serial.println("Task2 open");
                servoValve->openValve();
            }
            if (minuteOfDay == cronTaskSettings->task2MinuteOfDayStop)
            {
                Serial.println("Task2 close");
                servoValve->closeValve();
            }
        }
        if (cronTaskSettings->task3Active)
        {
            if (minuteOfDay == cronTaskSettings->task3MinuteOfDayStart && humidity < cronTaskSettings->task3MaxHumidity)
            {
                Serial.println("Task3 open");
                servoValve->openValve();
            }
            if (minuteOfDay == cronTaskSettings->task3MinuteOfDayStop)
            {
                Serial.println("Task3 close");
                servoValve->closeValve();
            }
        }
        if (cronTaskSettings->task4Active)
        {
            if (minuteOfDay == cronTaskSettings->task4MinuteOfDayStart && humidity < cronTaskSettings->task4MaxHumidity)
            {
                Serial.println("Task4 open");
                servoValve->openValve();
            }
            if (minuteOfDay == cronTaskSettings->task4MinuteOfDayStop)
            {
                Serial.println("Task4 close");
                servoValve->closeValve();
            }
        }
        if (cronTaskSettings->manualStartClose>=0 && minuteOfDay >= cronTaskSettings->manualStartClose){
            {
                Serial.println("Manual timer Close");
                servoValve->closeValve();
                cronTaskSettings->manualStartClose = -1;
            }
        }
    };

private:
    CronTaskSettings *cronTaskSettings;
};

#endif