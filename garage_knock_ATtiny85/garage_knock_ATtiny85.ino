// Pin definitions
const int knockSensor = 2;         //5 Piezo sensor on pin 0.
const int programSwitch = 2;       //2 If this is high we program a new code.
const int lockMotor = 1;           //5 Gear motor used to turn the lock.
const int greenLED = 0;            //3 Status LED
int potPin = 3;                    //4
 
// Tuning constants.  Could be made vars and hoooked to potentiometers for soft configuration, etc.
int threshold = 0;           // Minimum signal from the piezo to register as a knock
const int rejectValue = 25;        // If an individual knock is off by this percentage of a knock we don't unlock..
const int averageRejectValue = 15; // If the average timing of the knocks is off by this percent we don't unlock.
const int knockFadeTime = 150;     // milliseconds we allow a knock to fade before we listen for another one. (Debounce timer.)
const int lockTurnTime = 1000;      // milliseconds that we run the motor to get it to go a half turn.

const int maximumKnocks = 20;       // Maximum number of knocks to listen for.
int knockComplete = 2000;     // Longest time to wait for a knock before we assume that it's finished.


// Variables.
int secretCode[maximumKnocks] = {50, 25, 25, 50, 100, 50, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};  // Initial setup: "Shave and a Hair Cut, two bits."
int knockReadings[maximumKnocks];   // When someone knocks this array fills with delays between knocks.
int knockSensorValue = 0;           // Last reading of the knock sensor.
int programButtonPressed = false;   // Flag so we remember the programming button setting at the end of the cycle.

int state = 0;
unsigned long BtDnTm = 0;
int previous = 0;
unsigned long ptime = 0;
const int timehold = 2000;



void setup() {
  pinMode(lockMotor, OUTPUT);
  pinMode(greenLED, OUTPUT);
  pinMode(programSwitch, INPUT);

}

void loop() {
  knockSensorValue = analogRead(knockSensor);    // Listen for any knock at all.
  
  threshold = analogRead(potPin) + 1;
  
  state = digitalRead(programSwitch);
  
  ledon();
  
  
  
  if(state == true && previous == 0){
      previous = 1;
      BtDnTm = millis();
    }
    
    
    
  if(state == false && previous == 1 && millis()-BtDnTm < timehold && BtDnTm + 30 < millis()){
      ledlow();
      previous = 0;
      BtDnTm = 0;
      triggerDoorUnlock();
    }
    
    
    
    
    

if(state == true && previous == 1 && millis()-BtDnTm >= timehold){
  ledhigh();
  programButtonPressed = true;
  knockComplete = 5000;
  ptime = millis();

  while(millis() - ptime < knockComplete){
    knockSensorValue = analogRead(knockSensor);
    if(knockSensorValue >=threshold){
      listenToSecretKnock();
    }
  }
previous = 0;
BtDnTm = 0;
ptime = 0;
knockComplete = 2000;
}

  
  
    
    
    
    
  
if(digitalRead(programSwitch)==LOW){
  previous = 0;
  BtDnTm = 0;
  ptime = 0;
  programButtonPressed = false;
  ledon();
}
  
if(knockSensorValue >=threshold){
  listenToSecretKnock();
}

}



// Records the timing of knocks.
void listenToSecretKnock(){

  


  int i = 0;
  // First lets reset the listening array.
  for (i=0;i<maximumKnocks;i++){
    knockReadings[i]=0;
  }
  
  int currentKnockNumber=0;         			// Incrementer for the array.
  int startTime=millis();           			// Reference for when this knock started.
  int now=millis();
  
     			// we blink the LED for a bit as a visual indicator of the knock.
  if (programButtonPressed==true){
     ledhigh();                         // and the red one too if we're programming a new knock.
  }else{
    ledoff();
  }
  delay(knockFadeTime);                       	        // wait for this peak to fade before we listen to the next one.
  if (programButtonPressed==true){
     ledon();                         // and the red one too if we're programming a new knock.
  }else{
    ledon();
  }
  do {
    //listen for the next knock or wait for it to timeout. 
    knockSensorValue = analogRead(knockSensor);
    if (knockSensorValue >=threshold){                   //got another knock...
      //record the delay time.
      now=millis();
      knockReadings[currentKnockNumber] = now-startTime;
      currentKnockNumber ++;                             //increment the counter
      startTime=now;          
      // and reset our timer for the next knock
      if (programButtonPressed==true){
     ledhigh();                         // and the red one too if we're programming a new knock.
  }else{
    ledoff();
  }
  delay(knockFadeTime);                       	        // wait for this peak to fade before we listen to the next one.
  if (programButtonPressed==true){
     ledon();                         // and the red one too if we're programming a new knock.
  }else{
    ledon();
  }
    }

    now=millis();
    
    //did we timeout or run out of knocks?
  } while ((now-startTime < knockComplete) && (currentKnockNumber < maximumKnocks));
  
  //we've got our knock recorded, lets see if it's valid
  if (programButtonPressed==false){             // only if we're not in progrmaing mode.
    if (validateKnock() == true){
      triggerDoorUnlock(); 
    } else {
      ledflash();
    }
  } else { // if we're in programming mode we still validate the lock, we just don't do anything with the lock
    validateKnock();
    // and we blink the green and red alternately to show that program is complete.
    knockComplete = 2000;
    previous = 0;
    BtDnTm = 0;
    ledflashNLS();
    ledon();
  }
}


