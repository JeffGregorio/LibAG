# LibAG

C++ library for audio synthesis on AVR-based Arduino microcontrollers, developed and tested on the ATmega328P using Arduino Uno and Nano, and the ATmega2560 using the Arduino Mega. 

This library should be an accessible entry point for Arduino hobbyists and engineering students wanting to explore real-time audio synthesis at a lower level of abstraction than similar libraries. It's meant to be more efficient, readable, and workable than general and feature-laden.

## 0 Installation

To use with the Arduino IDE, download this repository as a ZIP and place in your Arduino/libraries directory, likely ~/Documents/Arduino/libraries/ on Mac OS and \My Documents\Arduino\libraries\ on Windows. 

## 1 Overview

LibAG offers a small set of peripheral controllers and example sketches that demonstrate configuration of timers, ADC, interrupt service routines, and SPI for use with external DACs. 

It also includes a small set of digital signal processing (DSP) objects and utility functions that demonstrate techniques for working within the limitations of 8-bit AVR processors. This includes fixed point math and table-based approaches to sinusoidal synthesis, exponential parameter control, and filter coefficients.

Memory use is also minimized by storing pre-computed, normalized tables in flash memory and rescaling outputs. Several exponential tables are provided alongside a python script for generating others. 

## 2 Introduction

In discrete time, it is impossible to generate or reproduce frequencies more half the sample rate (the Nyquist rate), and that's without accounting for the portion of the passband that'll be attenuated by the reconstruction filter. This means the higher the sample rate, the better.

Although sample rates like 44.1 or 48kHz permit only the most minimal computation on these AVRs, lower rates are emanently usable in modular synth applications like low frequency oscillators, envelope generators and followers, MIDI to CV converters, sequencers, etc., making these processors more than appropriate. 

To get the most out of the processor, we'll replace otherwise admirable Arduino staples like `delay()`, `millis()`, `analogWrite()`, `analogRead()`. Their generality is no free lunch, and the peripherals they use (timers and converters) can be much better configured for our task. 

What we'll need is a fuction that runs at a regular sample rate, and a way of converting from digital to analog and vice versa that takes up as little of the sample period as possible--leaving the rest for DSP code. 

A timer in one of its Pulse Width Modulation (PWM) modes can be used as a DAC. Ideally we want the PWM rate well above the sample rate, so we use the aptly-named Fast PWM mode. We can configure automatic ADC conversions at a regular rate, either using the ADC on its own (free running mode), with an external signal, or using a timer in Clear Timer on Compare Match (CTC) mode. 

Rather than `loop()`, we'll do our processing in interrupt service routines (ISRs) either called by a timer or by the ADC after it completes a conversion. 

## 3 Peripheral Drivers: Basic Usage 
### 3.1 Timers in PWM and CTC Modes

In this example, Timer 0 gives us a 16kHz sample rate to render a low-frequency sawtooth wave. 

```C
#include "Timer.h"

Timer0 timer0;		// Sample rate trigger
Timer2 timer2;		// PWM output

volatile uint16_t phase = 0;	// 16-bit phase in [0, 0xFFFF]

void setup() {
	timer2.set_prescaler(1);
	timer2.init_pwm();			// 8-bit PWM rate 16e6/1/256 = 62.5kHz
	timer0.set_prescaler(8);	
	timer0.init_ctc(124);		// Call ISR at 16e6/8/125 = 16kHz
}

...

ISR(TIMER0_COMPA_vect) {		
	phase++;						// Sawtooth, ~0.244 Hz
	timer2.pwm_write_a(phase >> 8);	// Write 8-bit signal to pin OCR2A
}
```

Notice that sample rates and PWM rates are determined in part by prescalers, which are values used to divide the system clock (16MHz on the Arduino Uno, Nano, Mega) that drives the peripheral. Timer 0, in the above example ticks forward (adding one to its 8-bit count value) at one eighth the rate of a timer 2. 

Prescaler options for Timer 0 and Timer 1 include {1, 8, 64, 256, 1024}. Timer 2 has additional prescaler options including {1, 8, 32 64, 128, 256, 1024}. 

We're using 8-bit PWM so timer 2's period, ergo the PWM rate, is the time it takes the timer to count from 0 to 255 and overflow back to 0. To output a PWM signal to pin OCR2A, we supply an 8-bit value to the timer's `pwm_write_a()` method. The timer compares this value to its count and toggles the OCR2A pin when they match. 

