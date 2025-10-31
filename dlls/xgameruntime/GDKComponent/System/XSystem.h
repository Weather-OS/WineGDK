/*
 * Xbox Game runtime Library
 *  GDK Component: System API -> XSystem
 * 
 * Written by Weather
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef XSYSTEM_H
#define XSYSTEM_H

#include "../../private.h"

#include <string.h>

struct x_system
{
    IXSystemImpl IXSystemImpl_iface;
    LONG ref;
};

const SIZE_T XSystemConsoleIdBytes = 39;
const SIZE_T XSystemXboxLiveSandboxIdMaxBytes = 16;
const SIZE_T XSystemAppSpecificDeviceIdBytes = 45;

#endif