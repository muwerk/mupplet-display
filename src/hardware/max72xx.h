// max72xx.h - MAX72XX driver class

#pragma once

#include <SPI.h>

namespace ustd {
/*! \brief The MAX72XX Controller Class
 *
 * This class implements the communication interface to a maxim MAX7219 or MAX7221 Serially
 * Interfaced, 8-Digit, LED Display Drivers
 * See https://datasheets.maximintegrated.com/en/ds/MAX7219-MAX7221.pdf
 */
class Max72XX {
  protected:
    // hardware configuration
    uint8_t csPin;
    uint8_t chainLen;

  public:
    /*! The maxim 7219/7221 operation modes */
    enum OP {
        noop = 0,         ///< No operation
        digit0 = 1,       ///< Digit 0
        digit1 = 2,       ///< Digit 1
        digit2 = 3,       ///< Digit 2
        digit3 = 4,       ///< Digit 3
        digit4 = 5,       ///< Digit 4
        digit5 = 6,       ///< Digit 5
        digit6 = 7,       ///< Digit 6
        digit7 = 8,       ///< Digit 7
        decodemode = 9,   ///< Decode Mode (0,1,15,255)
        intensity = 10,   ///< Intensity (0-15)
        scanlimit = 11,   ///< Scan-Limit (0-7)
        shutdown = 12,    ///< Shutdown Mode (0 or 1)
        displaytest = 15  ///< Display Test (0 or 1)
    };

    /*! Instantiate a Max72XX instance
     *
     * @param csPin The chip select pin
     * @param chainLen The length of the device chain
     * */
    Max72XX(uint8_t csPin, uint8_t chainLen) : csPin(csPin), chainLen(chainLen) {
        if (chainLen > 16) {
            chainLen = 16;
        }
    }

    /*! Start the driver class
     */
    void begin() {
        // initialize chip select
        pinMode(csPin, OUTPUT);
        digitalWrite(csPin, HIGH);

        // multiple init management is done inside the SPI library
        SPI.begin();
    }

    /*! Get the number of devices in the chain
     * @return The number ofdevices in the chain
     */
    inline uint8_t getChainLen() const {
        return chainLen;
    }

    /*! Sets the BCD code B (0-9, E, H, L, P, and -) or no-decode operation for each digit
     * @param mode The bitmask specifying the decode mode for each digit.
     */
    inline void setDecodeMode(uint8_t mode) {
        sendCommand(OP::decodemode, mode);
    }

    /*! Set the brightness of all devices
     * @param intensity The brightness of the devices. (0..15)
     */
    inline void setIntensity(uint8_t intensity) {
        sendCommand(OP::intensity, intensity > 15 ? 15 : intensity);
    }

    /*! The scan-limit register sets how many digits are displayed, from 1 to 8
     * @param scanLimit Number of digits to show from 1 to 8
     */
    inline void setScanLimit(uint8_t scanlimit) {
        sendCommand(OP::scanlimit, scanlimit - 1);
    }

    /*! Set the power saving mode for all devices
     * @param powersave If `true` the device goes into power-down mode. Set to `false` for normal
     * operation.
     */
    inline void setPowerSave(bool powersave) {
        sendCommand(OP::shutdown, powersave ? 0 : 1);
    }

    /*! Set the test mode for all devices
     *
     * In Test mode all leds are turned on in order to check for correct function.
     *
     * @param testmode If `true` the devices goes into test mode. Set to `false` for normal
     * operation.
     */
    inline void setTestMode(bool testmode) {
        sendCommand(OP::displaytest, testmode ? 1 : 0);
    }

    /*! Sends a command to all devices
     *
     * @param opcode Operation register
     * @param data Operation parameter
     */
    void sendCommand(OP opcode, uint8_t data) {
        digitalWrite(csPin, LOW);
        for (uint8_t count = 0; count < chainLen; count++) {
            SPI.transfer(opcode);
            SPI.transfer(data);
        }
        digitalWrite(csPin, HIGH);
    }

    /*! Sends a block of data
     * @param buffer Buffer to send
     * @param size Size in bytes of the buffer to send
     */
    void sendBlock(uint8_t *buffer, uint8_t size) {
        digitalWrite(csPin, LOW);
        SPI.transfer(buffer, size);
        digitalWrite(csPin, HIGH);
    }
};
}  // namespace ustd