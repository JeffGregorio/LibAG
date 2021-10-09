import numpy as np
import matplotlib.pyplot as plt

def print_c_array(array):
	print("const uint16_t table[] PROGMEM = {\n\t", end='')
	for n in range(len(array)):
		print('0x%04x' % int(np.round(array[n])), end='')
		if n == (len(array)-1):
			print('\n};')
		elif np.mod(n, 8) == 7:
			print(',\n\t', end='')
		else:
			print(', ', end='')
	return

# Sine wave table
def sine_table(length):
	table = (1 + np.sin(2 * np.pi * np.arange(length) / length)) / 2
	return table

# Exponential table
def exp_table(e0, e1, length):
	coeff = (e1 / e0) ** (1/(length-1))
	table = e0 * coeff ** np.arange(length)
	return table

def coeff_table(e0, e1, length):
	freq = exp_table(e0, e1, length)
	table = 2*np.pi*freq / (1 + 2*np.pi*freq)
	return table


bit_res = 16
bit_length = 10
ratio = 1000

# tab = exp_table(1.0/ratio, 1.0, 1 << bit_length)
tab = coeff_table(1.0/ratio, 1.0, 1 << bit_length)

fig, ax1 = plt.subplots()

color = 'tab:blue'
ax1.plot(tab, color=color)
ax1.set_xlabel('index')
ax1.set_ylabel('float', color=color)

tab *= (1 << bit_res) - 1

ax2 = ax1.twinx()
ax2.plot(tab)
ax2.set_xlabel('index')
ax2.set_ylabel('int')
ax2.tick_params(axis='y')

plt.show()

print_c_array(tab)