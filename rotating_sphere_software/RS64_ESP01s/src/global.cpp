#include "I2CMaster.h"
#include "I2CFileTransfer.h"
#include "my_ESP.h"

// Jetzt erst erzeugen wir die globalen Objekte
I2CMaster i2c;
I2CFileTransfer fileTransfer;
my_ESP ESP01s;  // falls Konstruktor mit Referenz

