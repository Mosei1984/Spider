// =============================================================================
// UiMenuV3.cpp - Implementierung des erweiterten UI-Menüs
// =============================================================================
#include "UiMenuV3.h"
#include "../Config.h"
#include <Arduino.h>
#include <Preferences.h>

void UiMenuV3::begin(WsClientV3* wsClient) {
    ws = wsClient;
    state = UiStateV3();
    
    // Servo-Kalibrierung aus NVS laden
    servoStore.begin();
    if (servoStore.isValid()) {
        servoStore.load(state.servoCalib, 8);
        state.servoCalibSaved = true;
    }
    
    // Walk-Parameter aus NVS laden
    loadWalkParamsLocal();
}

// =============================================================================
// Button-Handler
// =============================================================================

void UiMenuV3::onButtonLeft() {
    state.needsRedraw = true;
    
    switch (state.mode) {
        case MenuModeV3::DRIVE:
        case MenuModeV3::MOTION:
            // Im Fahrmodus: Terrain wechseln (rückwärts)
            if (state.terrainIndex > 0) {
                state.terrainIndex--;
                applyTerrain();
            }
            break;
            
        case MenuModeV3::WALK_PARAMS:
            walkParamDecrease();
            break;
            
        case MenuModeV3::SERVO_CALIB:
            // Menü-Index rückwärts (0-5)
            if (state.servoCalibMenuIndex > 0) state.servoCalibMenuIndex--;
            break;
            
        case MenuModeV3::SERVO_SELECT:
            servoSelectDown();
            break;
            
        case MenuModeV3::SERVO_OFFSET:
        case MenuModeV3::SERVO_LIMITS:
            calibValueDecrease();
            break;
            
        case MenuModeV3::ACTIONS:
            // Vorherige Aktion
            if (state.actionIndex > 0) state.actionIndex--;
            break;
            
        case MenuModeV3::TERRAIN:
            // Vorheriges Terrain + sofort anwenden
            if (state.terrainIndex > 0) {
                state.terrainIndex--;
                applyTerrain();
            }
            break;
            
        default:
            break;
    }
}

void UiMenuV3::onButtonMiddle() {
    state.needsRedraw = true;
    
    switch (state.mode) {
        case MenuModeV3::DRIVE:
        case MenuModeV3::MOTION:
            // Short press im Drive = nichts (Long press wechselt Mode in main)
            break;
            
        case MenuModeV3::WALK_PARAMS:
            // Index 0-5: Parameter, 6=Save, 7=Send
            if (state.walkParamIndex < 6) {
                // Nächster Parameter
                state.walkParamIndex++;
            } else if (state.walkParamIndex == 6) {
                // Lokal speichern
                saveWalkParamsLocal();
                state.walkParamIndex++;
            } else if (state.walkParamIndex == 7) {
                // An Bot senden und zurück
                sendWalkParamsToBot();
                state.walkParamIndex = 0;
                state.mode = MenuModeV3::DRIVE;
            }
            break;
            
        case MenuModeV3::SERVO_CALIB:
            // 0=Servo bearbeiten, 1=Grundstellung, 2=Lock/Unlock, 3=Vom Bot laden, 4=Lokal speichern, 5=An Bot senden
            switch (state.servoCalibMenuIndex) {
                case 0: state.mode = MenuModeV3::SERVO_SELECT; break;
                case 1: sendHomePosition(); break;
                case 2: toggleCalibLock(); break;
                case 3: requestCalibFromBot(); break;
                case 4: saveServoCalibLocal(); break;
                case 5: sendAllServoCalibToBot(); break;
            }
            break;
            
        case MenuModeV3::SERVO_SELECT:
            state.mode = MenuModeV3::SERVO_OFFSET;
            state.calibParamIndex = 0;
            break;
            
        case MenuModeV3::SERVO_OFFSET:
            state.calibParamIndex++;
            if (state.calibParamIndex > 0) {
                state.mode = MenuModeV3::SERVO_LIMITS;
                state.calibParamIndex = 0;
            }
            break;
            
        case MenuModeV3::SERVO_LIMITS:
            state.calibParamIndex++;
            if (state.calibParamIndex > 2) {  // min, max, center
                applyServoCalib(state.selectedServo);
                state.mode = MenuModeV3::SERVO_SELECT;
            }
            break;
            
        case MenuModeV3::ACTIONS:
            // Aktion ausführen
            executeAction();
            break;
            
        case MenuModeV3::TERRAIN:
            // Terrain setzen
            applyTerrain();
            break;
            
        default:
            state.mode = MenuModeV3::DRIVE;
            break;
    }
}

