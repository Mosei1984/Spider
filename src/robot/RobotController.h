#ifndef ROBOT_CONTROLLER_H
#define ROBOT_CONTROLLER_H

#include <Arduino.h>

// Command Enum statt String für Thread-Sicherheit
enum class MotionCmd : uint8_t {
    NONE = 0,
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT,
    TURNLEFT,
    TURNRIGHT,
    STANDBY,
    SLEEP,
    LIE,
    HELLO,
    PUSHUP,
    FIGHTING,
    DANCE1,
    DANCE2,
    DANCE3
};

class RobotController {
public:
    RobotController();
    
    // Haupt-Loop Funktion (nicht-blockierend)
    void processQueue();
    
    // Command-Verarbeitung
    void queueCommand(MotionCmd cmd);
    MotionCmd parseCommand(const char* name);
    const char* getCommandName(MotionCmd cmd);
    
    // Kontinuierliche Bewegung
    void startContinuous(MotionCmd cmd);
    void requestStop();    // Stoppt nach aktueller Sequenz
    void forceStop();      // Sofortiger Stop
    
    // Servo Offsets
    int getServoOffset(int index);
    void setServoOffset(int index, int value);

private:
    volatile MotionCmd currentCmd;
    volatile MotionCmd pendingCmd;
    volatile bool hasPendingCmd;
    volatile bool continuousMode;
    volatile bool stopAfterSequence;
    
    void executeCommand(MotionCmd cmd);
    bool isContinuousCmd(MotionCmd cmd);
};

extern RobotController robotController;

#endif
