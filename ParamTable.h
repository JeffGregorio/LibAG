/*
   ParamTable.h
  
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
*/  

#ifndef PARAMTABLE_H
#define PARAMTABLE_H

/*
 * Unsigned 16-bit parameter table with UQ16 scaling factor
 */
struct ParamTable16 {

	/*
	 *  Constructor
	 */
	ParamTable16(uint16_t *table, uint16_t scale) : table(table), scale(scale) {
		;	// Do nothing
	}

	/*
	 * 	Table lookup, scaled by UQ16 multiply
	 */
	uint16_t lookup(uint16_t idx) {
		return (uint32_t)scale * (uint16_t)pgm_read_ptr(table + idx) >> 16;
	}

	uint16_t *table;
	uint16_t scale;
};

#endif