void UiMenuV3::onButtonRight() {
    state.needsRedraw = true;
    
    switch (state.mode) {
        case MenuModeV3::DRIVE:
        case MenuModeV3::MOTION:
            // Im Fahrmodus: Terrain wechseln (vorwärts)
            if (state.terrainIndex < TERRAIN_COUNT - 1) {
                state.terrainIndex++;
                applyTerrain();
            }
            break;
            
        case MenuModeV3::WALK_PARAMS:
            walkParamIncrease();
            break;
            
        case MenuModeV3::SERVO_CALIB:
            // Menü-Index vorwärts (0-5)
            if (state.servoCalibMenuIndex < 5) state.servoCalibMenuIndex++;
            break;
            
        case MenuModeV3::SERVO_SELECT:
            servoSelectUp();
            break;
            
        case MenuModeV3::SERVO_OFFSET:
        case MenuModeV3::SERVO_LIMITS:
            calibValueIncrease();
            break;
            
        case MenuModeV3::ACTIONS:
            // Nächste Aktion
            if (state.actionIndex < ACTION_COUNT - 1) state.actionIndex++;
            break;
            
        case MenuModeV3::TERRAIN:
            // Nächstes Terrain + sofort anwenden
            if (state.terrainIndex < TERRAIN_COUNT - 1) {
                state.terrainIndex++;
                applyTerrain();
            }
            break;
            
        default:
            break;
    }
}

void UiMenuV3::onPotiChange(int value) {
    switch (state.mode) {
        case MenuModeV3::WALK_PARAMS:
            if (state.walkParamIndex == 0) {
                // Stride: 0.3 - 2.0
                state.walkParams.stride = 0.3f + (value / 100.0f) * 1.7f;
            } else if (state.walkParamIndex == 1) {
                // SubSteps: 1-16
                state.walkParams.subSteps = 1 + (value * 15 / 100);
            }
            state.walkParams.validate();
            state.needsRedraw = true;
            break;
            
        case MenuModeV3::SERVO_OFFSET:
            // Offset: -30 bis +30
            state.servoCalib[state.selectedServo].offset = -30 + (value * 60 / 100);
            state.needsRedraw = true;
            break;
            
        default:
            break;
    }
}

// =============================================================================
// Walk-Parameter Anpassung
// =============================================================================

void UiMenuV3::walkParamIncrease() {
    switch (state.walkParamIndex) {
        case 0: state.walkParams.stride += 0.1f; break;
        case 1: state.walkParams.subSteps += 1; break;
        case 2: 
            if ((int)state.walkParams.profile < 2) {
                state.walkParams.profile = (TimingProfile)((int)state.walkParams.profile + 1);
            }
            break;
        case 3: state.walkParams.swingMul += 0.05f; break;
        case 4: state.walkParams.stanceMul += 0.05f; break;
        case 5: state.walkParams.rampEnabled = !state.walkParams.rampEnabled; break;
    }
    state.walkParams.validate();
}

void UiMenuV3::walkParamDecrease() {
    switch (state.walkParamIndex) {
        case 0: state.walkParams.stride -= 0.1f; break;
        case 1: state.walkParams.subSteps -= 1; break;
        case 2:
            if ((int)state.walkParams.profile > 0) {
                state.walkParams.profile = (TimingProfile)((int)state.walkParams.profile - 1);
            }
            break;
        case 3: state.walkParams.swingMul -= 0.05f; break;
        case 4: state.walkParams.stanceMul -= 0.05f; break;
        case 5: state.walkParams.rampEnabled = !state.walkParams.rampEnabled; break;
    }
    state.walkParams.validate();
}