Timer 0 controls sample timing using CTC mode and a larger prescaler. Rather than toggling a pin on compare match like PWM mode, CTC mode calls the (in this case) A channel's interrupt service routine *and resets the timer count*, meaning the rate is a function of the compare value rather than the timer's full resolution. 

Side note: we could have divided the 16-bit phase by 256 to obtain an 8-bit output value, but division is generally slow and to be avoided if possible. Divison by a power of two is much, much faster via bit shifting. 

### 3.2 ADC Free Running Mode

In this example, we use the ADC's free running mode to automatically scan two channels at about 19kHz, multiply their values, and output the result. Notice the effective input sample rate for each channel is half the output sample rate, as the ADC only converts from one channel at a time. 

```C
#include "ADCAuto.h"
#include "Timer.h"

Timer2 timer2;				// PWM output
ADCFreeRunning adc(2);		// Analog inputs (A0, A1) and sample rate trigger
volatile uint16_t a0, a1;	// ADC samples

void setup() {
	timer2.set_prescaler(1);	// 8-bit PWM rate 16e6/1/256 = 62.5kHz
	timer2.init_pwm();
	adc.set_prescaler(64);		// Convert and retrigger at 16e6/64/13 = 19.2kHz
	adc.init();
}

...
	
ISR(ADC_vect) {					// Called after ADC conversion completes
	adc.update();				// Scans the current channel and stores result
	a0 = adc.results[0] >> 2;	// Convert ch 0 result to 8-bit
	a1 = adc.results[1] >> 2;	// Convert ch 1 result to 8-bit
	a0 = (a0 * a1) >> 8;		// 8-bit UQ multiply (see sec 4.1)
	timer2.pwm_write_a(sample);	// Write to pin OCR2A
}
```

Notice the ADC has a prescaler as well, which determines the free running rate alongside the fixed 13 clock cycles it takes the ADC to complete a conversion. Per the processor datasheet's recommendation, the ADC clock (i.e. the system clock divided by the ADC's prescaler) should be under 200kHz to guarantee full 10-bit resolution. 

With prescaler options {2, 4, 8, 16, 32, 64, 128}, this means only 128 provides full resolution. As in this example, ADC clocks up to 1MHz can be used if we don't mind trading *effective* resolution for speed (note we still retrieve results at 10-bit values). 

Notice our *only* free parameter for determining ADC free running rate is the ADC prescaler. With a 16MHz system clock, the usable options 128, 64, and 32 give approximate sample rates 9.6kHz, 19.2kHz, and 38.5kHz.

### 3.3 ADC Triggered by Timer 0

A more flexible solution for triggering conversions is to use Timer 0's CTC mode. Here, we modify the previous example for use with sample rates *up to* the ADC's free running rate. Note Timer 0's ISR can be empty, but must be included. 

```C
#include "ADCAuto.h"
#include "Timer.h"

Timer0 timer0;				// ADC trigger source 
Timer2 timer2;				// PWM output
ADCTimer0 adc(2);			// Analog inputs (A0, A1)
volatile uint16_t a0, a1;	// ADC samples

void setup() {
	timer2.set_prescaler(1);	// 8-bit PWM rate 16e6/1/256 = 62.5kHz
	timer2.init_pwm();
	adc.set_prescaler(64);		// Maximum conversion rate 16e6/64/13 = 19.2kHz
	adc.init();
	timer0.set_prescaler(8);	
	timer0.init_ctc(124);		// Call ISR at 16e6/8/125 = 16kHz
}

ISR(TIMER0_COMPA_vect) {	// Called on channel A compare match
	;	// Implicitly triggers ADC conversion; nothing to do
}

ISR(ADC_vect) {		// Called after ADC conversion completes
	adc.update();	// Scans the current channel and stores result
	a0 = adc.results[0] >> 2;	// Convert ch 0 result to 8-bit
	a1 = adc.results[1] >> 2;	// Convert ch 1 result to 8-bit
	a0 = (a0 * a1) >> 8;		// 8-bit multiply
	timer2.pwm_write_a(sample);	// Write to pin OCR2A
}
```

### 3.4 ADC Triggered by External Signal

