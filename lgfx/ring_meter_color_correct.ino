// ring_meter_final_fixed.ino - Fixed label redrawing
#define LGFX_USE_V1
#include <LovyanGFX.hpp>
#include "LGFX_JD9165_Device_working.hpp"
#include "color_helpers.h"

// Maak globale instance
LGFX_JD9165 lcd;

// Gebruik kleuren uit color_helpers.h
#define BACKGROUND_COLOR  COLOR_BLACK
#define RING_COLOR        COLOR_GRAY
#define NEEDLE_COLOR      COLOR_BLUE
#define TEXT_COLOR        COLOR_WHITE
#define ZONE1_COLOR       COLOR_RED
#define ZONE2_COLOR       COLOR_YELLOW
#define ZONE3_COLOR       COLOR_GREEN

// Meter parameters
#define CENTER_X          400
#define CENTER_Y          300
#define OUTER_RADIUS      180
#define NEEDLE_LENGTH     160
#define NEEDLE_WIDTH      3

float currentValue = 50.0;
float targetValue = 50.0;
float minValue = 0.0;
float maxValue = 100.0;
float lastDrawnValue = -1.0;

// Label posities bijhouden voor hertekening
struct LabelPos {
    int x;
    int y;
    const char* text;
};

LabelPos labelPositions[5]; // 5 labels: 0, 25, 50, 75, 100

void setup() 
{
    delay(1000);
    Serial.begin(115200);
    Serial.println("\n========================================");
    Serial.println("   JD9165 - RING METER FIXED");
    Serial.println("========================================");
    
    // Initialiseer display
    if (!lcd.begin(true)) {
        Serial.println("FAILED!");
        while(1) delay(1000);
    }
    
    Serial.println("SUCCESS!");
    
    // Teken compleet scherm
    drawCompleteScreen(50.0);
    
    delay(2000);
    
    // Start demo
    runDemo();
}

// ============================================
// DRAWING FUNCTIONS
// ============================================

void drawCompleteScreen(float value) {
    lcd.fillScreen(BACKGROUND_COLOR);
    drawStaticElements();
    drawNeedleSimple(value);
    drawValueBox(value);
    lastDrawnValue = value;
}

void drawStaticElements() {
    // 1. Buitenring
    drawThickRing(CENTER_X, CENTER_Y, OUTER_RADIUS, 10, RING_COLOR);
    
    // 2. Waardezones
    drawValueZones();
    
    // 3. Ticks
    drawTicks();
    
    // 4. Labels (sla posities op)
    drawAndStoreLabels();
    
    // 5. Centrum punt
    lcd.fillCircle(CENTER_X, CENTER_Y, 8, RING_COLOR);
    lcd.drawCircle(CENTER_X, CENTER_Y, 8, TEXT_COLOR);
}

void drawThickRing(int x, int y, int radius, int thickness, uint16_t color) {
    for (int r = radius - thickness/2; r <= radius + thickness/2; r++) {
        drawArc(x, y, r, 225, 315, color);
    }
}

void drawArc(int x, int y, int radius, int startDeg, int endDeg, uint16_t color) {
    for (int deg = startDeg; deg <= endDeg; deg++) {
        float rad = deg * 3.14159 / 180.0;
        int px = x + radius * cos(rad);
        int py = y + radius * sin(rad);
        lcd.drawPixel(px, py, color);
    }
}

void drawValueZones() {
    // Zone 1: 0-33% (225-255 graden)
    drawZone(225, 255, ZONE1_COLOR, 15);
    
    // Zone 2: 33-66% (255-285 graden)
    drawZone(255, 285, ZONE2_COLOR, 15);
    
    // Zone 3: 66-100% (285-315 graden)
    drawZone(285, 315, ZONE3_COLOR, 15);
}

