/**
 * @file      esp32-self-triggered-irq-pwm-18KHz-potentiometer-pushpull.ino
 * @author    cybernetic-research
 * @brief     A simple open loop SPWM generator intended to be used to 
 *            generate ac at 50 or 60Hz. This program outputs SPWM at 18KHz for 
 *            50hz, or 21600Hz for 60Hz. the PWM output is wired back into the
 *            chip as a rising edge interrupt allowing a new PWM value to be
 *            output every edge transition. An ADC is used to read a 
 *            potentiometer allowing the PWM output voltage to be raised or
 *            lowered (because we scale the sine lookup table).
 *            This program is meant to be used on ESP32 boards  
 *            
 *            This version is for a push pull configuration with centre tapped
 *            transformers
 *            
 * @version   0.1
 * @date      2020-09-19
 * 
 * @copyright Copyright (c) 2020
 * 
 */
#include <stdint.h>
//pinouts here: https://www.bing.com/images/search?view=detailV2&ccid=L%2bau9S29&id=5019621DC7350272217DF7C36EE9AD869B1CE64D&thid=OIP.L-au9S29i6JYHjCKPQ5WzQHaEk&mediaurl=https%3a%2f%2flastminuteengineers.com%2fwp-content%2fuploads%2f2018%2f08%2fESP32-Development-Board-Pinout.jpg&exph=468&expw=758&q=esp32+arduino+pinout&simid=607997902785809138&ck=FCAD5061950FE300E30F4A7416EE3687&selectedIndex=1&FORM=IRPRST&ajaxhist=0

int32_t     potValue    =   0;  // variable for storing the potentiometer value
int32_t     brightness  =   0;  // how bright the LED is
uint8_t     pwm         =   1;

#define PUSHPULL_DRIVE_1    33
#define PUSHPULL_DRIVE_2    25
#define SELF_TRIGGERING_IRQ 0
#define DRIVE1_PWM          1
#define DRIVE2_PWM          2
#define CYCLE_START_PIN     34 //TODO look if appropriate

const byte  led_gpio    =   32;       // the PWM pin the LED is attached to
const int   potPin      =   27;       // Potentiometer is connected to GPIO 27 (Analog ADC1_CH6) 
int32_t PWM_FEEDBACK_PIN =  18;       // connect a wire from GPIO 18 to GPIO 32 !!!!!!
int cycleStartTriggerd = 0;
int phaseOffset = 0; //TODO look if appropriate

//8-bit representation of a sinewave scaled to 0->255

#define FULL_BRIDGE             1       // change this to "1" if you want to drive a full bridge

//360 step
#define MODULATING_FREQ         18000   // <-- 18KHz is 360 x 50 Hz for American 60Hz use number 21600 instead
#define MAX_STEPS               360     //  one pwm update per degree of mains sine
#define TRANSISTOR_SWITCH_STEP  179     // when to use other transistor bank

