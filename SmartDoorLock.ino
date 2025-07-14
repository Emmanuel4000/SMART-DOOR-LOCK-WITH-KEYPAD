
#include <Keypad.h>
#include <LiquidCrystal.h>
#include <Servo.h>
#include <EEPROM.h> // MODIFIED FOR EEPROM

// Define pins for various components
#define SERVO_PIN 12          // Servo motor pin
#define RED_LED_PIN 10        // Red LED pin (for incorrect password indication)
#define GREEN_LED_PIN 11      // Green LED pin (for access granted indication)
#define BUZZER_PIN 13         // Buzzer pin (Changed from 1 to 13)

Servo myservo;
LiquidCrystal lcd(A0, A1, A2, A3, A4, A5); // LCD object with defined pins

// Keypad configuration
const byte rows = 4, cols = 4; // Keypad dimensions (4x4)
char keys[rows][cols] = {      // Keypad layout
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};
byte rowPins[rows] = {9, 8, 7, 6}; // Row pins connected to Arduino
byte colPins[cols] = {5, 4, 3, 2};  // Column pins connected to Arduino

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, rows, cols); // Keypad object

// Passwords
const int PASSWORD_LENGTH = 6; // Define password length here
char masterPassword[] = "123456"; // Master password for password change
char savedPassword[PASSWORD_LENGTH + 1]; // User-set password (+1 for null terminator)
char enteredPassword[PASSWORD_LENGTH + 1];    // Buffer for entering a password (+1 for null terminator)
int currentPosition = 0;             // Tracks the number of entered digits
int invalidCount = 0;                // Counts incorrect password attempts

// Flags for different modes
bool isSettingPassword = false;     // Indicates if the system is in password setup mode
bool isCheckingMaster = false;      // Indicates if the system is verifying the master password

// EEPROM address where the password will be stored
#define EEPROM_PASSWORD_ADDRESS 0 // You can choose any address, 0 is simple