void drawZone(int startDeg, int endDeg, uint16_t color, int thickness) {
    int innerRadius = OUTER_RADIUS - 20;
    
    for (int deg = startDeg; deg <= endDeg; deg++) {
        float rad = deg * 3.14159 / 180.0;
        
        int x1 = CENTER_X + (OUTER_RADIUS - 5) * cos(rad);
        int y1 = CENTER_Y + (OUTER_RADIUS - 5) * sin(rad);
        int x2 = CENTER_X + innerRadius * cos(rad);
        int y2 = CENTER_Y + innerRadius * sin(rad);
        
        lcd.drawLine(x1, y1, x2, y2, color);
    }
}

void drawTicks() {
    // Grote ticks (0, 25, 50, 75, 100)
    int majorTicks[] = {225, 247, 270, 293, 315};
    
    for (int i = 0; i < 5; i++) {
        float rad = majorTicks[i] * 3.14159 / 180.0;
        
        int x1 = CENTER_X + (OUTER_RADIUS - 5) * cos(rad);
        int y1 = CENTER_Y + (OUTER_RADIUS - 5) * sin(rad);
        int x2 = CENTER_X + (OUTER_RADIUS - 25) * cos(rad);
        int y2 = CENTER_Y + (OUTER_RADIUS - 25) * sin(rad);
        
        lcd.drawLine(x1, y1, x2, y2, TEXT_COLOR);
        lcd.drawLine(x1+1, y1, x2+1, y2, TEXT_COLOR);
    }
    
    // Kleine ticks (elke 5%)
    for (int deg = 225; deg <= 315; deg += 9) {
        if ((deg - 225) % 27 == 0) continue; // Skip grote ticks
        
        float rad = deg * 3.14159 / 180.0;
        
        int x1 = CENTER_X + (OUTER_RADIUS - 5) * cos(rad);
        int y1 = CENTER_Y + (OUTER_RADIUS - 5) * sin(rad);
        int x2 = CENTER_X + (OUTER_RADIUS - 15) * cos(rad);
        int y2 = CENTER_Y + (OUTER_RADIUS - 15) * sin(rad);
        
        lcd.drawLine(x1, y1, x2, y2, TEXT_COLOR);
    }
}

void drawAndStoreLabels() {
    const char* labels[] = {"0", "25", "50", "75", "100"};
    int labelDegrees[] = {225, 247, 270, 293, 315};
    int labelRadius = OUTER_RADIUS - 40;
    
    lcd.setTextSize(2);
    lcd.setTextColor(TEXT_COLOR);
    lcd.setTextDatum(middle_center);
    
    for (int i = 0; i < 5; i++) {
        float rad = labelDegrees[i] * 3.14159 / 180.0;
        
        int x = CENTER_X + labelRadius * cos(rad);
        int y = CENTER_Y + labelRadius * sin(rad);
        
        // Sla positie op
        labelPositions[i].x = x;
        labelPositions[i].y = y;
        labelPositions[i].text = labels[i];
        
        // Teken label
        lcd.drawString(labels[i], x, y);
    }
}

void redrawLabels() {
    lcd.setTextSize(2);
    lcd.setTextColor(TEXT_COLOR);
    lcd.setTextDatum(middle_center);
    
    for (int i = 0; i < 5; i++) {
        // Wis eerst het label gebied
        int clearW = 40;  // Breed genoeg voor 3 cijfers
        int clearH = 30;  // Hoog genoeg voor tekst
        int clearX = labelPositions[i].x - clearW/2;
        int clearY = labelPositions[i].y - clearH/2;
        
        lcd.fillRect(clearX, clearY, clearW, clearH, BACKGROUND_COLOR);
        
        // Teken label opnieuw
        lcd.drawString(labelPositions[i].text, labelPositions[i].x, labelPositions[i].y);
    }
}

