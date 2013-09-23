/*
 
 CinsImp <http://www.cinsimp.net>
 Copyright (C) 2009-2013 Joshua Hawcroft
 
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
 
 *************************************************************************************************
 JHCinsImp: General Assertions
 
 */

#include <assert.h>

#ifndef CinsImp_jhassert_h
#define CinsImp_jhassert_h


#define IS_BOOL(x) ((x == !0) || (x == !!0))


#define IS_LINE_NUMBER(x) ((x >= 1) && (x <= 100000000))
#define IS_INVALID_LINE_NUMBER(x) (x == 0)


#endif