This simple sample and hold CV processor triggers ADC conversions on the rising edge of an external digital signal on the `INT0` pin. Like the previous example, the trigger source's ISR can be empty, but must be included.

```C
#include "ADCAuto.h"
#include "Timer.h"

ADCInt0 adc(1);		// Analog input A0 and sample rate trigger
Timer2 timer2;		// PWM output

volatile uint16_t sample;		// CV sample

void setup() {
	timer2.set_prescaler(1);	// 8-bit PWM rate 16e6/1/256 = 62.5kHz
	timer2.init_pwm();
	adc.set_prescaler(64);		// Maximum conversion rate 16e6/64/13 = 19.2kHz
	adc.init();
}

ISR(INT0_vect) {	// Clalled on INT0 rising edge
	;	// Implicitly triggers ADC conversion; nothing to do
}

ISR(ADC_vect) {		// Called after ADC conversion completes
	adc.update();	// Scan the current channel and store result
	sample = adc.results[0] >> 2;	// Scale 10- to 8-bit
	timer2.pwm_write_a(sample);		// Write to pin OCR2A
}
```

Here, we should not expect our external signal to successfully trigger conversions faster than the ADC's free running rate of 19.2kHz. 

### 3.5 Up to 16-bit PWM with Timer 1

Timer 1 has the same prescaler options as Timer 0 {1, 8, 64, 256, 1024}, but has a 16-bit count and compare value. The libAG's `Timer1` class also uses a PWM mode that allows a variable resolution. 

Here, we modify the previous example for 10-bit output as well as input. 

```C
#include "ADCAuto.h"
#include "Timer.h"

ADCInt0 adc(1);		// Analog input A0 and sample rate trigger
Timer1 timer1;		// PWM output

void setup() {
	timer1.set_prescaler(1);	
	timer1.init_pwm(1023);		// 10-bit PWM rate 16e6/1/1024 = 15.625kHz
	adc.set_prescaler(64);		// Maximum conversion rate 16e6/64/13 = 19.2kHz
	adc.init();
}

ISR(INT0_vect) {	// Clalled on INT0 rising edge
	;	// Implicitly triggers ADC conversion; nothing to do
}

ISR(ADC_vect) {		// Called after ADC conversion completes
	adc.update();	// Scan the current channel and store result
	timer1.pwm_write_a(adc.results[0]);	// Write to pin OCR1A
}
```

Note that raising the timer's resolution lowers the PWM rate. Although we can still expect the ADC to convert as fast as 19.2kHz, the lower PWM rate constrains the output sample rate, meaning frequencies above half the PWM rate will alias. 

### 3.6 External Digital to Analog Converter (DAC)

The rate/resolution trade-off can be circumvented (at least at audio rates) by using an external DAC like the MCP4921/4922, which are one- and two-channel 12-bit DACs that use a simple Serial Peripheral Interface (SPI) protocol. When configured, the DAC takes a 16-bit control word consisting of bits (MSB to LSB) for the A/B channel selection, buffer enable, high/low gain setting, shutdown, followed by the 12 bits comprising an audio sample.

The AVR processors have an SPI peripheral which, at its maximum SPI clock rate of 4MHz can shift the control word's sixteen bits out at rates up to 250kHz, ensuring 12-bit resolution at any sample rate we could achieve. 

The peripheral generates the SPI clock and data out signals on the SCK and MOSI pins, respectively. Since we're not receiving any data from the DAC, we don't need to use the MISO pin. LibAG's `SPIMaster` class configures the SPI and shifts out 8 or 16-bit control words. The user must configure a CS (chip select) pin, clear it before transmission, and set it after. 

Revisiting the first example, we could now synthesize a sawtooth at 12-bit amplitude resolution without lowering the sample rate. 

```C
#include "Timer.h"
#include "SPI.h"

Timer0 timer0;					// Sample rate trigger
SPIMaster spi;					// SPI to DAC

volatile uint16_t phase = 0;	// 16-bit phase in [0, 0xFFFF]

void setup() {
	spi.init();					// Start SPI with 4MHz clock
	DDRB |= (1 << PB0);			// Configure pin PB0 as output (use as CS pin)
	timer0.set_prescaler(8);	
	timer0.init_ctc(124);		// Call ISR at 16e6/8/125 = 16kHz
}

...

// Note the use of MCP4922's control word:
// A'/B BUF GA' SHDN' D11 D10 D9 D8 D7 D6 D5 D4 D3 D2 D1 D0
ISR(TIMER0_COMPA_vect) {		
	phase++;					// Sawtooth, ~0.244 Hz
    PORTB &= ~(1 << PB0);		// Clear PB0 (select DAC chip by outputing LOW)
	spi.write_u16(0b0101000000000000 | (phase >> 4));		
	PORTB |= (1 << PB0);		// Set PB0 (deselect DAC chip by outputing HIGH)
}
```

