#include "IrManager.h"
#include <IRremote.h>

IrManager::IrManager(uint8_t irPin) : _irPin(irPin), _irAvailable(false), _irData(0) {}

void IrManager::begin()
{
    IrReceiver.begin(_irPin);
}

void IrManager::loop()
{
    if (IrReceiver.decode())
    {
        if (IrReceiver.decodedIRData.decodedRawData != 0)
        {
            _irData = IrReceiver.decodedIRData.decodedRawData;
            _irAvailable = true;
        }
        IrReceiver.resume();
    }
}

bool IrManager::available()
{
    return _irAvailable;
}

uint64_t IrManager::read()
{
    _irAvailable = false;
    return _irData;
}

void IrManager::resume()
{
    IrReceiver.resume();
}