void drawNeedleSimple(float value) {
    // Bereken positie
    float normalized = (value - minValue) / (maxValue - minValue);
    int angle = 225 + normalized * 90;
    float rad = angle * 3.14159 / 180.0;
    
    int tipX = CENTER_X + NEEDLE_LENGTH * cos(rad);
    int tipY = CENTER_Y + NEEDLE_LENGTH * sin(rad);
    
    // Wis oude naald
    if (lastDrawnValue >= 0) {
        eraseOldNeedle();
    }
    
    // Teken nieuwe naald
    lcd.drawLine(CENTER_X, CENTER_Y, tipX, tipY, NEEDLE_COLOR);
    lcd.drawLine(CENTER_X+1, CENTER_Y, tipX+1, tipY, NEEDLE_COLOR);
    
    // Naald punt
    lcd.fillCircle(tipX, tipY, NEEDLE_WIDTH, NEEDLE_COLOR);
    
    // HERteken labels die mogelijk gewist zijn
    redrawLabels();
    
    lastDrawnValue = value;
}

void eraseOldNeedle() {
    if (lastDrawnValue < 0) return;
    
    // Bereken oude positie
    float oldNormalized = (lastDrawnValue - minValue) / (maxValue - minValue);
    int oldAngle = 225 + oldNormalized * 90;
    float oldRad = oldAngle * 3.14159 / 180.0;
    
    int oldTipX = CENTER_X + NEEDLE_LENGTH * cos(oldRad);
    int oldTipY = CENTER_Y + NEEDLE_LENGTH * sin(oldRad);
    
    // Wis naald lijn (maar NIET te dik, anders wissen we labels)
    eraseLineThin(CENTER_X, CENTER_Y, oldTipX, oldTipY);
    
    // Wis naald punt
    lcd.fillCircle(oldTipX, oldTipY, NEEDLE_WIDTH + 2, BACKGROUND_COLOR);
    
    // Herteken meter elementen (zonder labels - die doen we apart)
    redrawMeterElements(oldAngle);
}

void eraseLineThin(int x1, int y1, int x2, int y2) {
    // Gebruik een dunnere lijn om labels te sparen
    lcd.drawLine(x1, y1, x2, y2, BACKGROUND_COLOR);
    lcd.drawLine(x1+1, y1, x2+1, y2, BACKGROUND_COLOR);
    lcd.drawLine(x1-1, y1, x2-1, y2, BACKGROUND_COLOR);
    lcd.drawLine(x1, y1+1, x2, y2+1, BACKGROUND_COLOR);
    lcd.drawLine(x1, y1-1, x2, y2-1, BACKGROUND_COLOR);
}

void redrawMeterElements(int angle) {
    // Herteken alleen de ring en zones (geen labels)
    for (int deg = angle - 15; deg <= angle + 15; deg++) {
        if (deg < 225 || deg > 315) continue;
        
        float rad = deg * 3.14159 / 180.0;
        
        // Herteken ring
        for (int r = OUTER_RADIUS - 5; r <= OUTER_RADIUS + 5; r++) {
            int px = CENTER_X + r * cos(rad);
            int py = CENTER_Y + r * sin(rad);
            lcd.drawPixel(px, py, RING_COLOR);
        }
        
        // Herteken zone
        uint16_t zoneColor;
        if (deg < 255) zoneColor = ZONE1_COLOR;
        else if (deg < 285) zoneColor = ZONE2_COLOR;
        else zoneColor = ZONE3_COLOR;
        
        int x1 = CENTER_X + (OUTER_RADIUS - 5) * cos(rad);
        int y1 = CENTER_Y + (OUTER_RADIUS - 5) * sin(rad);
        int x2 = CENTER_X + (OUTER_RADIUS - 20) * cos(rad);
        int y2 = CENTER_Y + (OUTER_RADIUS - 20) * sin(rad);
        
        lcd.drawLine(x1, y1, x2, y2, zoneColor);
    }
    
    // Herteken ticks in dit gebied
    redrawTicksNear(angle);
    
    // Herteken centrum punt
    lcd.fillCircle(CENTER_X, CENTER_Y, 8, RING_COLOR);
    lcd.drawCircle(CENTER_X, CENTER_Y, 8, TEXT_COLOR);
}