Note we replace even Arduino's `digitalWrite()` with faster, low-level alternatives to clearing and setting pins on (in this case) `PORTB`'s pin 0 directly. That's after configuring the pin as an output using the port's data direction register `DDRB`. 

## 4 Fixed Point Format

Due to the AVR processors' lack of dedicated hardware for floating point math, we use fixed point math whenever efficiency is critical, as in our sampling ISRs or any processing or parameter setting operation that's called from an ISR. 

Fixed point uses integer types, only we *treat* them as representing non-integer values over fixed intervals, and the processor's integer arithmetic logic unit (ALU) is none the wiser. We just have to take a few extra precautions concerning precision and integer overflow.

### 4.1 Unsigned (UQ8, UQ16, UQ32)

An unsigned 8-bit integer normally represents [0, 255] as

MSbit | | | | | | | LSbit
--|--|--|--|--|--|--|--
2<sup>7</sup>|2<sup>6</sup>|2<sup>5</sup>|2<sup>4</sup>|2<sup>3</sup>|2<sup>2</sup>|2<sup>1</sup>|2<sup>0</sup>

But we can think of it as [0, 1), i.e. 

MSbit | | | | | | | LSbit
--|--|--|--|--|--|--|--
2<sup>-1</sup>|2<sup>-2</sup>|2<sup>-3</sup>|2<sup>-4</sup>|2<sup>-5</sup>|2<sup>-6</sup>|2<sup>-7</sup>|2<sup>-8</sup>

and nothing changes from the processor's perspective. This is known as UQ8 format. We can also think of UQ8 as its integer value multiplied by an implicit scaling factor of 2<sup>-8</sup>, making the maximum value 255/256 = 0.9961.

We can add or subtract UQ8 values normally as long as the result isn't greater than 255 or less than 0, in which case we should use the saturating arithmetic functions defined in LibAG's `FixedPoint.h`. E.g. if a UQ8 addition might excede 255, use `addsat8()`, and if a UQ8 subtraction might be less than zero, use `subsat8()`. 

Multiplication of UQ8 values must be handled by casting one operand to a 16-bit type capable of holding the product of unsigned 8-bit integers, then multiplying by the UQ8 scaling factor 2<sup>-8</sup> as follows

```C
uint8_t a = /* value in [0, 255] */;
uint8_t b = /* value in [0, 255] */;
uint8_t c = (uint16_t)a * b >> 8;
```

Note that we could have divided by 256 to scale the product, but division via right bit shifting is much faster. The LibAG function `qmul8()`, defined in `FixedPoint.h` does this operation with an extra addition for rounding. 

### 4.2 Signed (Q8, Q16, Q32)

Likewise a signed 8-bit integer normally represents [-128, 127] in two's complement as

MSbit | | | | | | | LSbit
--|--|--|--|--|--|--|--
-2<sup>7</sup>|2<sup>6</sup>|2<sup>5</sup>|2<sup>4</sup>|2<sup>3</sup>|2<sup>2</sup>|2<sup>1</sup>|2<sup>0</sup>

But we can think of it as [-1, 1), i.e.

MSbit | | | | | | | LSbit
--|--|--|--|--|--|--|--
-2<sup>0</sup>|2<sup>-1</sup>|2<sup>-2</sup>|2<sup>-3</sup>|2<sup>-4</sup>|2<sup>-5</sup>|2<sup>-6</sup>|2<sup>-7</sup>

And again, nothing changes from the processor's perspective. This is known as Q8 format. We can also think of Q8 as its integer value multiplied by an implicit scaling factor of 2<sup>-7</sup>, making the maximum value 127/128 = 0.9922.

See `FixedPoint.h` for additional saturating arithmetic and multiply functions for signed types. 

### 4.3 Other formats