const char* UiMenuV3::getWalkParamName(int index) {
    switch (index) {
        case 0: return "Stride";
        case 1: return "SubSteps";
        case 2: return "Profile";
        case 3: return "SwingMul";
        case 4: return "StanceMul";
        case 5: return "Ramp";
        case 6: return "Lokal speichern";
        case 7: return "An Bot senden";
        default: return "?";
    }
}

// =============================================================================
// Servo-Kalibrierung
// =============================================================================

void UiMenuV3::servoSelectUp() {
    if (state.selectedServo < 7) state.selectedServo++;
}

void UiMenuV3::servoSelectDown() {
    if (state.selectedServo > 0) state.selectedServo--;
}

void UiMenuV3::calibValueIncrease() {
    ServoCalibParams& cal = state.servoCalib[state.selectedServo];
    
    if (state.mode == MenuModeV3::SERVO_OFFSET) {
        cal.offset += 1;
    } else {
        switch (state.calibParamIndex) {
            case 0: cal.minAngle += 5; break;
            case 1: cal.maxAngle += 5; break;
            case 2: cal.centerAngle += 1; break;
        }
    }
    cal.validate();
}

void UiMenuV3::calibValueDecrease() {
    ServoCalibParams& cal = state.servoCalib[state.selectedServo];
    
    if (state.mode == MenuModeV3::SERVO_OFFSET) {
        cal.offset -= 1;
    } else {
        switch (state.calibParamIndex) {
            case 0: cal.minAngle -= 5; break;
            case 1: cal.maxAngle -= 5; break;
            case 2: cal.centerAngle -= 1; break;
        }
    }
    cal.validate();
}

const char* UiMenuV3::getCalibParamName(int index) {
    switch (index) {
        case 0: return "Min";
        case 1: return "Max";
        case 2: return "Center";
        default: return "?";
    }
}

// =============================================================================
// Actions & Terrain
// =============================================================================

void UiMenuV3::executeAction() {
    if (ws && state.actionIndex >= 0 && state.actionIndex < ACTION_COUNT) {
        ws->sendCmd(ACTION_ITEMS[state.actionIndex]);
        Serial.printf("[UiV3] Action: %s\n", ACTION_ITEMS[state.actionIndex]);
    }
}

void UiMenuV3::applyTerrain() {
    if (ws && state.terrainIndex >= 0 && state.terrainIndex < TERRAIN_COUNT) {
        state.terrainActive = TERRAIN_ITEMS[state.terrainIndex];
        ws->sendTerrain(TERRAIN_ITEMS[state.terrainIndex]);
        Serial.printf("[UiV3] Terrain: %s\n", TERRAIN_ITEMS[state.terrainIndex]);
    }
}

// =============================================================================
// Display-Ausgabe
// =============================================================================

