#include "Cli.h"
#include "config.h"
#include "DccDecoder.h"

static String input;

void Cli_begin() {
    input.reserve(64);
}

void Cli_task() {
    while (Serial.available()) {
        char c = Serial.read();

        if (c == '\n' || c == '\r') {
            if (input.length() == 0) return;

            String cmd = input;
            input = "";

            cmd.trim();

            // ----- Commandes -----

            if (cmd == "stats") {
                uint32_t b0, b1, co, bad;
                DccDecoder_getStats(b0, b1, co, bad);
                Serial.printf("STATS: b0=%lu b1=%lu cutout=%lu bad=%lu\n",
                              b0, b1, co, bad);
            }

            else if (cmd == "reset") {
                Serial.println("Resetting ESP32...");
                delay(100);
                ESP.restart();
            }

            else if (cmd == "debug on") {
                Serial.println("Debug ON");
                // On ne modifie pas le define, mais on peut activer un flag runtime
            }

            else if (cmd == "debug off") {
                Serial.println("Debug OFF");
            }

            else if (cmd == "scope on") {
                Serial.println("Scope mode ON (recompile needed)");
            }

            else if (cmd == "scope off") {
                Serial.println("Scope mode OFF (recompile needed)");
            }

            else {
                Serial.printf("Unknown command: %s\n", cmd.c_str());
            }

            return;
        }

        input += c;
    }
}
