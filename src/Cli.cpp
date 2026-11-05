#include "Cli.h"
#include "Config.h"
#include "DccDecoder.h"
#include <CanUniversal.h>
#include <ESP.h>

extern volatile bool canMonitorEnabled;
extern volatile int32_t canMonitorFilter;

static String input;

// ---------------------------------------------------------------------------
// Définition d'un handler de commande
// ---------------------------------------------------------------------------
struct CliCommand {
    const char* name;
    void (*handler)(const String &args);
};

// ---------------------------------------------------------------------------
// Handlers des commandes
// ---------------------------------------------------------------------------
void cmd_stats(const String&) {
    uint32_t b0, b1, co, bad;
    DccDecoder_getStats(b0, b1, co, bad);
    Serial.printf("STATS: b0=%lu b1=%lu cutout=%lu bad=%lu\n",
                  b0, b1, co, bad);
    Serial.printf("Dropped events: %lu\n",
                  (unsigned long)DccDecoder_getDroppedEvents());
    Serial.printf("CAN0: %s\n", CanBus::bus(0).isValid() ? "valid" : "INVALID");
}

void cmd_reset(const String&) {
    Serial.println("Resetting ESP32...");
    delay(100);
    ESP.restart();
}

void cmd_debug_on(const String&) {
    Serial.println("Debug ON");
}

void cmd_debug_off(const String&) {
    Serial.println("Debug OFF");
}

void cmd_scope_on(const String&) {
    Serial.println("Scope mode ON (recompile needed)");
}

void cmd_scope_off(const String&) {
    Serial.println("Scope mode OFF (recompile needed)");
}

// ---------------- CAN MONITOR ----------------

void cmd_can_on(const String&) {
    canMonitorEnabled = true;
    Serial.println("CAN monitor ON");
}

void cmd_can_off(const String&) {
    canMonitorEnabled = false;
    Serial.println("CAN monitor OFF");
}

void cmd_can_filter(const String &args) {
    if (args == "off") {
        canMonitorFilter = -1;
        Serial.println("CAN filter disabled");
    } else {
        int id = (int) strtol(args.c_str(), nullptr, 16);
        canMonitorFilter = id;
        Serial.printf("CAN filter set to 0x%03X\n", id);
    }
}

// ---------------------------------------------------------------------------
// Tableau des commandes
// ---------------------------------------------------------------------------
static const CliCommand commands[] = {
    { "stats",        cmd_stats },
    { "reset",        cmd_reset },
    { "debug on",     cmd_debug_on },
    { "debug off",    cmd_debug_off },
    { "scope on",     cmd_scope_on },
    { "scope off",    cmd_scope_off },
    { "can on",       cmd_can_on },
    { "can off",      cmd_can_off },
    { "can filter",   cmd_can_filter },
};

// ---------------------------------------------------------------------------
// Dispatcher : recherche la commande et appelle son handler
// ---------------------------------------------------------------------------
static bool Cli_dispatch(const String &cmd) {
    for (auto &c : commands) {
        if (cmd.startsWith(c.name)) {
            String args = cmd.substring(strlen(c.name));
            args.trim();
            c.handler(args);
            return true;
        }
    }
    return false;
}

// ---------------------------------------------------------------------------
// Initialisation
// ---------------------------------------------------------------------------
void Cli_begin() {
    input.reserve(64);
}

// ---------------------------------------------------------------------------
// Boucle CLI
// ---------------------------------------------------------------------------
void Cli_task() {
    while (Serial.available()) {
        char c = Serial.read();

        if (c == '\n' || c == '\r') {
            if (input.length() == 0) return;

            String cmd = input;
            input = "";
            cmd.trim();

            if (!Cli_dispatch(cmd)) {
                Serial.printf("Unknown command: %s\n", cmd.c_str());
            }

            return;
        }

        input += c;
    }
}