void UiMenuV3::getDisplayLines(char* line1, char* line2, char* line3, char* line4, int& progressBar) {
    memset(line1, 0, 22);
    memset(line2, 0, 22);
    memset(line3, 0, 22);
    memset(line4, 0, 22);
    progressBar = -1;  // default: kein Balken
    
    switch (state.mode) {
        case MenuModeV3::DRIVE:
            strcpy(line1, "== DRIVE ==");
            // Aktuelle Bewegung anzeigen
            if (state.currentMove && state.currentMove[0] && strcmp(state.currentMove, "none") != 0) {
                snprintf(line2, 21, "> %s", state.currentMove);
            } else {
                strcpy(line2, "Joystick aktiv");
            }
            snprintf(line3, 21, "[<] %s [>]", state.terrainActive);
            strcpy(line4, "[HOLD MID] Menu");
            break;
            
        case MenuModeV3::MOTION:
            strcpy(line1, "== MOTION ==");
            if (!state.motionAvailable) {
                strcpy(line2, "MPU nicht gefunden!");
                strcpy(line4, "[HOLD MID] Menu");
            } else if (state.motionCalibState == CalibState::WAITING || 
                       state.motionCalibState == CalibState::RUNNING) {
                strcpy(line2, "Kalibriere...");
                strcpy(line3, "");  // wird durch progressBar ersetzt
                progressBar = state.motionCalibProgress;
                strcpy(line4, "Nicht bewegen!");
            } else if (state.motionCalibState == CalibState::DONE) {
                strcpy(line2, "Kalibrierung OK!");
                strcpy(line3, "");
                progressBar = 100;  // voller Balken
                strcpy(line4, "");
            } else if (!state.motionCalibrated) {
                strcpy(line2, "Nicht kalibriert");
                strcpy(line3, "[MID] Kalibrieren");
                strcpy(line4, "[HOLD MID] Menu");
            } else {
                // Kalibriert - Bewegung anzeigen
                if (state.currentMove && state.currentMove[0] && strcmp(state.currentMove, "none") != 0) {
                    snprintf(line2, 21, "> %s", state.currentMove);
                } else {
                    strcpy(line2, "Bereit");
                }
                snprintf(line3, 21, "[<] %s [>]", state.terrainActive);
                strcpy(line4, "[MID]Calib [HOLD]Menu");
            }
            break;
            
        case MenuModeV3::INPUT_CALIB:
            strcpy(line1, "= INPUT CALIB =");
            if (state.inputCalibStep == InputCalibStep::IDLE) {
                if (state.inputCalibrated) {
                    strcpy(line2, "Kalibriert");
                } else {
                    strcpy(line2, "Nicht kalibriert");
                }
                strcpy(line3, "[MID] Starten");
                strcpy(line4, "[HOLD MID] Menu");
            } else if (state.inputCalibStep == InputCalibStep::DEADBAND) {
                snprintf(line2, 21, "Deadband: %d%%", state.inputDeadbandPercent);
                strcpy(line3, "[L/R] +/-");
                strcpy(line4, "[MID] Speichern");
            } else if (state.inputCalibStep == InputCalibStep::DONE) {
                strcpy(line2, "Gespeichert!");
                strcpy(line3, "");
                strcpy(line4, "");
            } else if (state.inputCalibText && state.inputCalibText[0]) {
                // Mehrzeiligen Text aufteilen
                const char* nl = strchr(state.inputCalibText, '\n');
                if (nl) {
                    int len = nl - state.inputCalibText;
                    if (len > 20) len = 20;
                    strncpy(line2, state.inputCalibText, len);
                    line2[len] = '\0';
                    strncpy(line3, nl + 1, 20);
                    line3[20] = '\0';
                } else {
                    strncpy(line2, state.inputCalibText, 20);
                    line2[20] = '\0';
                }
                strcpy(line4, "");
            }
            break;
            
        case MenuModeV3::WALK_PARAMS:
            strcpy(line1, "= WALK PARAMS =");
            snprintf(line2, 21, "> %s", getWalkParamName(state.walkParamIndex));
            
            switch (state.walkParamIndex) {
                case 0: snprintf(line3, 21, "%.2f", state.walkParams.stride); break;
                case 1: snprintf(line3, 21, "%d", state.walkParams.subSteps); break;
                case 2: snprintf(line3, 21, "%s", state.walkParams.getProfileName()); break;
                case 3: snprintf(line3, 21, "%.2f", state.walkParams.swingMul); break;
                case 4: snprintf(line3, 21, "%.2f", state.walkParams.stanceMul); break;
                case 5: snprintf(line3, 21, "%s", state.walkParams.rampEnabled ? "ON" : "OFF"); break;
                case 6: 
                    strcpy(line3, state.walkParamsSaved ? "(gespeichert)" : "(nicht gespeichert)");
                    break;
                case 7: 
                    strcpy(line3, "[MID] Senden");
                    break;
            }
            if (state.walkParamIndex <= 5) {
                strcpy(line4, "[-] [OK] [+]");
            } else {
                strcpy(line4, "[OK] Weiter");
            }
            break;
            
        case MenuModeV3::SERVO_CALIB: {
            strcpy(line1, "= SERVO CALIB =");
            const char* menuItems[] = {
                "Servo bearbeiten",
                "Grundstellung",
                state.calibLocked ? "Entsperren" : "Sperren",
                "Vom Bot laden",
                "Lokal speichern",
                "An Bot senden"
            };
            snprintf(line2, 21, "> %s", menuItems[state.servoCalibMenuIndex]);
            switch (state.servoCalibMenuIndex) {
                case 0: strcpy(line3, ""); break;
                case 1: strcpy(line3, "Alle Servos 90"); break;
                case 2: strcpy(line3, state.calibLocked ? "(gesperrt)" : "(entsperrt)"); break;
                case 3: 
                    if (state.waitingForBotCalib) {
                        snprintf(line3, 21, "Empfange... %d/8", state.receivedCalibCount);
                    } else {
                        strcpy(line3, ""); 
                    }
                    break;
                case 4: strcpy(line3, state.servoCalibSaved ? "(gespeichert)" : ""); break;
                case 5: strcpy(line3, "Alle 8 Servos"); break;
            }
            strcpy(line4, "[<] [OK] [>]");
            break;
        }
            
        case MenuModeV3::SERVO_SELECT:
            strcpy(line1, "= SELECT SERVO =");
            snprintf(line2, 21, "Servo: %d", state.selectedServo);
            snprintf(line3, 21, "Off: %+d", state.servoCalib[state.selectedServo].offset);
            strcpy(line4, "[<] [OK] [>]");
            break;
            
        case MenuModeV3::SERVO_OFFSET:
            snprintf(line1, 21, "SERVO %d OFFSET", state.selectedServo);
            snprintf(line2, 21, "%+d", state.servoCalib[state.selectedServo].offset);
            strcpy(line3, "Poti/Buttons");
            strcpy(line4, "[-] [OK] [+]");
            break;
            
        case MenuModeV3::SERVO_LIMITS: {
            ServoCalibParams& cal = state.servoCalib[state.selectedServo];
            snprintf(line1, 21, "SERVO %d LIMITS", state.selectedServo);
            snprintf(line2, 21, "> %s", getCalibParamName(state.calibParamIndex));
            switch (state.calibParamIndex) {
                case 0: snprintf(line3, 21, "%d", cal.minAngle); break;
                case 1: snprintf(line3, 21, "%d", cal.maxAngle); break;
                case 2: snprintf(line3, 21, "%d", cal.centerAngle); break;
            }
            strcpy(line4, "[-] [OK] [+]");
            break;
        }
            
        case MenuModeV3::ACTIONS:
            strcpy(line1, "= AKTION =");
            snprintf(line2, 21, "> %s", ACTION_LABELS[state.actionIndex]);
            strcpy(line3, "");
            strcpy(line4, "[<] [OK] [>]");
            break;
            
        case MenuModeV3::TERRAIN:
            strcpy(line1, "= TERRAIN =");
            snprintf(line2, 21, "Aktiv: %s", state.terrainActive);
            snprintf(line3, 21, "> %s", TERRAIN_LABELS[state.terrainIndex]);
            strcpy(line4, "[<] wechseln [>]");
            break;
            
        default:
            strcpy(line1, "?");
            break;
    }
}

