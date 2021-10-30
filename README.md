# LibAG

Audio synthesis library for AVR-based Arduino microcontrollers, developed and tested on the Atmega328 using Arduino Uno and Nano. 

## Audience

This library is designed to be an accessible entry point for Arduino hobbyists and engineering students wishing to explore real-time audio synthesis and embedded development a lower level of abstraction than similar libraries. It emphasizes readability, efficiency, and expandability over feature completeness and wide applicability to different architectures.

## Overview

The library offers a small set of peripheral drivers and example sketches that demonstrate configuration of timers, ADC, interrupt service routines, and SPI for use with the external MCP4921/2 12-bit DAC. Rudimentary knowledge of these peripherals is assumed. 

It also includes a small set of digital signal processing (DSP) objects and utility functions for efficient fixed point signal synthesis and processing, including table-based approaches to sinusoidal synthesis, exponential parameter control, and filter coefficients.

Memory use is minimized by storing pre-computed, normalized tables in flash memory and rescaling outputs. Several exponential tables are provided alongside a python script for generating others. 

## Digital to Analog Conversion

The library provides two methods for generating output. 

### PWM (`Timer.h`)
The classes `Timer0`, `Timer1`, and `Timer2` each have two associated pulse width modulation (PWM) channels mapped to the AVR's associated `OCRnA` and `OCRnB` pins, where `n` is the number of the timer. The PWM rate for timers 0 and 2 are determined by the system clock, the timer's prescaler, and 8-bit timer resolution, i.e. <img src="https://render.githubusercontent.com/render/math?math=f_{PWM}=16MHz/prescaler/2^8">. 

Prescalers are set with the timer's `init_pwm()` methods. Options for `Timer0` and `Timer1` include `{1, 8, 64, 256, 1024}`. Timer 2 has additional prescaler options including `{1, 8, 32 64, 128, 256, 1024}`. 

Each timer can be used in fast PWM mode by calling its `init_pwm()` method. Output values are written to pin OCRnA using `pwm_write_a()` and OCRnB `pwm_write_b()`, using 8-bit values for `Timer0` and `Timer2`, and up to 16-bit values for `Timer1`. 

The 16-bit `Timer1` allows trading decreased PWM rate for increased resolution. Its `init_pwm()` method takes a bit resolution parameter `N` that sets the value at which the timer resets to a power of two, yielding N-bit PWM at rate <img src="https://render.githubusercontent.com/render/math?math=f_{PWM}=16MHz/prescaler/2^N">. 

Note it is necessary to use PWM rates greater than the sample rate, which limits output resolution. This rate/resolution trade-off can be eliminated by using an external SPI DAC.

### DAC (`DACSPI.h`)

The class `DACSPI` provides a generic SPI interface, which is parent to `MCP4921` and `MCP4922`, which interface with (respectively) single and dual-channel 12-bit DACs of the same model number. `MCP4921::write()` writes a 12-bit value to the DAC's single output channels, and `MCP4922::write_a()` and `MCP4922::write_b()` write to the dual DAC's channels A and B.

## Processing at Sample Rate (`ADCAuto.h`)

It is often advantageous to have input and output conversions synced to the same rate or multiples of one another. Thus, processing may best be accomplished in the interrupt service routine `ISR(ADC_vec)`, which is called following ADC conversions. Classes defined in `ADCAuto.h` provide a number of ways in which ADC conversions can be triggered automatically. Each is instantiated with a specified number of channels to scan sequentially, up to 6 or 8 on the Atmega328's 28 and 32-pin variants, respectively. 

Each has a `set_prescaler()` method taking values `{2, 4, 8, 16, 32, 64, 128}`. Note that the AVR datasheet recommends ADC clock <img src="https://render.githubusercontent.com/render/math?math=CLK_{ADC}=16MHz/prescaler<200kHz"> for full 10-bit resolution, and <img src="https://render.githubusercontent.com/render/math?math=CLK_{ADC}<1MHz"> in all cases. 

Each class has an `init()` method, and an `update()` method that must be called from the `ISR(ADC_vec)` routine to convert the next channel in sequence. Conversions from any channel can then be retrieved from the instance's `results` array. 

