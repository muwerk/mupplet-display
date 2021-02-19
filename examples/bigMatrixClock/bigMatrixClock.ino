// bigMatrixClock.ino - a nice example of a bug matrix clock using 12 8x8 led matrix modules driven
// by MAX7219
//
// This sample depends on the following libraries:
//
// - Arduino_JSON
// - PubSubClient@2.7
// - Wire
// - Adafruit BusIO
// - Adafruit GFX Library
// - SPI
// - ustd
// - muwerk
// - munet
// - mufonts
// - mupplet-core

#define __ESP__
// #define USE_SERIAL_DBG 1

#include "ustd_platform.h"
#include "scheduler.h"

#include "net.h"
#include "ota.h"
#include "mqtt.h"

#include "console.h"
#include "display_matrix_max72xx.h"
#include "mup_light.h"
#include "muMatrix8ptRegular.h"
#include "muHeavy8ptBold.h"

// entities for core functionality
ustd::Scheduler sched(10, 16, 32);
ustd::SerialConsole con;
ustd::Net net(LED_BUILTIN);
ustd::Ota ota;
ustd::Mqtt mqtt;
ustd::DisplayMatrixMAX72XX matrix("matrix", D8, 12, 1, 1);

// other global variables
time_t lastTime = 0;
bool showClock = false;

void setClock(String cmd) {
    if (cmd == "on" && !showClock) {
        sched.publish("matrix/display/items/time/set", "center;0;10000;16;2;--:--:--");
        sched.publish("matrix/display/items/date/set", "center;0;2000;16;2;--.--.----");
        showClock = true;

    } else if (cmd == "off" && showClock) {
        sched.publish("matrix/display/items/time/clear");
        sched.publish("matrix/display/items/date/clear");
        showClock = false;
    }
}

// main application task
void app() {
    if (!showClock) {
        return;
    }
    time_t nowTime = time(nullptr);
    if (nowTime != lastTime) {
        struct tm *lt = localtime(&nowTime);
        if (lt->tm_year > 100) {
            // update time only if time is valid (date > January 01, 2000)
            char szBuffer[64];
            sprintf(szBuffer, "%2.2i:%2.2i:%2.2i", (int)lt->tm_hour, (int)lt->tm_min,
                    (int)lt->tm_sec);
            sched.publish("matrix/display/content/time/set", szBuffer);
            sprintf(szBuffer, "%2.2i-%2.2i-%4.4i", (int)lt->tm_mday, (int)lt->tm_mon + 1,
                    (int)lt->tm_year + 1900);
            sched.publish("matrix/display/content/date/set", szBuffer);
        }
        lastTime = nowTime;
    }
}

// application setup
void setup() {
    Serial.begin(115200);
    DBG("\nStartup: " + WiFi.macAddress());

    // extend console
    con.extend("clock", [](String cmd, String args) {
        String arg = ustd::shift(args);
        if (arg == "on" || arg == "off") {
            setClock(arg);
        } else {
            Serial.print("Clock is ");
            Serial.println(showClock ? "on" : "off");
        }
    });
    con.extend("disp", [](String cmd, String args) {
        String verb = ustd::shift(args);
        if (verb.length()) {
            sched.publish("matrix/display/" + verb, args);
        } else {
        }
    });

    // initialize core components
    con.begin(&sched, "sub net/# mqtt/# matrix/light/# matrix/display/items/#", 115200);

    net.begin(&sched);
    ota.begin(&sched);
    mqtt.begin(&sched);

    matrix.begin(&sched);
    matrix.addfont(&muMatrix8ptRegular, 7);
    matrix.addfont(&muHeavy8ptBold, 7);
    sched.publish("matrix/light/set", "on");

    // initialize application
    int tID = sched.add(app, "main", 100000L);
    sched.subscribe(tID, "clock/#", [](String topic, String msg, String orig) {
        if (topic == "clock/set") {
            if (msg == "on" || msg == "off") {
                setClock(msg);
                sched.publish("clock", showClock ? "on" : "off");
            }
        } else if (topic == "clock/get") {
            sched.publish("clock", showClock ? "on" : "off");
        }
    });

    // start clock
    setClock("on");
}

// application runtime
void loop() {
    sched.loop();
}