void setup() {
  lcd.begin(16, 2);
  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(GREEN_LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  myservo.attach(SERVO_PIN);
  myservo.write(0); // Initial servo position (locked)
  Serial.begin(9600); // For debugging

  // MODIFIED FOR EEPROM: Read saved password from EEPROM
  EEPROM.get(EEPROM_PASSWORD_ADDRESS, savedPassword);

  // Check if EEPROM contains a valid password (e.g., not all 0xFF, which is default for empty EEPROM)
  // If the first character is null or 0xFF, assume no password is set yet
  if (savedPassword[0] == '\0' || savedPassword[0] == 0xFF) {
    lcd.print("Set Password:"); // Show setup message on LCD
    isSettingPassword = true;  // Enable password setup mode
    currentPosition = 0;       // Reset the current position
  } else {
    lcd.print(" ENTER PASSWORD "); // Prompt user to enter the code
  }
}

void loop() {
  char key = keypad.getKey(); // Read key pressed on the keypad
  if (key) {
    // REMOVED: playKeyPressSound(); // Buzzer removed for every key press

    if (isSettingPassword) {
      handlePasswordSetup(key); // Handle password setup mode
    } else if (isCheckingMaster) {
      handleMasterPasswordCheck(key); // Handle master password verification
    } else {
      handlePasswordEntry(key); // Handle normal password entry
    }
  }
}


// Function to handle password setup
void handlePasswordSetup(char key) {
  lcd.clear(); // Clear LCD for updated display

  if (key == 'B') { // Delete the last character
    if (currentPosition > 0) {
      currentPosition--;
      enteredPassword[currentPosition] = '\0'; // Clear the character
    }
  } else if (key >= '0' && key <= '9' && currentPosition < PASSWORD_LENGTH) {
    // Add digit to entered password if less than PASSWORD_LENGTH digits
    enteredPassword[currentPosition] = key;
    currentPosition++;
  } else if (key == 'C') { // Confirm the entered password
    if (currentPosition == PASSWORD_LENGTH) { // Check if PASSWORD_LENGTH digits are entered
      enteredPassword[PASSWORD_LENGTH] = '\0'; // Null-terminate the string
      strncpy(savedPassword, enteredPassword, PASSWORD_LENGTH + 1); // Save new password
      EEPROM.put(EEPROM_PASSWORD_ADDRESS, savedPassword); // MODIFIED FOR EEPROM: Store in EEPROM
      lcd.print("Password Set!"); // Display success message
      delay(2000);
      isSettingPassword = false; // Exit password setup mode
      lcd.clear();
      lcd.print(" ENTER PASSWORD "); // Go back to normal mode
      currentPosition = 0;       // Reset position for new entries
    } else {
      lcd.print("Incomplete Pass"); // Warn user of incomplete entry
      delay(2000);
    }
    return;
  }

  // Display entered digits during password setup
  lcd.setCursor(0, 0);
  lcd.print("Set New Pass:");
  lcd.setCursor(0, 1);
  for (int i = 0; i < currentPosition; i++) {
    lcd.print(enteredPassword[i]); // Show the actual digits
  }
}

// Function to handle master password verification for password change
void handleMasterPasswordCheck(char key) {
  lcd.clear(); // Clear LCD for updated display

  if (key == 'B') { // Delete the last character
    if (currentPosition > 0) {
      currentPosition--;
      enteredPassword[currentPosition] = '\0'; // Clear the character
    }
  } else if (key >= '0' && key <= '9' && currentPosition < PASSWORD_LENGTH) {
    // Add digit to entered password if less than PASSWORD_LENGTH digits
    enteredPassword[currentPosition] = key;
    currentPosition++;
  } else if (key == 'C') { // Confirm the entered password
    if (currentPosition == PASSWORD_LENGTH) { // Check if PASSWORD_LENGTH digits are entered
      enteredPassword[PASSWORD_LENGTH] = '\0'; // Null-terminate the string
      if (strcmp(enteredPassword, masterPassword) == 0) {
        lcd.print("Master Verified"); // Display success message
        delay(2000);
        isCheckingMaster = false; // Exit master password check
        isSettingPassword = true; // Enter password setup mode
        currentPosition = 0;      // Reset position for new entries
        lcd.clear();
        lcd.print("Set Password:"); // Prompt to set new password
      } else {
        lcd.print("Incorrect Master"); // Display error message
        delay(2000);
        isCheckingMaster = false; // Exit master password check
        currentPosition = 0;      // Reset position
        lcd.clear();
        lcd.print(" ENTER PASSWORD "); // Go back to normal mode
      }
    } else {
      lcd.print("Incomplete Pass"); // Warn user of incomplete entry
      delay(2000);
    }
    return;
  }

  // Display asterisks during master password entry
  lcd.setCursor(0, 0);
  lcd.print("Master Pass:");
  lcd.setCursor(0, 1);
  for (int i = 0; i < currentPosition; i++) {
    lcd.print("*"); // Show asterisks for entered digits
  }
}

// Function to handle normal password entry
void handlePasswordEntry(char key) {
  if (key == 'A') { // Initiate password change (current user changing password)
    lcd.clear();
    lcd.print("Master Pass:");
    currentPosition = 0;      // Reset position
    isCheckingMaster = true;  // Enter master password check mode
    return;
  } else if (key == 'D') { // NEW: Initiate new password setting for another person
    lcd.clear();
    lcd.print("Set new user pass:"); // Prompt to set new password
    currentPosition = 0;
    // Clear enteredPassword buffer when entering setup mode
    memset(enteredPassword, '\0', sizeof(enteredPassword));
    isSettingPassword = true; // Enter password setup mode
    return;
  }
  else if (key == 'B') { // Delete the last character
    if (currentPosition > 0) {
      currentPosition--;
      enteredPassword[currentPosition] = '\0'; // Clear the character
    }
  } else if (key >= '0' && key <= '9' && currentPosition < PASSWORD_LENGTH) {
    // Add digit to entered password if less than PASSWORD_LENGTH digits
    enteredPassword[currentPosition] = key;
    currentPosition++;
  } else if (key == 'C') { // Confirm the entered password
    if (currentPosition == PASSWORD_LENGTH) { // Check if PASSWORD_LENGTH digits are entered
      enteredPassword[PASSWORD_LENGTH] = '\0'; // Null-terminate the string
      if (strcmp(enteredPassword, savedPassword) == 0) {
        unlockDoor();           // Unlock the door if password is correct
      } else {
        incorrectPassword();    // Indicate incorrect password
      }
      currentPosition = 0;      // Reset position for new entries
    } else {
      lcd.clear();
      lcd.print("Incomplete Pass"); // Warn user of incomplete entry
      delay(2000);
      lcd.clear();
      lcd.print(" ENTER PASSWORD "); // Go back to normal mode
    }
    return;
  }

  // Display asterisks during normal password entry
  lcd.clear();
  lcd.print("PASSWORD:");
  lcd.setCursor(0, 1);
  for (int i = 0; i < currentPosition; i++) {
    lcd.print("*"); // Show asterisks for entered digits
  }
}

// Function to unlock the door
void unlockDoor() {
  lcd.clear();
  lcd.print("Access Granted");
  digitalWrite(GREEN_LED_PIN, HIGH); // Turn on the green LED

  // Play success melody (no buzzer here anymore, but keeping structure)
  // Removed buzzer tone for correct password
  digitalWrite(GREEN_LED_PIN, LOW); // Turn off the green LED

  delay(2500); // Display "Access Granted" for 2.5 seconds

  // Unlock the door and show countdown timer
  myservo.write(90); // Unlock position
  for (int secondsLeft = 20; secondsLeft > 0; secondsLeft--) {
    lcd.clear();
    lcd.print("DOOR OPENS");
    lcd.setCursor(6, 1);
    lcd.print(secondsLeft);    // Show countdown timer
    lcd.print(" sec");
    delay(1000);
  }

  // Lock the door
  lcd.clear();
  lcd.print("DOOR CLOSED...");
  myservo.write(0);          // Lock position
  delay(1000);
  lcd.clear();
  lcd.print(" ENTER PASSWORD "); // Go back to normal mode
}

// Function to handle incorrect password entry
void incorrectPassword() {
  lcd.clear();
  lcd.print("CODE INCORRECT");
  digitalWrite(RED_LED_PIN, HIGH); // Turn on the red LED

  // Play alert melody - Buzzer only sounds here!
  for (int i = 0; i < 3; i++) {
    tone(BUZZER_PIN, 300, 150);
    delay(200);
    tone(BUZZER_PIN, 100, 150);
    delay(200);
  }
  noTone(BUZZER_PIN);
  digitalWrite(RED_LED_PIN, LOW); // Turn off the red LED

  delay(2500); // Display "CODE INCORRECT" for 2.5 seconds

  lcd.clear();
  lcd.print(" ENTER PASSWORD "); // Go back to normal mode
}