// =============================================================================
// Parameter senden
// =============================================================================

void UiMenuV3::applyWalkParams() {
    if (ws) {
        ws->sendSetWalkParams(state.walkParams);
        Serial.println(F("[UiV3] WalkParams applied"));
    }
}

void UiMenuV3::applyServoCalib(uint8_t servo) {
    if (ws && servo < 8) {
        ws->sendSetServoCalib(servo, state.servoCalib[servo]);
        Serial.printf("[UiV3] Servo %d calib applied\n", servo);
    }
}

void UiMenuV3::saveAllCalib() {
    if (ws) {
        ws->sendSaveCalib();
    }
}

// =============================================================================
// Servo-Kalibrierung Speichern/Senden
// =============================================================================

void UiMenuV3::saveServoCalibLocal() {
    servoStore.save(state.servoCalib, 8);
    state.servoCalibSaved = true;
    Serial.println(F("[UiV3] Servo calib saved locally"));
}

void UiMenuV3::sendAllServoCalibToBot() {
    if (!ws) return;
    
    for (uint8_t i = 0; i < 8; i++) {
        ws->sendSetServoCalib(i, state.servoCalib[i]);
        delay(20);  // Kurze Pause zwischen Nachrichten
    }
    ws->sendSaveCalib();  // Bot soll speichern
    Serial.println(F("[UiV3] All servo calib sent to bot"));
}

