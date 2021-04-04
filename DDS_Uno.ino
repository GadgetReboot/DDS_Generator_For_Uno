// Arduino Uno/Mega simple DDS Waveform Generator Demo
// uses 8 bit R-2R DAC on pins 2-9
// tested on Arduino Mega
//
// Generates waveforms at chosen frequency using a 9060 Hz Sample Rate
//
// DDS generator equation:   fOut = (M (sampleRate)) / (2^32)
//
//              where        fOut = target output frequency
//                              M = tuningWord
//                     sampleRate = 9060 Hz (one sample approx. every 110.375 uSec)
//                           2^32 = highest count of a 32 bit phase accumulator
// Gadget Reboot

uint32_t phAcc = 0;                   // phase accumulator / counter counts up and rolls over to 0
uint8_t dacVal = 0;                   // data to send to DAC
volatile boolean sendSample = false;  // flag to send a sample to the DAC when interrupt occurs

#define fOut 130.5                                // target frequency to generate in Hz
uint32_t tuningWord = pow(2, 32) * fOut / 9060.0; // DDS tuning word for target frequency

// Sine wave look up table
// 256 samples of 8 bit sine wave data points
// 256 samples are accessed as table index 0 to 255
// which requires an 8 bit number to address.
// each 8 bit sample in the table ranges in value from 0 to 255
// to send to the DAC, generating a voltage from 0v - 5v
// the signal begins in the center (zero line) at 128,
// rises to max amplitude near 255, falls to minimum 0,
// and ends back near the center line, ready to repeat
uint8_t LUT[] = {128, 131, 134, 137, 140, 143, 146, 149, 152, 155, 158, 162, 165, 167, 170, 173, 176, 179, 182, 185, 188, 190, 193, 196, 198, 201, 203, 206, 208, 211, 213, 215,
                 218, 220, 222, 224, 226, 228, 230, 232, 234, 235, 237, 238, 240, 241, 243, 244, 245, 246, 248, 249, 250, 250, 251, 252, 253, 253, 254, 254, 254, 255, 255, 255,
                 255, 255, 255, 255, 254, 254, 254, 253, 253, 252, 251, 250, 250, 249, 248, 246, 245, 244, 243, 241, 240, 238, 237, 235, 234, 232, 230, 228, 226, 224, 222, 220,
                 218, 215, 213, 211, 208, 206, 203, 201, 198, 196, 193, 190, 188, 185, 182, 179, 176, 173, 170, 167, 165, 162, 158, 155, 152, 149, 146, 143, 140, 137, 134, 131,
                 128, 124, 121, 118, 115, 112, 109, 106, 103, 100, 97, 93, 90, 88, 85, 82, 79, 76, 73, 70, 67, 65, 62, 59, 57, 54, 52, 49, 47, 44, 42, 40,
                 37, 35, 33, 31, 29, 27, 25, 23, 21, 20, 18, 17, 15, 14, 12, 11, 10, 9, 7, 6, 5, 5, 4, 3, 2, 2, 1, 1, 1, 0, 0, 0,
                 0, 0, 0, 0, 1, 1, 1, 2, 2, 3, 4, 5, 5, 6, 7, 9, 10, 11, 12, 14, 15, 17, 18, 20, 21, 23, 25, 27, 29, 31, 33, 35,
                 37, 40, 42, 44, 47, 49, 52, 54, 57, 59, 62, 65, 67, 70, 73, 76, 79, 82, 85, 88, 90, 93, 97, 100, 103, 106, 109, 112, 115, 118, 121, 124
                };

void setup()
{
  // set 8 consecutive pins [2 through 9] as DAC outputs
  for (byte i = 2; i <= 9; i++)
  {
    pinMode(i, OUTPUT);
  }

  //set timer1 interrupt for 9060 Hz @ 16 MHz clock, no prescale
  cli();
  TCCR1A = 0;    // clear register
  TCCR1B = 0;    // clear register
  TCNT1  = 0;    // initialize counter value to 0
  OCR1A = 1766;  // 16MHz / 1766  = 9060 Hz
  TCCR1B |= (1 << WGM12) | (1 << CS10);  // turn on CTC mode, Set CS10 for 1 prescaler
  TIMSK1 |= (1 << OCIE1A);               // enable timer compare interrupt
  sei();
}

void loop()
{
  // send a sample to the DAC when interrupt occurs
  if (sendSample)
  {
    uint8_t count = (phAcc >> 24);       // take the 8 upper bits of the 32 bit phase accumulator as a LUT sample pointer

    // dacVal = count;                   //   ramp wave:  send counter value to the DAC to generate a rising ramp
    // dacVal = (count > 127) ? 255 : 0; // square wave:  send all 0 or all 1 to the DAC for each half of a wave cycle
    dacVal = LUT[count];                 //   sine wave:  send look up table sample value to the DAC

    // send calculated sine to the DAC
    // uint8_t sineCount = (uint8_t) ((1 + sin(((2.0 * PI) / 256) * count)) * 127.5);
    // dacVal = sineCount;

    // send the 8 bit counter data to the DAC pins
    for (byte i = 0; i <= 7; i++)
    {
      digitalWrite(i + 2, bitRead(dacVal, i));
    }

    phAcc += tuningWord; // increment phase accumulator/counter
    sendSample = false;  // clear interrupt flag
  }
}

// timer1 interrupt 9060 Hz
// set a flag so the main loop can process a sample
ISR(TIMER1_COMPA_vect)
{
  sendSample = true;
}