Parts of LibAG treat `uint8_t`, `uint16_t`, and `uint32_t`, respectively as UQ8, UQ16, and UQ32 types, and likewise `int8_t`, `int16_t`, and `int32_t` as Q8, Q16, and Q32. Each of these types uses all of their bits for fractional precision. 

In general, fixed point values can use M bits for their integer part, and N bits for their fractional part. As such, UQM.N and QM.N formats are not implemented by LibAG.

## 5 DSP Classes

LibAG includes classes for basic oscillators like sawtooths and wavetables (`Phasor16` and `Wavetable16` defined in `Oscillator.h`), linear envelope generators (`ASR16` in `Envelope.h`), and one pole IIR filters (`OnePole16` and `OnePole16_LF` in `IIR.h`). 

These classes use normalized frequencies specified as integer values rather than Hz. The following sections are essential to using them effectively. 

### 5.1 Frequency Normalization

Recall that the sample rate f<sub>s</sub> limits the frequencies we can capture and synthesize in discrete time. I.e. we are limited to frequencies below f<sub>s</sub>/2. Note in some cases we can use the negative frequency band down to -f<sub>s</sub>/2.

If you're coming from an engineering background, you might be accustomed to seeing this interval normalized to (-&#960;, &#960;), a natural choice given the domain of periodic functions like sin, cos, and the complex exponential. The choice is arbitrary, however. For example, if we had hardware floating point we might use (-0.5, 0.5) as a convenience and scale these normalized frequencies to the former range by multiplying with 2&#960; if needed. 

Perhaps it shouldn't come as a surprise that when all we have is integer arithmetic, we can normalize to an integer range, or equivalently, a fixed point range. While UQ8 or Q8 fixed point give poor frequency resolution, UQ16 and Q16 are satisfactory for most purposes, with an effective resolution (in Hz) of f<sub>s</sub>/2<sup>16</sup>. 

Frequencies in Hz are normalized to Q16 as such

```C
float f_s = /* system sample rate in Hz */
int16_t freq = 20.0 / f_s * 0xFFFF;
```

And here we revisit the sawtooth example with a frequency specified in Hz. 

```C
#include "Timer.h"

const float f_s = 16e3;		// Sample rate

Timer0 timer0;		// Sample rate trigger
Timer2 timer2;		// PWM output

volatile uint16_t phase = 0;				  // 16-bit phase in [0, 0xFFFF]
volatile int16_t freq = 20.0f / f_s * 0xFFFF; // 16-bit freq in [-0x8000, 0x7FFF]

void setup() {
	timer2.set_prescaler(1);
	timer2.init_pwm();			// 8-bit PWM rate 16e6/1/256 = 62.5kHz
	timer0.set_prescaler(8);	
	timer0.init_ctc(124);		// Call ISR at 16e6/8/125 = 16kHz
}

...

ISR(TIMER0_COMPA_vect) {		
	phase += freq;					// Sawtooth, ~20 Hz
	timer2.pwm_write_a(phase >> 8);	// Write 8-bit signal to pin OCR2A
}
```

The classes defined in `Oscillator.h` use Q16 frquency, as do the attack and release rates of the envelope generator defined in `Envelope.h`. In many cases, a Q16 frequency can be used as a filter coefficient for the filters in `IIR.h`. See Section 6.3 for a full explanation. 

Since floating point operations like this normalization step will be slow, it is advantageous to pre-compute values where possible. 

### 5.2 Frequency parameter curves

Prior to normalizing the frequency, we typically want to specify an exponential mapping from an integer value, say a MIDI note number or an ADC conversion, to frequency. For example, MIDI note to frequency conversion accomplishes doubling and halving of a (typically) 440Hz reference frequency for every increase or decrease of 12 semitones, centered so that note 69 corresponds to the reference. 

```C
uint8_t note = /* MIDI note number in [0, 127] */
float freq = 440.0f * powf(2.0f, (note-69)/12.0f);
```

Similarly, we can compute the base of an exponential sweep for specified nonzero endpoints to map a 10-bit ADC conversion result.

```C
float base = powf((2000.0f/20.0f), 1.0f/1023);
float freq = 20.0f * powf(base, adc.results[0]);
```

Both involve expensive floating point operations and function calls. More efficient fixed point approximations are possible, but with integer control values, it can be useful to pre-compute an entire normalized frequency range in a table, say, of length 128 (for MIDI note lookup), or 1024 (or 10-bit ADC conversion lookup). LibAG takes this approach to exponetial frequency control. See Section 6.2 for a full explanation. 

