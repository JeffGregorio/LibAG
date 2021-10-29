/*
  PgmTable.h

  Lookup and scaling for tables stored in program memory.
  
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

#ifndef PGMTABLE_H
#define PGMTABLE_H

/*
 * Unsigned 16-bit table lookup with UQ16 scaling factor
 */
struct PgmTable16 {

	/*
	 *  Constructors
	 */
	PgmTable16(uint16_t *table) : table(table), scale(0xFFFF) {
		;	// Do nothing
	}
	PgmTable16(uint16_t *table, uint16_t scale) : table(table), scale(scale) {
		; 	// Do nothing
	}

	/*
	 * 	Table lookup, direct or scaled by UQ16 multiply
	 */
	uint16_t lookup(uint16_t idx) {
		return (uint16_t)pgm_read_ptr(table + idx);
	}
	uint16_t lookup_scale(uint16_t idx) {
		return (uint32_t)scale * (uint16_t)pgm_read_ptr(table + idx) >> 16;
	}

	uint16_t *table;
	uint16_t scale;
};
 
#endif