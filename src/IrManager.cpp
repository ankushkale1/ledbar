#include "IrManager.h"
#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRutils.h>

// Globals
IRrecv* irrecv = nullptr;
decode_results results;

IrManager::IrManager(uint8_t irPin) : _irPin(irPin), _irAvailable(false), _irData(0) {}

void IrManager::begin() {
    irrecv = new IRrecv(_irPin);
    irrecv->enableIRIn();
}

void IrManager::loop() {
    if (irrecv->decode(&results)) {
        if (results.value != 0) {
            _irData = results.value;
            _irAvailable = true;
        }
        irrecv->resume();
    }
}

bool IrManager::available() {
    return _irAvailable;
}

uint64_t IrManager::read() {
    _irAvailable = false;
    return _irData;
}

void IrManager::resume() {
    irrecv->resume();
}
