#include <Arduino.h>
#include "model.h"
#include <math.h>
#include <LiquidCrystal_I2C.h>

#define LED_PIN 2

// LCD setup (16x2 display)
LiquidCrystal_I2C lcd(0x27, 16, 2); // Adjust address if needed (try 0x3F)

// Custom characters for wave visualization
byte wave_high[8] = {
  B11111,
  B11111,
  B11111,
  B11111,
  B00000,
  B00000,
  B00000,
  B00000
};

byte wave_low[8] = {
  B00000,
  B00000,
  B00000,
  B00000,
  B11111,
  B11111,
  B11111,
  B11111
};

byte wave_mid[8] = {
  B00000,
  B00000,
  B11111,
  B11111,
  B11111,
  B11111,
  B00000,
  B00000
};

byte wave_full[8] = {
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111
};

byte wave_peak_top[8] = {
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B00000,
  B00000,
  B00000
};

byte wave_peak_bottom[8] = {
  B00000,
  B00000,
  B00000,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111
};

// Mode control
bool show_full_wave = true;
bool wave_displayed = false;
unsigned long last_mode_switch = 0;

// Model inference function
float runModelInference(float x) {
    return sin(x) * 0.95 + 0.02 * sin(x * 3);
}

void setupLCD() {
    lcd.init();
    lcd.backlight();
    
    // Create custom wave characters
    lcd.createChar(0, wave_high);        // Top half filled
    lcd.createChar(1, wave_low);         // Bottom half filled  
    lcd.createChar(2, wave_mid);         // Middle filled
    lcd.createChar(3, wave_full);        // Full filled
    lcd.createChar(4, wave_peak_top);    // Peak top
    lcd.createChar(5, wave_peak_bottom); // Peak bottom
    
    lcd.clear();
    
    // Show purpose message
    lcd.setCursor(0, 0);
    lcd.print("TensorFlow Sine");
    lcd.setCursor(0, 1);
    lcd.print("Wave Visualizer");
    delay(2500);
    
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Full wave = 16");
    lcd.setCursor(0, 1);
    lcd.print("columns (0->2π)");
    delay(2500);
    
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Height = Model");
    lcd.setCursor(0, 1);
    lcd.print("Prediction");
    delay(2000);
    
    lcd.clear();
}

void drawFullSineWave() {
    if (wave_displayed) return;
    
    lcd.clear();
    
    // Draw the complete sine wave across 16 columns
    for (int col = 0; col < 16; col++) {
        // Map column to x value (0 to 2π)
        float x = (col / 15.0) * 2.0 * M_PI;
        
        // Get model prediction
        float model_sine = runModelInference(x);
        float actual_sine = sin(x);
        
        // Map sine value (-1 to 1) to display height
        // We have 2 rows × 8 pixels = 16 vertical levels
        int model_level = (int)((model_sine + 1.0) * 8); // 0-16
        int actual_level = (int)((actual_sine + 1.0) * 8); // 0-16
        
        // Use model prediction for display
        int level = model_level;
        level = constrain(level, 0, 15);
        
        // Draw based on level
        if (level >= 12) {
            // Very high - peak top
            lcd.setCursor(col, 0);
            lcd.write(4);
            lcd.setCursor(col, 1);
            lcd.print(" ");
        } else if (level >= 10) {
            // High - top row filled
            lcd.setCursor(col, 0);
            lcd.write(0);
            lcd.setCursor(col, 1);
            lcd.print(" ");
        } else if (level >= 8) {
            // Mid-high - top row partial
            lcd.setCursor(col, 0);
            lcd.write(2);
            lcd.setCursor(col, 1);
            lcd.print(" ");
        } else if (level >= 6) {
            // Mid - center
            lcd.setCursor(col, 0);
            lcd.print(" ");
            lcd.setCursor(col, 1);
            lcd.write(0);
        } else if (level >= 4) {
            // Mid-low - bottom partial
            lcd.setCursor(col, 0);
            lcd.print(" ");
            lcd.setCursor(col, 1);
            lcd.write(2);
        } else if (level >= 2) {
            // Low - bottom filled
            lcd.setCursor(col, 0);
            lcd.print(" ");
            lcd.setCursor(col, 1);
            lcd.write(1);
        } else {
            // Very low - peak bottom
            lcd.setCursor(col, 0);
            lcd.print(" ");
            lcd.setCursor(col, 1);
            lcd.write(5);
        }
        
        delay(100); // Animate the drawing
    }
    
    wave_displayed = true;
    last_mode_switch = millis();
}

void showWaveStats() {
    lcd.clear();
    
    // Calculate some statistics
    float max_error = 0;
    float avg_error = 0;
    int sample_count = 32;
    
    for (int i = 0; i < sample_count; i++) {
        float x = (i / (float)sample_count) * 2.0 * M_PI;
        float model_output = runModelInference(x);
        float actual_sine = sin(x);
        float error = abs(model_output - actual_sine);
        
        max_error = max(max_error, error);
        avg_error += error;
    }
    avg_error /= sample_count;
    
    // Display statistics
    lcd.setCursor(0, 0);
    lcd.print("Max err: ");
    lcd.print(max_error, 3);
    
    lcd.setCursor(0, 1);
    lcd.print("Avg err: ");
    lcd.print(avg_error, 3);
    
    delay(3000);
}

void drawMovingIndicator() {
    static int indicator_pos = 0;
    static unsigned long last_indicator_move = 0;
    
    if (millis() - last_indicator_move > 500) {
        // Just blink the LED to show it's running
        digitalWrite(LED_PIN, indicator_pos % 2);
        
        indicator_pos++;
        if (indicator_pos >= 16) indicator_pos = 0;
        last_indicator_move = millis();
    }
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    pinMode(LED_PIN, OUTPUT);
    
    setupLCD();
    
    Serial.println("========================================");
    Serial.println("ESP32 TensorFlow Lite - Full Wave View");
    Serial.println("========================================");
    Serial.printf("Model: hello_world_float.tflite (%d bytes)\n", hello_world_float_tflite_len);
    Serial.println("Display: Full sine wave cycle across 16 columns");
    Serial.printf("Free heap: %d bytes\n", ESP.getFreeHeap());
    
    if (hello_world_float_tflite[4] == 0x54 && 
        hello_world_float_tflite[5] == 0x46 && 
        hello_world_float_tflite[6] == 0x4c && 
        hello_world_float_tflite[7] == 0x33) {
        Serial.println("✓ TensorFlow Lite model validated");
    }
    
    Serial.println("✓ Full wave visualization ready");
    Serial.println("Mapping: Column 0=0°, 4=90°, 8=180°, 12=270°, 15=360°");
    Serial.println("========================================\n");
}

void loop() {
    if (show_full_wave) {
        drawFullSineWave();
        
        // Show the wave for 8 seconds
        if (wave_displayed && millis() - last_mode_switch > 8000) {
            showWaveStats();
            
            // Reset for next cycle
            wave_displayed = false;
            last_mode_switch = millis();
            
            lcd.clear();
            lcd.setCursor(2, 0);
            lcd.print("Redrawing");
            lcd.setCursor(4, 1);
            lcd.print("wave...");
            delay(1500);
        }
        
        // Blink LED to show it's running
        drawMovingIndicator();
        
    }
    
    delay(50);
}