The ADC can be configured in free-running mode using `ADCFreeRunning` at a rate determined by the system clock, its prescaler, and the thirteen clock cycles required to complete a conversion <img src="https://render.githubusercontent.com/render/math?math=f_{ADC}=16MHz/prescaler/13">. Thus, the free-running rate determines the maximum possible sample rate for a given system clock and ADC prescaler. 

Rates less than the maximum can be accomplished by triggering conversions using Timer 0's CTC mode with `ADCTimer0`. Note that `Timer0` must be independently configured in CTC mode with `Timer0::set_prescaler()` and `Timer0::init_ctc()`, providing the prescaler and an 8-bit OCR0A value, respectively. The `ISR(Timer0_COMPA_vect)` must be declared as well, which results in `ISR(ADC_vect)` being called at a rate <img src="https://render.githubusercontent.com/render/math?math=f_{ADC}=16MHz/prescaler_{Timer0}/OCR0A">. 

The ADC can also be triggered on the rising edge of an external clock signal on the INT0 pin using `ADCInt0`. 

## Note on fixed point format

Fixed point formats are typically denoted as QM.N for signed values or UQM.N unsigned, where M bits are used to represent the integer part, and N bits used for the fractional part. This library's DSP object and fixed point helper functions simplify by assuming no integer part, thus treating unsigned types (`uint8_t`, `uint16_t`, and `uint32_t`) as UQN values in the range <img src="https://render.githubusercontent.com/render/math?math=[0, \frac{2^{N}-1}{2^{N}}]"> and signed types (`int8_t`, `int16_t`, and `int32_t`) as QN values in <img src="https://render.githubusercontent.com/render/math?math=[-1, \frac{2^{N-1}-1}{2^{N-1}}]">.

## Note on frequency normalization

Processing in discrete time imposes an upper frequency bound <img src="https://render.githubusercontent.com/render/math?math=|f|<\frac{f_s}{2}"> dictated by the Nyquist-Shannon sampling theorem. That is, we cannot capture, process, or synthesize any frequency whose magnitude is greater than half the sample rate.

Thus, the frequency interval <img src="https://render.githubusercontent.com/render/math?math=(-\frac{f_s}{2}, \frac{f_s}{2}) Hz"> is typically mapped onto the interval <img src="https://render.githubusercontent.com/render/math?math=(-\pi, \pi)rad"> for correspondence with the domain of periodic functions such as sin, cos, and the complex exponential. This is done using the function 

<img src="https://render.githubusercontent.com/render/math?math=\omega_n(f) = 2\pi\frac{f}{f_s}"> 

and ensuring the magnitude of frequency <img src="https://render.githubusercontent.com/render/math?math=f"> never excedes <img src="https://render.githubusercontent.com/render/math?math=\frac{f_s}{2}">. 

In a fixed point context, it is advantageous to redefine this domain according to the resolution of an integer type, as single periods of periodic functions can be stored in lookup tables with integer length, and we can use integer overflow to guarantee we stay within this domain. 

Thus, the library's DSP objects assume Q16 frequency, where the frequency range or <img src="https://render.githubusercontent.com/render/math?math=(-\frac{f_s}{2}, \frac{f_s}{2}) Hz"> maps onto the signed integer range <img src="https://render.githubusercontent.com/render/math?math=[-2^15, 2^{15}-1]">. 

The effective frequency resolution is <img src="https://render.githubusercontent.com/render/math?math=\frac{f_s}{2^{16}}">. Desired frequencies in Hz are converted to normalized Q16 frequency using

<img src="https://render.githubusercontent.com/render/math?math=(2^{16}-1)\frac{f}{f_s}"> 

## Table Generation

Though tables can be computed at startup and stored in SRAM, space is very limited (2kB on the Atmega328 and 8kB on the Atmega2560). Rather, pre-computed tables can be stored in flash memory (up to 32kB on the Atmega328 and 256kB on the Atmega2560) and read using macros defined in the standard avr-gcc library `<avr/pgmspace.h>`. LibAG classes including `Wavetable16` and `PgmTable16` take pointers to these table addresses and handle lookup and output scaling.