void redrawTicksNear(int angle) {
    int majorTicks[] = {225, 247, 270, 293, 315};
    
    for (int i = 0; i < 5; i++) {
        if (abs(angle - majorTicks[i]) < 20) {
            float rad = majorTicks[i] * 3.14159 / 180.0;
            
            int x1 = CENTER_X + (OUTER_RADIUS - 5) * cos(rad);
            int y1 = CENTER_Y + (OUTER_RADIUS - 5) * sin(rad);
            int x2 = CENTER_X + (OUTER_RADIUS - 25) * cos(rad);
            int y2 = CENTER_Y + (OUTER_RADIUS - 25) * sin(rad);
            
            lcd.drawLine(x1, y1, x2, y2, TEXT_COLOR);
            lcd.drawLine(x1+1, y1, x2+1, y2, TEXT_COLOR);
        }
    }
}

void drawValueBox(float value) {
    int boxX = 600;
    int boxY = 250;
    int boxW = 150;
    int boxH = 60;
    
    // Wis alleen het binnenste van de box
    lcd.fillRect(boxX + 1, boxY + 1, boxW - 2, boxH - 2, BACKGROUND_COLOR);
    
    // Teken rand opnieuw
    lcd.drawRect(boxX, boxY, boxW, boxH, TEXT_COLOR);
    
    // Waarde tekst
    lcd.setTextSize(4);
    lcd.setTextColor(TEXT_COLOR);
    lcd.setTextDatum(middle_center);
    
    char valueStr[20];
    sprintf(valueStr, "%.1f", value);
    lcd.drawString(valueStr, boxX + boxW/2, boxY + boxH/2);
    
    // Eenheid
    lcd.setTextSize(2);
    lcd.drawString("%", boxX + boxW - 20, boxY + boxH/2 - 5);
}

// ============================================
// ANIMATION FUNCTIONS
// ============================================

void updateTargetValue(float newValue) {
    targetValue = constrain(newValue, minValue, maxValue);
}

void animateToTarget() {
    static uint32_t lastUpdate = 0;
    
    if (millis() - lastUpdate < 50) return; // 20 FPS
    lastUpdate = millis();
    
    float diff = targetValue - currentValue;
    
    if (fabs(diff) > 0.3) {
        float step = diff * 0.3;
        
        float oldValue = currentValue;
        currentValue += step;
        
        // Update needle als waarde voldoende veranderd
        if (fabs(oldValue - currentValue) >= 0.5) {
            drawNeedleSimple(currentValue);
            
            // Update waarde box minder vaak
            static uint32_t lastValueUpdate = 0;
            if (millis() - lastValueUpdate > 300) {
                lastValueUpdate = millis();
                drawValueBox(currentValue);
            }
        }
    }
}

void runDemo() {
    Serial.println("Starting demo...");
    
    // Langzame sweep om te testen
    Serial.println("Slow sweep 0 -> 100");
    for (int i = 0; i <= 100; i += 5) {
        updateTargetValue(i);
        
        while (fabs(targetValue - currentValue) > 1.0) {
            animateToTarget();
            delay(40); // Langzamer voor debugging
        }
        delay(100);
        Serial.printf("At: %.1f%%\n", currentValue);
    }
    
    Serial.println("Demo complete!");
}

void loop() 
{
    // Eenvoudige demo - beweeg tussen 3 posities
    static uint8_t demoPhase = 0;
    static uint32_t lastPhaseChange = 0;
    
    if (millis() - lastPhaseChange > 4000) {
        lastPhaseChange = millis();
        
        float newTarget;
        switch(demoPhase) {
            case 0: newTarget = 25.0; break;
            case 1: newTarget = 50.0; break;
            case 2: newTarget = 75.0; break;
            default: newTarget = 50.0; demoPhase = 0; break;
        }
        
        updateTargetValue(newTarget);
        demoPhase++;
        
        Serial.printf("Moving to: %.1f%%\n", newTarget);
    }
    
    // Animate
    animateToTarget();
    delay(20);
}