void UiMenuV3::loadServoCalibFromStore() {
    if (servoStore.isValid()) {
        servoStore.load(state.servoCalib, 8);
        state.servoCalibSaved = true;
    }
}

void UiMenuV3::sendHomePosition() {
    if (ws) {
        ws->sendCmd("calibpose");  // Kalibrierungsstellung (alle Servos auf 90°)
        Serial.println(F("[UiV3] Calibpose sent"));
    }
}

void UiMenuV3::toggleCalibLock() {
    state.calibLocked = !state.calibLocked;
    if (ws) {
        ws->sendSetCalibLock(state.calibLocked);
        Serial.printf("[UiV3] Calib lock: %s\n", state.calibLocked ? "locked" : "unlocked");
    }
}

void UiMenuV3::requestCalibFromBot() {
    if (ws) {
        state.waitingForBotCalib = true;
        state.receivedCalibCount = 0;
        ws->sendGetServoCalib();
    }
}

void UiMenuV3::onServoCalibReceived(uint8_t servo, const ServoCalibParams& params) {
    if (servo < 8) {
        state.servoCalib[servo] = params;
        state.receivedCalibCount++;
        state.needsRedraw = true;
        
        // Wenn alle 8 empfangen, lokal speichern
        if (state.receivedCalibCount >= 8) {
            state.waitingForBotCalib = false;
            saveServoCalibLocal();
            Serial.println(F("[UiV3] All 8 servo calibrations received and saved"));
        }
    }
}

// =============================================================================
// Walk-Parameter Speichern/Laden
// =============================================================================

void UiMenuV3::saveWalkParamsLocal() {
    Preferences prefs;
    prefs.begin("walkparams", false);
    prefs.putBool("valid", true);
    prefs.putFloat("stride", state.walkParams.stride);
    prefs.putUChar("substeps", state.walkParams.subSteps);
    prefs.putUChar("profile", (uint8_t)state.walkParams.profile);
    prefs.putFloat("swingMul", state.walkParams.swingMul);
    prefs.putFloat("stanceMul", state.walkParams.stanceMul);
    prefs.putBool("rampEn", state.walkParams.rampEnabled);
    prefs.putUChar("rampCyc", state.walkParams.rampCycles);
    prefs.end();
    state.walkParamsSaved = true;
    Serial.println(F("[UiV3] Walk params saved locally"));
}

void UiMenuV3::loadWalkParamsLocal() {
    Preferences prefs;
    prefs.begin("walkparams", true);
    if (prefs.getBool("valid", false)) {
        state.walkParams.stride = prefs.getFloat("stride", 1.0f);
        state.walkParams.subSteps = prefs.getUChar("substeps", 8);
        state.walkParams.profile = (TimingProfile)prefs.getUChar("profile", 1);
        state.walkParams.swingMul = prefs.getFloat("swingMul", 0.75f);
        state.walkParams.stanceMul = prefs.getFloat("stanceMul", 1.25f);
        state.walkParams.rampEnabled = prefs.getBool("rampEn", false);
        state.walkParams.rampCycles = prefs.getUChar("rampCyc", 3);
        state.walkParams.validate();
        state.walkParamsSaved = true;
        Serial.println(F("[UiV3] Walk params loaded"));
    }
    prefs.end();
}

void UiMenuV3::sendWalkParamsToBot() {
    if (ws) {
        ws->sendSetWalkParams(state.walkParams);
        Serial.println(F("[UiV3] Walk params sent to bot"));
    }
}