The library provides a python script `tablegen.py` which can be used to generate C header files containing tables declared in flash memory. The script has no dependencies for generating tables and writing files, but if the user has [Matplotlib](https://matplotlib.org/) installed, the script will generate plots of the resulting table functions.

### Sine
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
Wavetable16 lfo(sine_u16x1024, 6);
```

Note that due to the division via right shift, table lengths must be a power of two for use with Wavetable16.

### Exponential

Exponential tables of length N can be generated over a nonzero range <img src="https://render.githubusercontent.com/render/math?math=(e_0, e_1)"> using

<img src="https://render.githubusercontent.com/render/math?math=y[n] = e_0 \cdot c^{n}"> 

where 

<img src="https://render.githubusercontent.com/render/math?math=c = \sqrt[N-1]{\frac{e_1}{e_0}}">

Rather than generate separate tables over specific Q16 frequency ranges, it is useful to normalize the table such that the maximum value is `0xFFFF`, and dynamically re-scale the output to a maximum value less than `0xFFFF` using a Q multiply. This places the minimum value at <img src="https://render.githubusercontent.com/render/math?math=\frac{0xFFFF}{ratio}">, where the ratio is <img src="https://render.githubusercontent.com/render/math?math=\frac{e1}{e0}">.

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

The table can be used with a `PgmTable16` instance's `lookup_scale()` method. The scaling factor can be determined at startup or changed dynamically. 

For example, a 10-bit ADC reading can be used to control an exponential Q16 frequency sweep in the range <img src="https://render.githubusercontent.com/render/math?math=[0.2, 200.0] Hz"> using

```C
#include "tables/exp1000_u16x1024.h"
float fs = 16e3;
uint16_t scale = 200.0/fs * 0xFFFF;
PgmTable16 freq_table(exp1000_u16x1024, scale);
```

### Filter Coefficients 

The `OnePole16` and `OnePole16_LF` (low-frequency) objects declared in `IIR.h` implement a multimode filter with variable cutoff frequency. The filter implements the difference equation of an exponential moving average filter

<img src="https://render.githubusercontent.com/render/math?math=y[n] = y[n-1] %2B \alpha(x[n] - y[n-1])">

The `OnePole` classes provide no setter methods for the coefficient, as it is meant to be assigned directly. The coefficient <img src="https://render.githubusercontent.com/render/math?math=\alpha"> is some function of the desired normalized cutoff frequency <img src="https://render.githubusercontent.com/render/math?math=\omega_n">. One can obtain a coefficient such that the -3dB cutoff frequency corresponds exactly to the desired frequency <img src="https://render.githubusercontent.com/render/math?math=\omega_n"> by equating the magnitude of the difference equation's Z-transform to <img src="https://render.githubusercontent.com/render/math?math=\frac{1}{\sqrt{2}}"> (or -3dB), giving a value 

<img src="https://render.githubusercontent.com/render/math?math=\alpha = -b %2B \sqrt{b^2 %2B 2b}">

with 

<img src="https://render.githubusercontent.com/render/math?math=b = 1 - cos(\omega_n)">


which is expensive to compute. Common approximations are derived from discretizing the filter's differential equation using the finite differences method, yielding 

<img src="https://render.githubusercontent.com/render/math?math=\alpha = \frac{\omega_n}{\omega_n %2B 1}">

or by solving the differential equation and discretizing its transient response, yielding

<img src="https://render.githubusercontent.com/render/math?math=\alpha = 1 - e^{-\omega_n}">

which are also expensive to compute. We can plot each of the coefficient approximations as the normalized radian frequency varies up to <img src="https://render.githubusercontent.com/render/math?math=\omega_n = \pi">. We can see that these curves are not self-similar, thus cannot be normalized and rescaled for different frequency ranges and sample rates as the exponential curves can. 

![Coefficients](/images/coeffs_light.png)

Unsigned 16-bit integer lookup tables of length 1024 for these three variants (scaled to Q16) can be generated using

```
> python tablegen.py coeff <method> <fmin> <fmax>
```

where `method` is one of `{z, diff, trans}`, and `fmin` and `fmax` are the frequency bounds, normalized such that <img src="https://render.githubusercontent.com/render/math?math=(0, 0.5)"> corresponds to the range <img src="https://render.githubusercontent.com/render/math?math=(0, \pi)">, or <img src="https://render.githubusercontent.com/render/math?math=(0, \frac{f_s}{2})">.

For example, at <img src="https://render.githubusercontent.com/render/math?math=f_s = 16kHz">, an exact filter coefficient table mapping a 10-bit ADC reading to the range <img src="https://render.githubusercontent.com/render/math?math=(16, 1600)Hz"> can be generated with

```
> python tablegen.py coeff z 0.001 0.1
```

Thus, the sample rate must be known at the time the table is generated to obtain accurate cutoff frequencies. 

A more flexible option for cutoff frequencies much less than <img src="https://render.githubusercontent.com/render/math?math=\frac{f_s}{2}"> is to note that each coefficient table is approximately exponential for low frequencies, where the frequency <img src="https://render.githubusercontent.com/render/math?math=\omega_n"> serves as a usable approximation of <img src="https://render.githubusercontent.com/render/math?math=\alpha">. This approximation yields a stable filter for <img src="https://render.githubusercontent.com/render/math?math=\omega_n < 1">, or <img src="https://render.githubusercontent.com/render/math?math=f < \frac{1}{\pi} \approx 0.3183 f_s">.

![Coefficients and Frequency Response](/images/coeffs_light.gif)

This means that we can often use normalized exponential tables with `PgmTable16` as filter coefficients without significant inaccuracy in the magnitude response. For example, the following three coefficient tables will result in filters with more or less the same frequency responses over a cutoff frequency range of apprxoimately <img src="https://render.githubusercontent.com/render/math?math=(0.2, 2000)Hz">. 

#### Option 1
The following yields accurate cutoff frequencies over its range as long as <img src="https://render.githubusercontent.com/render/math?math=f_s = 16kHz">

```
> python tablegen.py coeff z 1.25e-5 0.125
```

```C
#include "tables/coeff_z_u16x1024.h"
PgmTable16 coeff_table(coeff_z_u16x1024);	
...
onepole.coeff = coeff_table.lookup(adc_value); 
```

#### Option 2
The following yields less accurate cutoff frequencies toward its maximum value, but sample rates can be changed without regenerating the table. 

```
> python tablegen.py exp 10000
```

```C
#include "tables/exp10000_u16x1024.h"
float f_s = 16e3;
uint16_t scale = 2000.0/fs * 0xFFFF;
PgmTable16 coeff_table(exp10000_u16x1024, scale);
...
onepole.coeff = coeff_table.lookup_scale(adc_value);
```

#### Option 3
The following yields less accurate cutoff frequencies toward its minimum value, but sample rates can be changed without regenerating the table. 

```
> python tablegen.py exp 10000
```

```C
#include "tables/exp10000_u16x1024.h"
float f_s = 16e3;
float b = 1 - cosf(2000.0/f_s);
float alpha = -b + sqrtf(b*b + 2*b);
uint16_t scale = alpha * 0xFFFF;
PgmTable16 coeff_table(exp10000_u16x1024, scale);
...
onepole.coeff = coeff_table.lookup_scale(adc_value);
```

## LibAG Examples

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

This example uses `MCP4922` to control the external dual 12-bit SPI DAC of the same name. Its two channels are used to output sinusoidal waveforms offset by 90 degrees using the locally-defined `Quad16` oscillator class, which inherits from `Wavetable16`.

### A note on sample timing

Examples 1-4 use pins `PD3` and `PD2` to monitor the timing of samples. `PD3` is toggled at the beginning of the sample processing interrupt, which if monitored on an oscilloscope should display a square wave at half the sample rate. `PD2` is set high at the beginning of the sample processing interrupt, and cleared at the end, which should display a pulse whose width scales with the runtime of the processing code. 

These examples provide ample headroom for expansion, and these timing pins should be monitored to ensure samples are generated on time as processing code is added. 

