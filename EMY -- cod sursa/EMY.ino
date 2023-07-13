// Include all nedeed libraries
#include <UTFT.h>
#include <URTouch.h>
#include <Wire.h>
#include <TimeLib.h>
#include <DS1307RTC.h>
#include <Servo.h>

// Define the 2 variables : step and direction for the stepper motor
#define DIRECTION 8 // assigned pin is PWM pin 8
#define STEP 9 // assigned pin is PWM pin 9 

// Initialize all the components
UTFT LCD(ILI9341_16, 38, 39, 40, 41);
URTouch Touch(6, 5, 4, 3, 2);
Servo servoMotor;

// Create a data structer that stores the pozition of a compartiment, if it is filled or not and its release time 
struct compartiment {
  bool filled; // 1 -- FILLED / 0 -- EMPTY
  int poz; // pozition
  int h, m; // release time
};

// Declare an one dimensional array that stores all the data for the 12 compariments
compartiment C[12];

// Declare 2 variables for adjusting what appears on screen 5
int first = 0, last = 4;

// Declare the fonts for the TFT screen
extern uint8_t BigFont[];
extern uint8_t SmallFont[]; 

// Declare the months as a constant so they can be accessed by the index
const char *monthName[13] = { "NULL",
  "Jan", "Feb", "Mar", "Apr", "May", "Jun",
  "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

// Declare a variable to know which screen the LCD needs to print
int SCREEN = 1;

// Declare 2 variables to know the exact position where the LCD was pressed
int xT, yT;

// Declare a few variables for storing the wheel numbers
int prevH = 23, currH = 0, nextH = 1, prevM = 59, currM = 0, nextM = 1;

// Declare the data structure for the RTC (stores sec,min,h,d,mo,y)
tmElements_t TIME;

// The motors steps (1600) do not divide exactly to the number of compartiments (12) so we  will use an adjustment (133*12+4 = 1600)
int adjustment = 1;

//----------------------//
//-- Custom functions --//
//----------------------//

// Function that adjust the format of the time if the value is one digit (BigFont)
void print2digitsB(int value, int x, int y) {
  if (value < 10) {
    LCD.printNumI(0, x, y);
    LCD.printNumI(value, x+15, y);  
  } else {
    LCD.printNumI(value, x, y);
  }
}

// Function that adjust the format of the time if the value is one digit (SmallFont)
void print2digitsS(int value, int x, int y) {
  if (value < 10) {
    LCD.printNumI(0, x, y);
    LCD.printNumI(value, x+7, y);  
  } else {
    LCD.printNumI(value, x, y);
  }
}

// Function that prepares the LCD for display
void prepLCD(int br, int bg, int bb, int cr, int cg, int cb, char fontSize[] ) {
  LCD.setBackColor(br, bg, bb);
  LCD.setColor(cr, cg, cb);
  if (strcmp(fontSize, "BigFont") == 0) { 
    LCD.setFont(BigFont);
  } else if (strcmp(fontSize, "SmallFont") == 0) { 
    LCD.setFont(SmallFont);
  }
}

// Function that tells if a button was pressed 
bool buttonPressed(int xc, int yc, int xst, int yst, int xdr, int ydr) {
  return (xc >= xst && xc <= xdr && yc >= yst && yc <= ydr); 
}

// Function that prints the time and date
void printTimeDate() {
  // Prepare the LCD for the date & time
  prepLCD(0, 0, 0, 255, 255, 255, "BigFont");
  
  // Check if the RTC is working
  if (RTC.read(TIME)) {
    // Print the time
    print2digitsB(TIME.Hour, 110, 55);
    LCD.print(":", 150, 55);
    print2digitsB(TIME.Minute, 170, 55);

    // Print the date
    print2digitsB(TIME.Day, 70, 95);
    LCD.print(monthName[TIME.Month], 117, 95);
    LCD.printNumI(tmYearToCalendar(TIME.Year), 180, 95);
  }
}

// Functions that prints UP-ARROW
void printUP_ARROW(int x, int y) {
  LCD.fillRect(x, y, x+30, y-2); // 65 90
  LCD.fillRect(x+2, y-2, x+28, y-4);
  LCD.fillRect(x+4, y-4, x+26, y-6);
  LCD.fillRect(x+6, y-6, x+24, y-8);
  LCD.fillRect(x+8, y-8, x+22, y-10);
  LCD.fillRect(x+10, y-10, x+20, y-12);
  LCD.fillRect(x+12, y-12, x+18, y-14);
  LCD.fillRect(x+14, y-14, x+16, y-16);  
}

// Functions that prints DOWN-ARROW
void printDOWN_ARROW(int x, int y) {
  LCD.fillRect(x, y-14, x+30, y-16); 
  LCD.fillRect(x+2, y-12, x+28, y-14);
  LCD.fillRect(x+4, y-10, x+26, y-12);
  LCD.fillRect(x+6, y-8, x+24, y-10);
  LCD.fillRect(x+8, y-6, x+22, y-8);
  LCD.fillRect(x+10, y-4, x+20, y-6);
  LCD.fillRect(x+12, y-2, x+18, y-4);
  LCD.fillRect(x+14, y, x+16, y-2);  
}

// Function that prints BACK-ARROW
void printBACK_ARROW() {
  LCD.fillRect(5, 10, 7, 12);
  LCD.fillRect(7, 8, 14, 14);
  LCD.fillRect(11, 6, 15, 16);
  LCD.fillRect(15, 4, 19, 18);
  LCD.fillRect(19, 2, 23, 20);
  LCD.fillRect(23, 6, 43, 16);
  LCD.fillRect(35, 8, 45, 28);
  LCD.fillRect(37, 28, 43, 30);
}

// Functions that prints the numbers wheel
void printNumbWheel() {
  prepLCD(0, 0, 0, 255, 255, 255, "BigFont");
  print2digitsB(currH, 107, 105);
  print2digitsB(currM, 187, 105);
  prepLCD(0, 0, 0, 255, 255, 255, "SmallFont");
  print2digitsS(prevH, 118, 80);
  print2digitsS(nextH, 118, 135);
  print2digitsS(prevM, 198, 80);
  print2digitsS(nextM, 198, 135);
}

// Function that updates the number wheel
void updateNumbWheel(int xT, int yT) {
  // Hour section
  if (buttonPressed(xT, yT, 65, 90, 95, 106)) {
    if (prevH == 23) prevH = 0;
    else prevH++;
    if (currH == 23) currH = 0;
    else currH++;
    if (nextH == 23) nextH = 0;
    else nextH++;
  }
  if (buttonPressed(xT, yT, 65, 136, 95, 152)) {
    if (prevH == 0) prevH = 23;
    else prevH--;
    if (currH == 0) currH = 23;
    else currH--;
    if (nextH == 0) nextH = 23;
    else nextH--;
  }
  // Minute section
  if (buttonPressed(xT, yT, 230, 90, 260, 106)) {
    if (prevM == 59) prevM = 0;
    else prevM++;
    if (currM == 59) currM = 0;
    else currM++;
    if (nextM == 59) nextM = 0;
    else nextM++;
  }
  if (buttonPressed(xT, yT, 230, 136, 260, 152)) {
    if (prevM == 0) prevM = 59;
    else prevM--;
    if (currM == 0) currM = 59;
    else currM--;
    if (nextM == 0) nextM = 59;
    else nextM--;
  }
  delay(200);
}

// Function that searches for a empty compartiment
int emptyComp () {
  for (int i = 0; i < 12; i++)
    if (C[i].filled == 0)
      return i;
  return -1; 
}

// Function that adds data to the array and resets the variables for the number wheel
void addPill(int H, int M, int poz) {
   C[poz].h = H;
   C[poz].m = M;
   C[poz].filled = 1;
   prevH = 23; currH = 0; nextH = 1; prevM = 59; currM = 0; nextM = 1;
}

// Function that compares two compartiment type variables
int comp(compartiment a, compartiment b) {
  if (a.h > b.h) {
    return 1;
  } if (a.h == b.h) {
    if (a.m > b.m) {
      return 1;
    } else if (a.m < b.m){
      return 0;
    } else return 1;
  } else return 0;
}

// Function that swaps 2 elements int the compartiment array
void swapC(int poz1, int poz2) {
   int aux1 = C[poz1].h, aux2 = C[poz1].m, aux3 = C[poz1].poz, aux4 = C[poz1].filled;
   C[poz1].h = C[poz2].h; C[poz1].m = C[poz2].m; C[poz1].poz = C[poz2].poz; C[poz1].filled = C[poz2].filled;
   C[poz2].h = aux1; C[poz2].m = aux2; C[poz2].poz = aux3; C[poz2].filled = aux4;
}

// Function that sorts the elements of an array in descending order
void sort(compartiment arr[], int lenght) {
  int i, j;
  bool swapped;
  do{
    swapped = false;
    for (i = 0; i < lenght-1; i++) {
      if (comp(C[i], C[i+1]) == 0) {
        swapC(i, i+1);
        swapped = true;
      }
    }
  } while (swapped == true);
}

// Function that prints the timer text
void printTimerText(int pozY, int i) {
  LCD.print("MED ", 20, pozY);
  LCD.printNumI(i, 80, pozY);
  if (i > 9)
    LCD.print(".", 110, pozY);
  else
    LCD.print(". ", 93, pozY);
}

// Function that calculates the timer to a specific pill
void calcTimer(int i, int &hRemaining, int &mRemaining) {
    // Check if the RTC is working
    if (RTC.read(TIME)) {
      // Calculate the timer
      hRemaining = C[i].h - TIME.Hour;
      mRemaining = C[i].m - TIME.Minute;
      if (mRemaining < 0) {
        mRemaining += 60;
        hRemaining --;
      }
      if (hRemaining < 0) hRemaining += 24;
    }
}

// Function that prints the timer to a specific pill
void printTimer (int pozY, int i) {
  // Variables to store the timer data
  int mRemaining = 0, hRemaining = 0;
  if (C[i].filled == 1) {
      // Calculate the timer
      calcTimer(i, hRemaining, mRemaining);
      // Print the timer
      print2digitsB(hRemaining, 135, pozY);
      LCD.print(":", 165, pozY);
      print2digitsB(mRemaining, 180, pozY);
  } else LCD.print(" GOL  ", 130, pozY);
}

// Function that prints the schedule
void printSchedule (int first, int last) {
  int yPoz = 50;
  prepLCD(0, 0, 0, 255, 255, 255, "BigFont");
  for (first; first <= last; first++) {
    printTimerText(yPoz, first+1);
    printTimer(yPoz, first);
    yPoz += 40;
  }
}

// Function that updates the med wheel
void updateMedWheel() {
  if (buttonPressed(xT, yT, 260, 100, 290, 116) && first >= 1) {
    first--;
    last--;
  }
  if (buttonPressed(xT, yT, 260, 152, 290, 168) && last <= 10) { 
    first++;
    last++;
  }
}

// Function that rotates the motor
void rotateMotor(int numbOfComp) {
  for (int i = 1; i <= 133*numbOfComp + adjustment/3; i++) {
    digitalWrite(9, HIGH);
    delay(1);
    digitalWrite(9, LOW);
    adjustment++;
    if (adjustment > 3)
    adjustment = 1;
  }
  for (int i = 1; i <= numbOfComp; i ++) {
    for (int j = 0; j < 12; j++) {
      C[j].poz++;
      if (C[j].poz > 12)
        C[j].poz = 1;
    }
  }
}

// Function that checks for empty compartiments
bool checkForEmpty() {
  for (int i = 0; i < 12; i++) {
    if (C[i].filled == 0) {
      if (C[i].poz != 1)
        rotateMotor(13-C[i].poz);
      return 1;
    } 
  }
  return 0;
}

// Function that checks if one of the timers is 00.00 
void checkTimer() {
  for (int i = 0; i < 12; i++) {
    if (C[i].m == TIME.Minute && C[i].h == TIME.Hour && C[i].filled == 1) { 
      if (C[i].poz <= 7)
        rotateMotor(7-C[i].poz);
      else
        rotateMotor(19-C[i].poz);
    C[i].m = 0; C[i].h = 0; C[i].filled = 0;
    servoMotor.write(25);
    sort(C, 12);
    delay(5000);
    servoMotor.write(119);
    }
  }
}

// Function that draws screen 1 (HOME SCREEN)
void drawScreen1() {
  // Print the time and date
  printTimeDate();
  
  // Draw the button
  LCD.fillRect(40, 145, 279, 185);

  // Prepare the LCD for the text 
  prepLCD(255, 255, 255, 0, 0, 0, "BigFont");

  // Print the text
  LCD.print("MENIU", CENTER, 158);
}

// Function that draws screen 2 (MENU SCREEN)
void drawScreen2() {
  // Prepare the LCD for the buttons
  prepLCD(0, 0, 0, 255, 255, 255, "BigFont");

  // Print the retun arrow at the top left of the screen
  printBACK_ARROW();

  // Draw the buttons
  LCD.fillRect(40, 60, 279, 100);
  LCD.fillRect(40, 140, 279, 180);

  // Prepare the LCD for the text
  prepLCD(255, 255, 255, 0, 0, 0, "BigFont");

  // Print the text
  LCD.print("ADAUGA MED.", CENTER, 73);
  LCD.print("PROGRAM", CENTER, 153);
}

// Function that draws screen 3 (INSERTION SCREEN)
void drawScreen3() {
  // Prepare the LCD for the top text
  prepLCD(0, 0, 0, 255, 255, 255, "BigFont");

  // Print the top text
  LCD.print("INTRODUCE-TI MED.", CENTER, 55);
  LCD.print("IN ORIFICIU", CENTER, 75);
  
  // Prepare the LCD for the button
  prepLCD(0, 0, 0, 255, 255, 255, "BigFont");

  // Print the retun arrow at the top left of the screen
  printBACK_ARROW();

  // Draw the button
  LCD.fillRect(40, 130, 279, 170);

  // Prepare the LCD for the button text
  prepLCD(255, 255, 255, 0, 0, 0, "BigFont");

  // Print the button text
  LCD.print("GATA", CENTER, 143);
}

// Function that draws screen 4 (TIMER SCREEN)
void drawScreen4() {
  // Prepare the LCD for the top text
  prepLCD(0, 0, 0, 255, 255, 255, "BigFont");

  // Print the text
  LCD.print("SETA-TI ORA", CENTER, 20);
  LCD.print(":", 155, 105);

  // Print the numbers
  printNumbWheel();
  
  // Print the arrows
  printBACK_ARROW();
  printUP_ARROW(65, 90);
  printUP_ARROW(230, 90);
  printDOWN_ARROW(65, 152);
  printDOWN_ARROW(230, 152);

  // Draw the button
  LCD.fillRect(40, 190, 279, 230);

  // Prepare the LCD for the button text
  prepLCD(255, 255, 255, 0, 0, 0, "BigFont");

  // Print the button text
  LCD.print("GATA", CENTER, 203);
}

// Function that draws screen 5 (SCHEDULE SCREEN)
void drawScreen5() {
  prepLCD(0, 0, 0, 255, 255, 255, "BigFont");
  printBACK_ARROW();
  printSchedule(first, last);
  prepLCD(0, 0, 0, 0, 0, 0, "BigFont");
  LCD.fillRect(0, 230, 319, 240);
  prepLCD(0, 0, 0, 255, 255, 255, "BigFont");
  printUP_ARROW(260, 105);
  printDOWN_ARROW(260, 167);
}

// Function that tells the user the dispenser id full
void fullError() {
    // Prepare the LCD
    prepLCD(0, 0, 0, 255, 0, 0, "BigFont");
    LCD.clrScr();

    // Flash the message
    for (int i=1; i<=3; i++) {
      LCD.print("DISPENSER PLIN!", CENTER, 120);
      delay(700);
      LCD.clrScr();
      delay(100);
    }
}

void setup() {
  // Attach the servo to a pin and set it to the initial position
  servoMotor.attach(10);
  servoMotor.write(119);

  // Initialize the LCD and clear it and rotate it
  LCD.InitLCD();
  LCD.clrScr();

  // Initialize the Touch and set its precision
  Touch.InitTouch();
  Touch.setPrecision(PREC_MEDIUM);

  // Set the correct pinmode to the moor PWM pins
  pinMode(8, OUTPUT);
  pinMode(9, OUTPUT);

  // Set the rotation direction and reset the step pin
  digitalWrite(8, HIGH);
  digitalWrite(9, LOW);

  // Set the position for all the compartiments
  for (int i = 0; i < 12; i++)
    C[i].poz = i+1;
}

void loop() {
  // Print the coresponding screen
  switch (SCREEN) {
    case 1:
      drawScreen1();
      while (SCREEN == 1) {
        // Check if one of the pills needs to drop
        checkTimer();  
        // Update the date 
        printTimeDate();
        // Check if the button was pressed and if yes switch to screen 2
        if(Touch.dataAvailable()) {
          Touch.read();
          xT = 319 - Touch.getX();
          yT = 269 - Touch.getY();
          if (buttonPressed(xT, yT, 40, 145, 279, 185) == 1) {
           SCREEN = 2;
           LCD.clrScr();
           break;
          }
        }
      }
      break;
    case 2: 
      drawScreen2();
      while (SCREEN == 2) {
        // Check if one of the pills needs to drop
        checkTimer();  
        // Check if one of the buttons was pressed and if yes switch to the coresponding screen
        if(Touch.dataAvailable()) {
          Touch.read();
          xT = 319 - Touch.getX();
          yT = 269 - Touch.getY();
          if (buttonPressed(xT, yT, 40, 60, 279, 100)) {
           if (checkForEmpty()) {
            SCREEN = 3;
            LCD.clrScr();
            break;
           } else {
            fullError();
            SCREEN = 2;
            LCD.clrScr();
            break;
           }
          }
          if (buttonPressed(xT, yT, 40, 140, 279, 180)) {
           SCREEN = 5;
           LCD.clrScr();
           break;
          }
          if (buttonPressed(xT, yT, 5, 2, 45, 65)) {
            SCREEN = 1;
            LCD.clrScr();
            break;
          }
        }
      }
      break;
    case 3:
      drawScreen3();
      while (SCREEN == 3) {
        // Check if one of the pills needs to drop
        checkTimer();  
        // Check if the button was pressed and if yes switch to screen 4
        if(Touch.dataAvailable()) {
          Touch.read();
          xT = 319 - Touch.getX();
          yT = 269 - Touch.getY();
          if (buttonPressed(xT, yT, 40, 130, 279, 170)) {
           SCREEN = 4;
           LCD.clrScr();
           break;
          }
          if (buttonPressed(xT, yT, 5, 2, 45, 65)) {
            SCREEN = 2;
            LCD.clrScr();
            break;
          }
        }
      }
      break;
    case 4:
      drawScreen4();
      while (SCREEN == 4) {
        // Check if one of the pills needs to drop
        checkTimer();  
        // Print the number wheel
        printNumbWheel();
        // Check if the button was pressed and if yes switch to screen 2
        if (Touch.dataAvailable()) {
          Touch.read();
          xT = 319 - Touch.getX();
          yT = 269 - Touch.getY();
          // Update the number wheel
          updateNumbWheel(xT, yT);
          if (buttonPressed(xT, yT, 40, 190, 279, 230)) {
            addPill(currH, currM, emptyComp()); 
            sort(C, 12);
            SCREEN = 2;
            LCD.clrScr();
            break; 
          }
          if (buttonPressed(xT, yT, 5, 2, 45, 65)) {
            SCREEN = 3;
            LCD.clrScr();
            break;
          }
        }
      }
      break;
    case 5:
      drawScreen5();
      // Check if the button was pressed and if yes switch to screen 2
      if (Touch.dataAvailable()) {
        Touch.read();
        xT = 319 - Touch.getX();
        yT = 269 - Touch.getY();
        if (buttonPressed(xT, yT, 5, 2, 45, 65)) {
          SCREEN = 2;
          LCD.clrScr();
          break;
        }
        updateMedWheel();
      }
      // Check if one of the pills needs to drop
      checkTimer();        
      break;
  }
}
