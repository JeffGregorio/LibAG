"""
   tablegen.py
  
  Copyright (C) 2021 Jeff Gregorio
  
  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
"""

import os
import sys
import argparse
import math

def tab_print(array, dtype, vname):

	# Type names for argument specification
	typenames = {
		'u8' : 'uint8_t', 
		's8' : 'int8_t', 
		'u16': 'uint16_t', 
		's16': 'int16_t',
		'u32': 'uint32_t',
		's32': 'int32_t'
	}

	# Vertical resolution of type
	y_res = int(dtype[1:])
	if y_res == 8:
		fmt_str = '0x%02x'
	elif y_res == 16:
		fmt_str = '0x%04x'
	elif y_res == 32:
		fmt_str = '0x%08x'

	# Variable declaration
	print("const %s %s[] PROGMEM = {\n\t" % (typenames[dtype], vname), end='')

	# Values
	for n in range(len(array)):
		val = int(round(array[n]))
		print('-' if val < 0 else ' ', end='')
		print(fmt_str % abs(val), end='')
		if n == (len(array)-1):
			print('\n};')
		elif n % 8 == 7:
			print(',\n\t', end='')
		else:
			print(', ', end='')
	return

# Scale a (0, 1) normalized table to signed or unsigned int range 
# specified by type in {u8, s8, u16, s16, u32, s32}
def tab_scale(table, dtype):
	out = []
	y_res = int(dtype[1:])
	for n in range(len(table)):
		out.append(table[n] * ((1 << y_res) - 1))
	if dtype[0] == 's':
		for n in range(args.tablen):
			out[n] -= (1 << (y_res-1))
	return out

# Sine wave table
def table_sine(length):
	table = []
	for n in range(length):
		table.append(0.5 *  math.sin(2 * math.pi * n / length) + 0.5)
	return table

# Exponential frequency table
def table_exp(e0, e1, length):
	table = []
	coeff = (e1 / e0) ** (1/(length-1))
	for n in range(length):
		table.append(e0 * coeff ** n)
	return table

# Volts per octave table
def table_voct(ref, n_octs, length):
	table = []
	div = length / n_octs
	cent = length >> 1
	for n in range(length):
		table.append(ref * pow(2.0, (n - cent) / div))
	return table

# Exact coefficient table
def table_coeff_z(omega_n):
	table = []
	for i in range(len(omega_n)):
		b = 1 - math.cos(omega_n[i])
		table.append(-b + (b*b + 2*b) ** 0.5)
	return table

# Coefficient table for finite differences method
def table_coeff_diff(omega_n):
	table = []
	for i in range(len(omega_n)):
		table.append(omega_n[i] / (1 + omega_n[i]))
	return table

# Coefficient table for transient response discretization method
def table_coeff_trans(omega_n):
	table = []
	for i in range(len(omega_n)):
		table.append(1 - math.exp(-omega_n[i]))
	return table

# TPT onepole filter coefficient
def table_coeff_tpt(omega_n):
	table = []
	for i in range(len(omega_n)):
		table.append(math.tan(omega_n[i]/2))
		table[i] /= (1 + table[i])
	return table

# Main
if __name__ == "__main__":

	# Create parser for command line arguments
	parser = argparse.ArgumentParser(
		description='Precompute wave/parameter tables')

	# Data type
	parser.add_argument('-T', '--dtype', 
		choices=['u8', 'u16', 'u32', 's8', 's16', 's32'], 
		default='u16', 
		dest='dtype',
		help='C integer data type')

	# Table length
	parser.add_argument('-L', '--length', 
		type=int, 
		default=1024, 
		dest='length',
		help='Table length')

	# Subparsers for different table functions
	subparser = parser.add_subparsers(
		help='Table to generate', 
		dest='func')

	# Sine-specific options
	sine_parser = subparser.add_parser('sine')

	# Exponential-specific options
	exp_parser = subparser.add_parser('exp')
	exp_parser.add_argument('ratio', 
		type=float, 
		help='Max/Min Ratio')

	# V/oct-specific options
	voct_parser = subparser.add_parser('voct')
	voct_parser.add_argument('ref', 
		type=float, 
		help='Reference frequency in (0, 0.5)')
	voct_parser.add_argument('n_octs', 
		type=float, 
		help='Number of octaves')

	# Coefficient-specific options
	coeff_parser = subparser.add_parser('coeff')
	coeff_parser.add_argument('method',
		choices=['z', 'diff', 'trans', 'tpt'],
		default='z',
		help='Coefficient calculation method')
	coeff_parser.add_argument('fmin', 
		type=float, 
		help='Minimum frequency in (0, 0.5)')
	coeff_parser.add_argument('fmax', 
		type=float, 
		help='Maximum frequency in (0, 0.5)')

	# Parse
	args = parser.parse_args()

	# Compute the specified table
	prefix = args.func
	if args.func == 'sine':
		tab = table_sine(args.length)
	elif args.dtype[0] != 'u':
		raise TypeError('Signed types only valid for sine table')
	elif args.func == 'exp':
		tab = table_exp(1.0/args.ratio, 1.0, args.length)
		prefix += str(int(round(args.ratio)))
	elif args.func == 'voct':
		tab = table_voct(440.0/15638.0, args.n_octs, args.length)
		prefix += ''
	elif args.func == 'coeff':
		if args.method == 'z':
			tab = table_coeff_z(table_exp(args.fmin, args.fmax, args.length))
			prefix += '_z'
		elif args.method == 'diff':
			tab = table_coeff_diff(table_exp(args.fmin, args.fmax, args.length))
			prefix += '_diff'
		elif args.method == 'trans':
			tab = table_coeff_trans(table_exp(args.fmin, args.fmax, args.length))
			prefix += '_trans'
		elif args.method == 'tpt':
			tab = table_coeff_tpt(table_exp(args.fmin, args.fmax, args.length))
			prefix += '_tpt'

	# Scale to integer range
	tab_out = tab_scale(tab, args.dtype)

	# Print to file
	name = '%s_%sx%d' % (prefix, args.dtype, args.length)
	num = 0
	outpath = os.path.relpath('tables')
	outfile = os.path.join(outpath, name + '.h')
	# while os.path.exists(outfile):
	# 	num += 1
	# 	outfile = os.path.join(outpath, name + '(%d).h' % num)
	with open(outfile, 'w') as f:
		dflt_stdout = sys.stdout
		sys.stdout = f
		print('#ifndef ' + name.upper() + '_H')
		print('#define ' + name.upper() + '_H')
		tab_print(tab_out, args.dtype, name)
		print('#endif')
		sys.stdout = dflt_stdout
 
	# Plotting (optional, with matplotlib)
	try: 
		import matplotlib.pyplot as plt
		fig, ax1 = plt.subplots()
		ax1.plot(tab)
		ax1.set_xlabel('index')
		ax1.set_ylabel('float')

		r = args.fmax / args.fmin
		# r *= 0.7
		t1 = table_exp(1/r, 1, args.length)
		t1 = [v * tab[-1] for v in t1]
		ax1.plot(t1)

		ax2 = ax1.twinx()
		ax2.plot(tab_out)
		ax2.set_xlabel('index')
		ax2.set_ylabel('int')
		ax2.tick_params(axis='y')

		t1o = tab_scale(t1, args.dtype)
		ax2.plot(t1o)

		plt.show()
	except:
		pass # Don't plot