#if FULL_BRIDGE
///
/// Full Bridge Sine Wave Table
///
int32_t sinetable[]= 
{
0 ,
4 ,
8 ,
13  ,
17  ,
22  ,
26  ,
31  ,
35  ,
39  ,
44  ,
48  ,
53  ,
57  ,
61  ,
65  ,
70  ,
74  ,
78  ,
83  ,
87  ,
91  ,
95  ,
99  ,
103 ,
107 ,
111 ,
115 ,
119 ,
123 ,
127 ,
131 ,
135 ,
138 ,
142 ,
146 ,
149 ,
153 ,
156 ,
160 ,
163 ,
167 ,
170 ,
173 ,
177 ,
180 ,
183 ,
186 ,
189 ,
192 ,
195 ,
198 ,
200 ,
203 ,
206 ,
208 ,
211 ,
213 ,
216 ,
218 ,
220 ,
223 ,
225 ,
227 ,
229 ,
231 ,
232 ,
234 ,
236 ,
238 ,
239 ,
241 ,
242 ,
243 ,
245 ,
246 ,
247 ,
248 ,
249 ,
250 ,
251 ,
251 ,
252 ,
253 ,
253 ,
254 ,
254 ,
254 ,
254 ,
254 ,
255 ,
254 ,
254 ,
254 ,
254 ,
254 ,
253 ,
253 ,
252 ,
251 ,
251 ,
250 ,
249 ,
248 ,
247 ,
246 ,
245 ,
243 ,
242 ,
241 ,
239 ,
238 ,
236 ,
234 ,
232 ,
231 ,
229 ,
227 ,
225 ,
223 ,
220 ,
218 ,
216 ,
213 ,
211 ,
208 ,
206 ,
203 ,
200 ,
198 ,
195 ,
192 ,
189 ,
186 ,
183 ,
180 ,
177 ,
173 ,
170 ,
167 ,
163 ,
160 ,
156 ,
153 ,
149 ,
146 ,
142 ,
138 ,
135 ,
131 ,
127 ,
123 ,
119 ,
115 ,
111 ,
107 ,
103 ,
99  ,
95  ,
91  ,
87  ,
83  ,
78  ,
74  ,
70  ,
65  ,
61  ,
57  ,
53  ,
48  ,
44  ,
39  ,
35  ,
31  ,
26  ,
22  ,
17  ,
13  ,
8 ,
4 ,
0 ,
5 ,
9 ,
14  ,
18  ,
23  ,
27  ,
32  ,
36  ,
40  ,
45  ,
49  ,
54  ,
58  ,
62  ,
66  ,
71  ,
75  ,
79  ,
84  ,
88  ,
92  ,
96  ,
100 ,
104 ,
108 ,
112 ,
116 ,
120 ,
124 ,
128 ,
132 ,
136 ,
139 ,
143 ,
147 ,
150 ,
154 ,
157 ,
161 ,
164 ,
168 ,
171 ,
174 ,
178 ,
181 ,
184 ,
187 ,
190 ,
193 ,
196 ,
199 ,
201 ,
204 ,
207 ,
209 ,
212 ,
214 ,
217 ,
219 ,
221 ,
224 ,
226 ,
228 ,
230 ,
232 ,
233 ,
235 ,
237 ,
239 ,
240 ,
242 ,
243 ,
244 ,
246 ,
247 ,
248 ,
249 ,
250 ,
251 ,
252 ,
252 ,
253 ,
254 ,
254 ,
255 ,
255 ,
255 ,
255 ,
255 ,
255 ,
255 ,
255 ,
255 ,
255 ,
255 ,
254 ,
254 ,
253 ,
252 ,
252 ,
251 ,
250 ,
249 ,
248 ,
247 ,
246 ,
244 ,
243 ,
242 ,
240 ,
239 ,
237 ,
235 ,
233 ,
232 ,
230 ,
228 ,
226 ,
224 ,
221 ,
219 ,
217 ,
214 ,
212 ,
209 ,
207 ,
204 ,
201 ,
199 ,
196 ,
193 ,
190 ,
187 ,
184 ,
181 ,
178 ,
174 ,
171 ,
168 ,
164 ,
161 ,
157 ,
154 ,
150 ,
147 ,
143 ,
139 ,
136 ,
132 ,
128 ,
124 ,
120 ,
116 ,
112 ,
108 ,
104 ,
100 ,
96  ,
92  ,
88  ,
84  ,
79  ,
75  ,
71  ,
66  ,
62  ,
58  ,
54  ,
49  ,
45  ,
40  ,
36  ,
32  ,
27  ,
23  ,
18  ,
14  ,
9 ,
5 ,
1
};

