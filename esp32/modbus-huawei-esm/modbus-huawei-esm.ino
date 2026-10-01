#include <Arduino.h>
#include <ModbusMaster.h>

// =====================================================
// RS485
// =====================================================

#define RS485_RX_PIN 16
#define RS485_TX_PIN 17

// Huawei ESM-48100 default address (214)
#define BMS_SLAVE_ID 0xD6

// Huawei ESM
#define MODBUS_BAUDRATE 9600

HardwareSerial RS485Serial(1);
ModbusMaster node;

// =====================================================
// Setup
// =====================================================

void setup()
{
    Serial.begin(115200);

    delay(1000);

    Serial.println();
    Serial.println("======================================");
    Serial.println(" Huawei ESM-48100 Modbus Reader");
    Serial.println("======================================");

    // Huawei ESM-48100
    // 9600 8N1
    RS485Serial.begin(
        MODBUS_BAUDRATE,
        SERIAL_8N1,
        RS485_RX_PIN,
        RS485_TX_PIN
    );

    node.begin(BMS_SLAVE_ID, RS485Serial);

    Serial.println("Modbus initialized.");
    Serial.printf(
        "Slave ID : 0x%02X (%d)\n",
        BMS_SLAVE_ID,
        BMS_SLAVE_ID
    );

    Serial.println();
}


// =====================================================
// Read Basic Data
// =====================================================

bool readBasicData()
{
    uint8_t result;

    /*
     * Read:
     *
     * 0x0000
     * 0x0001
     * 0x0002
     * 0x0003
     * 0x0004
     * 0x0005
     * 0x0006
     *
     * 7 registers
     */

    result = node.readHoldingRegisters(0x0000, 7);

    if (result != node.ku8MBSuccess)
    {
        Serial.print("Basic register read failed. Error: 0x");
        Serial.println(result, HEX);

        return false;
    }

    uint16_t busVoltageRaw =
        node.getResponseBuffer(0);

    uint16_t packVoltageRaw =
        node.getResponseBuffer(1);

    int16_t currentRaw =
        (int16_t)node.getResponseBuffer(2);

    uint16_t socRaw =
        node.getResponseBuffer(3);

    uint16_t sohRaw =
        node.getResponseBuffer(4);

    int16_t maxTempRaw =
        (int16_t)node.getResponseBuffer(5);

    int16_t minTempRaw =
        (int16_t)node.getResponseBuffer(6);


    // ============================================
    // Decode
    // ============================================

    float busVoltage =
        busVoltageRaw * 0.01f;

    float packVoltage =
        packVoltageRaw * 0.01f;

    float current =
        currentRaw * 0.01f;

    float soc =
        socRaw;

    float soh =
        sohRaw;

    float maxTemperature =
        maxTempRaw;

    float minTemperature =
        minTempRaw;


    // ============================================
    // Print
    // ============================================

    Serial.println();
    Serial.println("--------------------------------------");
    Serial.println("BASIC BATTERY DATA");
    Serial.println("--------------------------------------");

    Serial.printf(
        "Bus Voltage      : %.2f V\n",
        busVoltage
    );

    Serial.printf(
        "Pack Voltage     : %.2f V\n",
        packVoltage
    );

    Serial.printf(
        "Current          : %.2f A\n",
        current
    );

    Serial.printf(
        "SOC              : %.0f %%\n",
        soc
    );

    Serial.printf(
        "SOH              : %.0f %%\n",
        soh
    );

    Serial.printf(
        "Max Temperature  : %.0f C\n",
        maxTemperature
    );

    Serial.printf(
        "Min Temperature  : %.0f C\n",
        minTemperature
    );


    // ============================================
    // Power
    // ============================================

    float power =
        packVoltage * current;

    Serial.printf(
        "Battery Power    : %.2f W\n",
        power
    );

    return true;
}


// =====================================================
// Read Cell Voltage
// =====================================================

bool readCellVoltages()
{
    uint8_t result;

    /*
     * Cell 1  = 0x0022
     * Cell 15 = 0x0030
     *
     * 15 registers
     */

    result = node.readHoldingRegisters(
        0x0022,
        15
    );

    if (result != node.ku8MBSuccess)
    {
        Serial.print("Cell voltage read failed. Error: 0x");
        Serial.println(result, HEX);

        return false;
    }


    float cellSum = 0;

    float minCell = 999.0;
    float maxCell = 0;


    Serial.println();
    Serial.println("--------------------------------------");
    Serial.println("CELL VOLTAGES");
    Serial.println("--------------------------------------");


    for (int i = 0; i < 15; i++)
    {
        uint16_t raw =
            node.getResponseBuffer(i);

        float voltage =
            raw * 0.001f;

        cellSum += voltage;

        if (voltage < minCell)
            minCell = voltage;

        if (voltage > maxCell)
            maxCell = voltage;


        Serial.printf(
            "Cell %02d : %.3f V  [0x%04X]\n",
            i + 1,
            voltage,
            raw
        );
    }


    float averageCell =
        cellSum / 15.0f;

    float deltaCell =
        maxCell - minCell;


    Serial.println();
    Serial.println("--------------------------------------");
    Serial.printf(
        "Cell Average : %.3f V\n",
        averageCell
    );

    Serial.printf(
        "Cell Min     : %.3f V\n",
        minCell
    );

    Serial.printf(
        "Cell Max     : %.3f V\n",
        maxCell
    );

    Serial.printf(
        "Cell Delta   : %.3f V\n",
        deltaCell
    );

    Serial.printf(
        "Cell Sum     : %.3f V\n",
        cellSum
    );

    return true;
}


// =====================================================
// Read Statistics
// =====================================================

bool readStatistics()
{
    uint8_t result;

    /*
     * 0x0042 - 0x0043
     * Discharge times
     *
     * 0x0044 - 0x0045
     * Discharge Ah
     *
     * Read 4 registers
     */

    result = node.readHoldingRegisters(
        0x0042,
        4
    );

    if (result != node.ku8MBSuccess)
    {
        Serial.print("Statistics read failed. Error: 0x");
        Serial.println(result, HEX);

        return false;
    }


    uint32_t dischargeTimes =
        ((uint32_t)node.getResponseBuffer(0) << 16)
        |
        node.getResponseBuffer(1);


    uint32_t dischargeAh =
        ((uint32_t)node.getResponseBuffer(2) << 16)
        |
        node.getResponseBuffer(3);


    Serial.println();
    Serial.println("--------------------------------------");
    Serial.println("BATTERY STATISTICS");
    Serial.println("--------------------------------------");

    Serial.printf(
        "Discharge Times : %lu\n",
        (unsigned long)dischargeTimes
    );

    Serial.printf(
        "Discharge Ah    : %lu Ah\n",
        (unsigned long)dischargeAh
    );

    return true;
}


// =====================================================
// Main Loop
// =====================================================

void loop()
{
    Serial.println();
    Serial.println("======================================");
    Serial.println("Reading Huawei ESM-48100...");
    Serial.println("======================================");


    bool basicOK =
        readBasicData();

    delay(100);


    bool cellsOK =
        readCellVoltages();

    delay(100);


    bool statisticsOK =
        readStatistics();


    Serial.println();
    Serial.println("======================================");

    if (basicOK && cellsOK && statisticsOK)
    {
        Serial.println("READ SUCCESS");
    }
    else
    {
        Serial.println("READ ERROR");
    }

    Serial.println("======================================");


    // Poll every 10 seconds
    delay(10000);
}