// Runs the motor (or whatever) to unlock the door.
void triggerDoorUnlock(){
  int i=0;
  
  // turn the motor on for a bit.
  digitalWrite(lockMotor, HIGH);
  ledlow();            // And the green LED too.
  
  delay (lockTurnTime);                    // Wait a bit.
  
  digitalWrite(lockMotor, LOW);            // Turn the motor off.
  
  // Blink the green LED a few times for more visual feedback. 
  ledon();
   
}

// Sees if our knock matches the secret.
// returns true if it's a good knock, false if it's not.
// todo: break it into smaller functions for readability.
boolean validateKnock(){
  int i=0;
 
  // simplest check first: Did we get the right number of knocks?
  int currentKnockCount = 0;
  int secretKnockCount = 0;
  int maxKnockInterval = 0;          			// We use this later to normalize the times.
  
  for (i=0;i<maximumKnocks;i++){
    if (knockReadings[i] > 0){
      currentKnockCount++;
    }
    if (secretCode[i] > 0){  					//todo: precalculate this.
      secretKnockCount++;
    }
    
    if (knockReadings[i] > maxKnockInterval){ 	// collect normalization data while we're looping.
      maxKnockInterval = knockReadings[i];
    }
  }
  
  // If we're recording a new knock, save the info and get out of here.
  if (programButtonPressed==true){
      for (i=0;i<maximumKnocks;i++){ // normalize the times
        secretCode[i]= map(knockReadings[i],0, maxKnockInterval, 0, 100); 
      }
      // And flash the lights in the recorded pattern to let us know it's been programmed.
      ledlow();
      delay(1000);
      ledhigh();
      delay(50);
      for (i = 0; i < maximumKnocks ; i++){
        ledlow();
        // only turn it on if there's a delay
        if (secretCode[i] > 0){                                   
          delay( map(secretCode[i],0, 100, 0, maxKnockInterval)); // Expand the time back out to what it was.  Roughly. 
          ledhigh();
        }
        delay(50);
      }
	  return false; 	// We don't unlock the door when we are recording a new knock.
  }
  
  if (currentKnockCount != secretKnockCount){
    return false; 
  }
  
  /*  Now we compare the relative intervals of our knocks, not the absolute time between them.
      (ie: if you do the same pattern slow or fast it should still open the door.)
      This makes it less picky, which while making it less secure can also make it
      less of a pain to use if you're tempo is a little slow or fast. 
  */
  int totaltimeDifferences=0;
  int timeDiff=0;
  for (i=0;i<maximumKnocks;i++){ // Normalize the times
    knockReadings[i]= map(knockReadings[i],0, maxKnockInterval, 0, 100);      
    timeDiff = abs(knockReadings[i]-secretCode[i]);
    if (timeDiff > rejectValue){ // Individual value too far out of whack
      return false;
    }
    totaltimeDifferences += timeDiff;
  }
  // It can also fail if the whole thing is too inaccurate.
  if (totaltimeDifferences/secretKnockCount>averageRejectValue){
    return false; 
  }
  return true; 
}




void ledhigh(){
  analogWrite(greenLED, 100);
}


void ledlow(){
  analogWrite(greenLED, 1); 
}  

void ledon(){
  analogWrite(greenLED, 10);  
}


void ledoff(){
  analogWrite(greenLED, 0);
}


void ledflash(){
  int i = 0;
for (i=0;i<5;i++){					
  ledhigh();
  delay(100);
  ledlow();
  delay(100);
}
}


void ledflashNLS(){ 
  // fade in from min to max in increments of 5 points:
  for(int fadeValue = 0 ; fadeValue <= 255; fadeValue +=5) { 
    // sets the value (range from 0 to 255):
    analogWrite(greenLED, fadeValue);         
    // wait for 30 milliseconds to see the dimming effect    
    delay(30);                            
  }


  // fade out from max to min in increments of 5 points:
  for(int fadeValue = 255 ; fadeValue >= 0; fadeValue -=5) { 
    // sets the value (range from 0 to 255):
    analogWrite(greenLED, fadeValue);         
    // wait for 30 milliseconds to see the dimming effect    
    delay(30);                            
  }
}