#else
///
/// Half Bridge Sine Wave Table
///
int32_t sinetable[]= 
{
128 ,
129 ,
131 ,
133 ,
135 ,
137 ,
139 ,
141 ,
143 ,
145 ,
147 ,
149 ,
151 ,
153 ,
155 ,
157 ,
159 ,
161 ,
163 ,
165 ,
167 ,
168 ,
170 ,
172 ,
174 ,
176 ,
177 ,
179 ,
181 ,
183 ,
185 ,
186 ,
188 ,
190 ,
191 ,
193 ,
195 ,
196 ,
198 ,
200 ,
201 ,
203 ,
204 ,
205 ,
207 ,
209 ,
210 ,
211 ,
213 ,
214 ,
215 ,
217 ,
218 ,
219 ,
220 ,
221 ,
222 ,
223 ,
225 ,
226 ,
227 ,
228 ,
229 ,
230 ,
231 ,
231 ,
232 ,
233 ,
234 ,
235 ,
235 ,
236 ,
236 ,
237 ,
238 ,
238 ,
239 ,
239 ,
240 ,
240 ,
240 ,
240 ,
241 ,
241 ,
241 ,
242 ,
242 ,
242 ,
242 ,
242 ,
242 ,
242 ,
242 ,
242 ,
242 ,
242 ,
241 ,
241 ,
241 ,
240 ,
240 ,
240 ,
240 ,
239 ,
239 ,
238 ,
238 ,
237 ,
236 ,
236 ,
235 ,
235 ,
234 ,
233 ,
232 ,
231 ,
231 ,
230 ,
229 ,
228 ,
227 ,
226 ,
225 ,
223 ,
222 ,
221 ,
220 ,
219 ,
218 ,
217 ,
215 ,
214 ,
213 ,
211 ,
210 ,
209 ,
207 ,
205 ,
204 ,
203 ,
201 ,
200 ,
198 ,
196 ,
195 ,
193 ,
191 ,
190 ,
188 ,
186 ,
185 ,
183 ,
181 ,
179 ,
177 ,
176 ,
174 ,
172 ,
170 ,
168 ,
167 ,
165 ,
163 ,
161 ,
159 ,
157 ,
155 ,
153 ,
151 ,
149 ,
147 ,
145 ,
143 ,
141 ,
139 ,
137 ,
135 ,
133 ,
131 ,
129 ,
128 ,
125 ,
123 ,
121 ,
119 ,
117 ,
115 ,
113 ,
111 ,
110 ,
107 ,
105 ,
103 ,
101 ,
100 ,
98  ,
96  ,
94  ,
92  ,
90  ,
88  ,
86  ,
84  ,
83  ,
81  ,
79  ,
77  ,
75  ,
74  ,
72  ,
70  ,
68  ,
66  ,
65  ,
63  ,
61  ,
60  ,
58  ,
57  ,
55  ,
54  ,
52  ,
51  ,
49  ,
47  ,
46  ,
45  ,
43  ,
42  ,
41  ,
39  ,
38  ,
37  ,
36  ,
34  ,
33  ,
32  ,
31  ,
30  ,
29  ,
28  ,
27  ,
26  ,
25  ,
24  ,
23  ,
23  ,
22  ,
21  ,
20  ,
20  ,
19  ,
18  ,
18  ,
17  ,
16  ,
16  ,
15  ,
15  ,
15  ,
14  ,
14  ,
14  ,
13  ,
13  ,
13  ,
13  ,
13  ,
13  ,
13  ,
13  ,
13  ,
13  ,
13  ,
13  ,
13  ,
13  ,
13  ,
14  ,
14  ,
14  ,
15  ,
15  ,
15  ,
16  ,
16  ,
17  ,
18  ,
18  ,
19  ,
20  ,
20  ,
21  ,
22  ,
23  ,
23  ,
24  ,
25  ,
26  ,
27  ,
28  ,
29  ,
30  ,
31  ,
32  ,
33  ,
34  ,
36  ,
37  ,
38  ,
39  ,
41  ,
42  ,
43  ,
45  ,
46  ,
47  ,
49  ,
51  ,
52  ,
54  ,
55  ,
57  ,
58  ,
60  ,
61  ,
63  ,
65  ,
66  ,
68  ,
70  ,
72  ,
74  ,
75  ,
77  ,
79  ,
81  ,
83  ,
84  ,
86  ,
88  ,
90  ,
92  ,
94  ,
96  ,
98  ,
100 ,
101 ,
103 ,
105 ,
107 ,
110 ,
111 ,
113 ,
115 ,
117 ,
119 ,
121 ,
123 ,
125 ,
127
};
#endif

uint8_t readPot =0;
int32_t ctr=0;
int32_t ctr2=180;
int32_t val2 = 0;
/**
 * @brief IRAM_ATTR
 * An interrupt service routine that runs whenever a rising edge is detected on
 * the feedback I/O pin. We generate a PWM pulse at 18KHz for 50Hz, or 21.6Khz 
 * for a 60Hz pulse. the PWM output pin is fed back into the chip as an 
 * interrupt and we retrigger from it.
 * Every time an interrupt is received we load in the next PWM value from the 
 * sine table. this allows each degree of the 50 or 60Hz waveform to have a 
 * separate pulse associated with it  
 */
void IRAM_ATTR isr() 
{

  int32_t val;

  ///
  /// 1. Compute next PWM register value
  ///
    if ((ctr+phaseOffset) < 360){
    val = sinetable[(ctr + phaseOffset)] ; //scale the sine tablkee and apply offset
  }
  else{
    val = sinetable[(ctr + phaseOffset - 360)] ; //scale the sine tablkee and apply offset
  }
  
  
#if FULL_BRIDGE  
  ///
  /// Full Bridge 
  /// 2. select which transitor bank to use
  ///
  if(ctr>TRANSISTOR_SWITCH_STEP)
  {
    ledcWrite(DRIVE1_PWM,   val);                           //signal to mosfet gate
    ledcWrite(DRIVE2_PWM,   0);                             //signal to mosfet gate
  }
  else
  {
    ledcWrite(DRIVE1_PWM,   0);                             //signal to mosfet gate
    ledcWrite(DRIVE2_PWM,   val);                           //signal to mosfet gate
  }
#else
  ///
  /// Half Bridge
  ///
  ledcWrite(DRIVE1_PWM,   val);                             //signal to mosfet gate
  ledcWrite(DRIVE2_PWM,   0);                               //signal to mosfet gate
#endif

  ///
  /// 3. increment counter
  ///
  ctr += 1;
  if(ctr==MAX_STEPS)
  {
    ctr     = 0;
    readPot = 1;  //allow amplitude to change
  }

  
  ///
  /// 4. self triggering interrupt (mandatory, it just has to have some non-zero value)
  ///
  ledcWrite(SELF_TRIGGERING_IRQ, 1); // set the brightness of the LED

}






