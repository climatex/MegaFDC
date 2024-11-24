// MegaFDC (c) 2023-2024 J. Bogin, http://boginjr.com
// Floppy disk controller interrupt service routines

#pragma once

// public for FDC
void FDCACK();
void FDCREAD();
void FDCVERIFY();
void FDCWRITE();