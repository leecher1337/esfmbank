/*
 * OPL Bank Editor by Wohlstand, a free tool for music bank editing
 * Copyright (c) 2016-2023 Vitaly Novichkov <admin@wohlnet.ru>
 *
 * OPN2 Bank Editor by Wohlstand, a free tool for music bank editing
 * Copyright (c) 2017-2023 Vitaly Novichkov <admin@wohlnet.ru>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef INSTRUMENTNAMES_H
#define INSTRUMENTNAMES_H

#pragma pack(push, 1)
struct MidiProgram
{
    //! Kind of instrument. 'M' melodic 'P' percussive
    char kind;
    //! Bank identifier MSB
    unsigned char bankMsb;
    //! Bank identifier LSB. (if percussive, this is the program number)
    unsigned char bankLsb;
    //! Program number (if percussive, this is the key number)
    unsigned char program;
    //! Bank name
    const char *bankName;
    //! Patch name
    const char *patchName;
};
#pragma pack(pop) 

#endif // INSTRUMENTNAMES_H 