## 6 Table Generation

Though tables can be computed at startup and stored in SRAM, space is very limited (2kB on the Atmega328 and 8kB on the Atmega2560). Rather, pre-computed tables can be stored in flash memory (up to 32kB on the Atmega328 and 256kB on the Atmega2560) and read using macros defined in the standard avr-gcc library `<avr/pgmspace.h>`. LibAG classes `Wavetable16` and `PgmTable16` take pointers to these table addresses and handle lookup and output scaling.

The library provides a python script `tablegen.py` which can be used to generate C header files containing tables declared in flash memory. The script has no dependencies for generating tables and writing files, but if the user has [Matplotlib](https://matplotlib.org/) installed, the script will generate plots of the resulting table functions.

### 6.1 Sine
Sine wave tables can be generated at the command line using

```
> python tablegen.py sine
```

Which generates an unsigned 16-bit integer wave table of length 1024. Unsigned 16-bit types are required for use with the library's wavetable oscillator class `Wavetable16`.

Other data types and lengths can be generated using optional arguments as follows

```
> python tablegen.py --dtype <type> --length <length> sine
```

Where `type` is one of `{u8, s8, u16, s16, u32, s32}` indicating signed/unsigned integer types of the specified size. 

The resulting tables are written to files with unique names given their parameters. For example, an unsigned 16-bit sine wave table of length 1024 will be written to `tables/sine_u16x1024.h`.

The user must include the table and pass its address to the wavetable oscillator alongside an integer number used to scale the 16-bit phase oscillator to the table length via right shift before lookup. For example, the length-1024 sine wave table can be used with Wavetable16 as follows

```C
#include "tables/sine_u16x1024.h"
...

// Unsigned 16-bit integer sine table of length 1024
Wavetable16 lfo(sine_u16x1024, 6);	
// 16-bit phase is right-shifted 6 bits to use as a 10-bit lookup index

...

ISR(/* interrupt vector */) {
	lfo.freq = /* frequency in [-0x8000, 7FFFF] */;			
	sine_val = lfo.render();	// Render a sine wave sample
}
```

Note that due to the division via right shift, table lengths must be a power of two for use with Wavetable16.

### 6.2 Exponential

Exponential tables of length N and index n in [0, N-1] can be generated over a nonzero range (e<sub>0</sub>, e<sub>1</sub>) using

<img src="https://render.githubusercontent.com/render/math?math=y[n] = e_0 \cdot c^{n}"> 

where 

<img src="https://render.githubusercontent.com/render/math?math=c = \sqrt[{N-1}]{\frac{e_1}{e_0}}">

Rather than generate separate tables over specific Q16 frequency ranges, it is useful to normalize the table such that the maximum value is `0xFFFF`, and dynamically re-scale the output using a UQ16 multiply. This places the maximum value at the UQ16 scaling factor, and the minimum value at scale/ratio, where the ratio is e<sub>1</sub>/e<sub>0</sub>.

![ExpTable](/images/exp.png)

Thus, `tablegen` generates exponential tables with specified ratios 

```
> python tablegen.py exp <ratio>
```

Other data types and lengths can be generated using optional arguments as follows

```
> python tablegen.py --dtype <type> --length <length> exp <ratio>
```

Where `type` is one of `{u8, u16, u32}` indicating the size of an unsigned integer type.

The table can be used with a `PgmTable16` instance's `lookup_scale()` method. The scaling factor can be computed at startup and/or changed dynamically. 

For example, a 10-bit ADC reading can be used to control the previous example's `Wavetable16` oscillator's frequency with an exponential sweep over [0.2, 200]Hz using

```C
#include "tables/sine_u16x1024.h"
#include "tables/exp1000_u16x1024.h"

const float fs = 16e3;

...

Wavetable16 lfo(sine_u16x1024, 6);	

uint16_t scale = 200.0/fs * 0xFFFF;
PgmTable16 freq_table(exp1000_u16x1024, scale);

...

ISR(ADC_vect) {
	adc.update();
	lfo.freq = freq_table.lookup_scale(adc.results[0]);
	sine_val = lfo.render();	// Render a sine sample
}
```

### 6.3 Filter Coefficients 

The `OnePole16` and `OnePole16_LF` (low-frequency) objects declared in `IIR.h` implement a multimode filter with variable cutoff frequency. The filter implements the difference equation of an exponential moving average filter

<img src="https://render.githubusercontent.com/render/math?math=y[n] = y[n-1] %2B \alpha(x[n] - y[n-1])">

The `OnePole` classes provide no setter methods for the coefficient, as it is meant to be assigned directly. The coefficient &#945; is some function of the desired normalized cutoff frequency &#969;<sub>n</sub>. One can obtain a coefficient such that the -3dB cutoff frequency corresponds exactly to the desired frequency &#969;<sub>n</sub> by equating the magnitude of the difference equation's Z-transform to <img src="https://render.githubusercontent.com/render/math?math=\frac{1}{\sqrt{2}}"> (or -3dB), giving a value 

<img src="https://render.githubusercontent.com/render/math?math=\alpha = -b %2B \sqrt{b^2 %2B 2b}">

with 

<img src="https://render.githubusercontent.com/render/math?math=b = 1 - cos(\omega_n)">


which is expensive to compute. Common approximations are derived from discretizing the filter's differential equation using the finite differences method, yielding 

<img src="https://render.githubusercontent.com/render/math?math=\alpha = \frac{\omega_n}{\omega_n %2B 1}">

or by solving the differential equation and discretizing its transient response, yielding

<img src="https://render.githubusercontent.com/render/math?math=\alpha = 1 - e^{-\omega_n}">

which are also expensive to compute. We can plot each of the coefficient approximations as the normalized radian frequency varies up to &#969;<sub>n</sub> = &#960;. We can see that these curves are not self-similar, thus cannot be normalized and rescaled for different frequency ranges and sample rates as the exponential curves can. 

![Coefficients](/images/coeffs_light.png)

Unsigned 16-bit integer lookup tables of length 1024 for these three variants (scaled to Q16) can be generated using

```
> python tablegen.py coeff <method> <fmin> <fmax>
```

where `method` is one of `{z, diff, trans}`, and `fmin` and `fmax` are the frequency bounds, normalized such that (0, 0.5) corresponds to the range (0, &#960;), or (0, f<sub>s</sub>/2).

For example, at f<sub>s</sub> = 16kHz, an exact filter coefficient table mapping a 10-bit ADC reading to the range [16, 1600]Hz can be generated with

```
> python tablegen.py coeff z 0.001 0.1
```

Thus, the sample rate must be known at the time the table is generated to obtain accurate cutoff frequencies. 

A more flexible option for cutoff frequencies much less than f<sub>s</sub>/2 is to note that each coefficient table is approximately exponential for low frequencies, where the frequency &#969;<sub>n</sub> serves as a usable approximation of &#945;. This approximation yields a stable filter for &#969;<sub>n</sub> < 1, or <img src="https://render.githubusercontent.com/render/math?math=f < \frac{1}{\pi} \approx 0.3183 f_s">.

![Coefficients and Frequency Response](/images/coeffs_light.gif)

This means that we can often use normalized exponential tables with `PgmTable16` as filter coefficients without significant inaccuracy in the magnitude response. For example, the following three coefficient tables will result in filters with more or less the same frequency responses over a cutoff frequency range of approximately (0.2, 2000)Hz. 

#### Option 1
The following yields accurate cutoff frequencies over its range as long as f<sub>s</sub> = 16kHz. 

```
> python tablegen.py coeff z 1.25e-5 0.125
```

```C
#include "tables/coeff_z_u16x1024.h"

...

PgmTable16 coeff_table(coeff_z_u16x1024);	

...

ISR(ADC_vect) {
	...
	onepole.coeff = coeff_table.lookup(adc.results[0]); 
	...
}
```

#### Option 2
The following yields less accurate cutoff frequencies toward its maximum value, but sample rates can be changed without regenerating the table. 

```
> python tablegen.py exp 10000
```

```C
#include "tables/exp10000_u16x1024.h"

const float f_s = 16e3;

...

uint16_t scale = 2000.0/fs * 0xFFFF;
PgmTable16 coeff_table(exp10000_u16x1024, scale);	

...

ISR(ADC_vect) {
	...
	onepole.coeff = coeff_table.lookup_scale(adc.results[0]); 
	...
}
```

#### Option 3
The following yields less accurate cutoff frequencies toward its minimum value, but sample rates can be changed without regenerating the table. 

```
> python tablegen.py exp 10000
```

```C
#include "tables/exp10000_u16x1024.h"

const float f_s = 16e3;

...

float b = 1 - cosf(2000.0/f_s);
float alpha = -b + sqrtf(b*b + 2*b);
uint16_t scale = alpha * 0xFFFF;
PgmTable16 coeff_table(exp10000_u16x1024, scale);	

...

ISR(ADC_vect) {
	...
	onepole.coeff = coeff_table.lookup_scale(adc.results[0]); 
	...
}
```

## 7 LibAG Examples

The library's examples 0-3 use `Timer1` in PWM mode for 10-bit digital to analog conversion, and example 4 uses the `MCP4922` external DAC for 12-bit resolution. In examples 1-4, samples are processed at sample rate 10kHz using `Timer0` in CTC mode and an `ADCTimer0` instance configured to convert two control voltages on pins `A0` and `A1` in sequence for parameter control, giving a control rate of half the sample rate. 

For reconstruction of PWM or DAC outputs, a good starting point is to use the following Sallen-Key low pass filter, which has two poles, giving -12dB attenuation per octave.

![Sallen-Key](/images/sallen-key.png)

Using R = R1 = R2, and C = C1 = C2 gives a Q of 0.5 and a cutoff frequency <img src="https://render.githubusercontent.com/render/math?math=f_c=\frac{1}{2\pi RC}">. 

For the examples, R = 4.7k and C = 0.1u yields a cutoff frequency of 339Hz and about -50dB of attenuation at 5kHz. A similar filter should be used if the ADC is used to process periodic signals that may contain frequency components above the control rate. In either case, higher order filters can extend the passband while maintaining the same attenuation at the Nyquist rate. 

The op amp can be powered with a single supply if a bipolar supply is unavailable. If the op amp is supplied from the Arduino's 5V output, a rail-to-rail amplifier such as the TLV2372 can be used to allow use of the full [0, 5V] range.

### 0_MIDI

This example implements a basic MIDI to CV/Gate converter using `Timer1` configured for 12-bit PWM. The example illustrates the use of `MIDIDispatcher`, a minimal state machine for parsing MIDI message bytes and delegating control to user-defined message handlers. Only `Note On` and `Note Off` messages are handled in the example, but the dispatcher includes handlers for pitch bends, mod wheel, etc.

This example is the only one included which uses Arduino's `void loop()`, as it does not require signals to be generated at a fixed rate. `SoftwareSerial` is used to receive raw MIDI bytes and relay them to `MIDIDispatcher`. I recommend the following MIDI input circuit.

![MIDI In](/images/midi-in.png)

### 1_LFO

This example uses `Wavetable16` to implement a low-frequency sinusoidal oscillator, an exponential table with `PgmTable16` for variable frequency over [0.2, 200]Hz, and a Q16 multiply for variable amplitude.

### 2_ASR

This example uses `ASR16` to implement a linear, 16-bit envelope generator with Attack, Sustain, and Release states. Attack and release rates are set using an exponential table with `PgmTable16`. Lookup is reversed so CCW potentiometer rotation corresponds to short attacks and releases. A rising edge on `PD4` attacks and a falling edge releases. 

### 3_LPF

This example uses `OnePole16` to filter a square wave, writing the filter's low-pass output to `OCR1A` and the filter's high-pass output to `OCR1B`. The square wave is produced by thresholding a sawtooth waveform produced by `Phasor16`.

### 4_DAC

This example uses `MCP4922`, a subclass of `SPIMaster`, to control the external dual 12-bit SPI DAC of the same name. Its two channels are used to output sinusoidal waveforms offset by 90 degrees using the locally-defined `Quad16` oscillator class, is a subclass of `Wavetable16`.

### A note on sample timing

Examples 1-4 use pins `PD3` and `PD2` to monitor the timing of samples. `PD3` is toggled at the beginning of the sample processing interrupt, which if monitored on an oscilloscope should display a square wave at half the sample rate. `PD2` is set high at the beginning of the sample processing interrupt, and cleared at the end, which should display a pulse whose width scales with the runtime of the processing code. 

These examples provide ample headroom for expansion, and these timing pins should be monitored to ensure samples are generated on time as processing code is added. 

