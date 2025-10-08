#ifndef IR_MANAGER_H
#define IR_MANAGER_H

#include <Arduino.h>

class IrManager {
public:
    IrManager(uint8_t irPin);
    void begin();
    void loop();
    bool available();
    uint64_t read();
    void resume();

private:
    uint8_t _irPin;
    volatile bool _irAvailable;
    volatile uint64_t _irData;
    static void IRAM_ATTR irISR();
};

#endif // IR_MANAGER_H