int32_t t = 0;
int32_t oldPotValue = -1;

const int numReadings = 10;

int readings[numReadings];      // the readings from the analog input
int readIndex = 0;              // the index of the current reading
int total = 0;                  // the running total
int average = 0;                // the average
int pV = 0;


/**
 * @brief setup
 * the setup routine runs once when you press reset:
 */
void setup() {
 

  // Initialize channels
  // channels 0-15, resolution 1-16 bits, freq limits depend on resolution
  // ledcSetup(uint8_t channel, uint32_t freq, uint8_t resolution_bits);
  ledcSetup(SELF_TRIGGERING_IRQ,  MODULATING_FREQ, 8);                 // 18 kHz PWM, 8-bit resolution
  ledcSetup(DRIVE1_PWM,           MODULATING_FREQ, 8);                 // 18 kHz PWM, 8-bit resolution
  ledcSetup(DRIVE2_PWM,           MODULATING_FREQ, 8);                 // 18 kHz PWM, 8-bit resolution


  ledcAttachPin(led_gpio,         SELF_TRIGGERING_IRQ); // assign a led pins to a channel
  ledcAttachPin(PUSHPULL_DRIVE_1, DRIVE1_PWM);
  ledcAttachPin(PUSHPULL_DRIVE_2, DRIVE2_PWM);

  
  pinMode(PWM_FEEDBACK_PIN, INPUT_PULLDOWN);
  attachInterrupt(PWM_FEEDBACK_PIN, isr, RISING);           //this is wired to the PWM output 
  
  ledcWrite(SELF_TRIGGERING_IRQ,  pwm);                                        //signal to mosfet gate
  ledcWrite(DRIVE1_PWM,           127);                           //signal to mosfet gate
  ledcWrite(DRIVE2_PWM,           127);                           //signal to mosfet gate
  Serial.begin(115200);

  pinMode(CYCLE_START_PIN, INPUT_PULLUP);

  for (int thisReading = 0; thisReading < numReadings; thisReading++) {
    readings[thisReading] = 0;
  }

  //Only start the loop once the signal is already high. Hopefully this will work, one downside is that is will work for only a moment.
  //TODO add multithreading and interrupt

int tempValue = 1;

while (tempValue){
  if(analogRead(CYCLE_START_PIN) > 3680){
    tempValue = 0;
  }
}

}



const int CAP_ADC = 255;///176;

/**
 * @brief loop
 * Shall read ADC value and allow user to modify PWM amplitude by means of
 * potentiometer. We scale the SPWM in fact, allowing a synthesised lower voltage
 */
void loop() {
  if(analogRead(CYCLE_START_PIN) > 3680) {
    cycleStartTriggerd = 1;
  }

  if(cycleStartTriggerd){
  if(readPot)
  {
   
    t+=1;
    if(20==t)
    {
       readPot = 0;
    int p = analogRead(potPin);  //https://randomnerdtutorials.com/esp32-adc-analog-read-arduino-ide/
      pV = p/16;
// subtract the last reading:
  total = total - readings[readIndex];
  // read from the sensor:
  readings[readIndex] = pV;
  // add the reading to the total:
  total = total + readings[readIndex];
  // advance to the next position in the array:
  readIndex = readIndex + 1;

  // if we're at the end of the array...
  if (readIndex >= numReadings) {
    // ...wrap around to the beginning:
    readIndex = 0;
  }

  // calculate the average:
  average = total / numReadings;


      potValue = average;
      potValue = 255;
      if(potValue>CAP_ADC)
      {
        potValue=CAP_ADC;
      }
      if(oldPotValue!=potValue)
      {
        Serial.printf("ADCM %u\n", potValue);
        oldPotValue=potValue;
      }
     t=0;
   
    if(potValue<1)
    {
      potValue = 0;
    }
    if(potValue>255)
    {
      potValue = 255;
    }
    }
   
  }
  cycleStartTriggerd = 0;
  }
}
