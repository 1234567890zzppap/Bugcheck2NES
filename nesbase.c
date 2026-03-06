/************************************************************/
/*    SMOLNES by Ben Smith (Oct 2022) (updated Jun 2025)    */
/* -------------------------------------------------------- */
/*                                                          */
/* A NES emulator that can play mapper 0/1/2/3/4/7 games.   */
/* Requires gcc/clang and Linux/macOS.                      */
/*                                                          */
/* $ cc smolnes.c -O2 -lSDL2 -o smolnes                     */
/* $ smolnes <rom.nes>                                      */
/*                                                          */
/* Controls: DPAD=Arrow keys B=Z A=X Start=Enter Select=Tab */
/*                                                          */
/************************************************************/
// https://github.com/binji/smolnes/blob/main/smolnes.c
//#define WIN32_LEAN_AND_MEAN
// #include <windows.h>
#include "bootvid.h"
#include "stdint.h"
#include <ntddk.h>
#include <stdio.h>
#include <stdlib.h>
#include "stdbool.h"
#include "gamedata.h"

#define SCREEN_W 256
#define SCREEN_H 240

static uint32_t frame_buffer[SCREEN_W * SCREEN_H];

#pragma warning(disable:4013)

PKBUGCHECK_CALLBACK_RECORD BugcheckCallbackRecord = NULL;

#define PULL mem(++S, 1, 0, 0)
#define PUSH(x) mem(S--, 1, x, 1)

#define NES_WIDTH  256
#define NES_HEIGHT 240

BOOLEAN UsingCapsOrNot = FALSE;

static char ktocSHIFT(uint8_t key) {
	char c = 0;
	int i = 0;
	uint8_t dict[2][94] = {
		{41, 2,	 3,	 4,	 5,	 6,	 7,	 8,	 9,	 10, 11, 12, 13, 26, 27, 43,
		 39, 40, 51, 52, 53, 30, 48, 46, 32, 18, 33, 34, 35, 23, 36, 37,
		 38, 50, 49, 24, 25, 16, 19, 31, 20, 22, 47, 17, 45, 21, 44, 57},
		{126, 33, 64, 35, 36, 37, 94, 38, 42, 40, 41, 95, 43, 123, 125, 124,
		 58,  34, 60, 62, 63, 65, 66, 67, 68, 69, 70, 71, 72, 73,  74,	75,
		 76,  77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89,  90,	32} };
	for (i = 0; i < 94; i++) {
		if (dict[0][i] == key) {
			c = (char)dict[1][i];
		}
	}
	return c;
}


static char ktoc(uint8_t key) {
	char c = 0;
	int i = 0 ;
	uint8_t dict[2][94] = {
		{57, 40, 51, 12, 52, 53, 11, 2,	 3,	 4,	 5,	 6,	 7,	 8,	 9,	 10,
		 39, 13, 26, 43, 27, 41, 30, 48, 46, 32, 18, 33, 34, 35, 23, 36,
		 37, 38, 50, 49, 24, 25, 16, 19, 31, 20, 22, 47, 17, 45, 21, 44},
		{32,  39,  44,	45,	 46,  47,  48,	49,	 50,  51,  52,	53,
		 54,  55,  56,	57,	 59,  61,  91,	92,	 93,  96,  97,	98,
		 99,  100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110,
		 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122} };
	for (i = 0; i < 94; i++) {
		if (dict[0][i] == key) {
			c = (char)dict[1][i];
		}
	}
	return c;
}

static const uint8_t bv_palette[16][3] = {
    {0, 0, 0},        // Black
    {128, 0, 0},      // Red
    {0, 128, 0},      // Green
    {128, 128, 0},    // Brown
    {0, 0, 128},      // Blue
    {128, 0, 128},    // Magenta
    {0, 128, 128},    // Cyan
    {192, 192, 192},  // Dark Gray
    {128, 128, 128},  // Light Gray
    {255, 0, 0},      // Light Red
    {0, 255, 0},      // Light Green
    {255, 255, 0},    // Yellow
    {0, 0, 255},      // Light Blue
    {255, 0, 255},    // Light Magenta
    {0, 255, 255},    // Light Cyan
    {255, 255, 255},  // White
};

void viddraw();
void keychecker();

int mini_rv32ima_key_hit(void) {
	return (__inbyte(0x64) & 1);
}

int mini_rv32ima_get_key(void) {
	char key = __inbyte(0x60);
	if (key == 0x1c) return '\r';
	if (key == 0x3A) { UsingCapsOrNot = !UsingCapsOrNot; return -1; }
	if (UsingCapsOrNot) return ktocSHIFT(key);
	else return ktoc(key);
}

uint32_t mini_rv32ima_handle_other_csr_read()
{
    
        if (!mini_rv32ima_key_hit())
        {
            return (uint32_t)0xFFFFFFFFUL;
        }
        return (uint32_t)(mini_rv32ima_get_key() & 0xFF);
    
    return 0;
}



static unsigned short nes_palette[64] =
{
    25356, 34816, 39011, 30854, 24714, 4107, 106, 2311,
    2468, 2561, 4642, 6592, 20832, 0, 0, 0,
    44373, 49761, 55593, 51341, 43186, 18675, 434, 654,
    4939, 5058, 3074, 19362, 37667, 0, 0, 0,
    0xFFFF, 0xFCED, 64497, 64342, 62331, 43932, 23612, 9465,
    1429, 1550, 20075, 36358, 52713, 16904, 0, 0,
    0xFFFF, 0xFEB8, 0xFE56, 0xFE34, 0xFE12, 58911, 50814, 42620,
    40667, 40729, 48951, 53078, 61238, 44405
};

uint8_t *rom, *chrrom,                  // Points to the start of PRG/CHR ROM
    prg[4], chr[8],                     // Current PRG/CHR banks
    prgbits = 14, chrbits = 12,         // Number of bits per PRG/CHR bank
    A, X, Y, P = 4, S = ~2, _PCH, _PCL, // CPU Registers
    addr_lo, addr_hi,                   // Current instruction address
    nomem,  // 1 => current instruction doesn't write to memory
    result, // Temp variable
    val,    // Current instruction value
    cross,  // 1 => page crossing occurred
    tmp,    // Temp variables
    ppumask, ppuctrl, ppustatus, // PPU registers
    ppubuf,                      // PPU buffered reads
    W,                           // Write toggle PPU register
    fine_x,                      // X fine scroll offset, 0..7
    opcode,                      // Current instruction opcode
    nmi_irq,                     // 1 => IRQ occurred
                                 // 4 => NMI occurred
    ntb,                         // Nametable byte
    ptb_lo,                      // Pattern table lowbyte
    vram[2048],                  // Nametable RAM
    palette_ram[64],             // Palette RAM
    ram[8192],                   // CPU RAM
    chrram[8192],                // CHR RAM (only used for some games)
    prgram[8192],                // PRG RAM (only used for some games)
    oam[256],                    // Object Attribute Memory (sprite RAM)
    mask[] = {128, 64, 1, 2,     // Masks used in branch instructions
              1,   0,  0, 1, 4, 0, 0, 4, 0,
              0,   64, 0, 8, 0, 0, 8}, // Masks used in SE*/CL* instructions.
    keys,                              // Joypad shift register
    mirror,                            // Current mirroring mode
    mmc1_bits, mmc1_data, mmc1_ctrl,   // Mapper 1 (MMC1) registers
    mmc3_chrprg[8], mmc3_bits,         // Mapper 4 (MMC3) registers
    mmc3_irq, mmc3_latch,              //
    chrbank0, chrbank1, prgbank;       // Current PRG/CHR bank

uint16_t scany,          // Scanline Y
    T, V,                // "Loopy" PPU registers
    sum,                 // Sum used for ADC/SBC
    dot,                 // Horizontal position of PPU, from 0..340
    atb,                 // Attribute byte
    shift_hi, shift_lo,  // Pattern table shift registers
    cycles;              // Cycle count for current instruction
    //frame_buffer[61440]; // 256x240 pixel frame buffer. Top and bottom 8 rows
                         // are not drawn.

int shift_at = 0;
// input
uint8_t controller1 = 0;

// Read a byte from CHR ROM or CHR RAM.
uint8_t *get_chr_byte(uint16_t a) {
  return &chrrom[chr[a >> chrbits] << chrbits | a % (1 << chrbits)];
}

// Read a byte from nametable RAM.
uint8_t *get_nametable_byte(uint16_t a) {
  return &vram[mirror == 0   ? a % 1024                  // single bank 0
               : mirror == 1 ? a % 1024 + 1024           // single bank 1
               : mirror == 2 ? a & 2047                  // vertical mirroring
                             : a / 2 & 1024 | a % 1024]; // horizontal mirroring
}

// If `write` is non-zero, writes `val` to the address `hi:lo`, otherwise reads
// a value from the address `hi:lo`.
uint8_t mem(uint8_t lo, uint8_t hi, uint8_t val, uint8_t write) {
  uint16_t addr = hi << 8 | lo;

  switch (hi >>= 4) {
  case 0: case 1: // $0000...$1fff RAM
    return write ? ram[addr] = val : ram[addr];

  case 2: case 3: // $2000..$2007 PPU (mirrored)
    lo &= 7;

    // read/write $2007
    if (lo == 7) {
      uint8_t *rom;
      tmp = ppubuf;
      rom =
          // Access CHR ROM or CHR RAM
          V < 8192 ? write && chrrom != chrram ? &tmp : get_chr_byte(V)
          // Access nametable RAM
          : V < 16128 ? get_nametable_byte(V)
                      // Access palette RAM
                      : palette_ram + (uint8_t)((V & 19) == 16 ? V ^ 16 : V);
      write ? *rom = val : (ppubuf = *rom);
      V += ppuctrl & 4 ? 32 : 1;
      V %= 16384;
      return tmp;
    }

    if (write)
      switch (lo) {
      case 0: // $2000 ppuctrl
        ppuctrl = val;
        T = T & 0xf3ff | val % 4 << 10;
        break;

      case 1: // $2001 ppumask
        ppumask = val;
        break;

      case 5: // $2005 ppuscroll
        T = (W ^= 1)
          ? fine_x = val & 7, T & ~31 | val / 8
          : T & 0x8c1f | val % 8 << 12 | val * 4 & 0x3e0;
        break;

      case 6: // $2006 ppuaddr
        T = (W ^= 1)
          ? T & 0xff | val % 64 << 8
          : (V = T & ~0xff | val);
      }

    if (lo == 2) { // $2002 ppustatus
      tmp = ppustatus & 0xe0;
      ppustatus &= 0x7f;
      W = 0;
      return tmp;
    }
    break;

  case 4:
  {
  uint16_t i = 0;
  uint16_t base = val << 8;
    if (write && lo == 20) // $4014 OAM DMA
      for (i = 0; i < 256; i++)
{
    oam[i] = ram[(base + i) & 0x7FF];
}
    // $4016 Joypad 1
    tmp = controller1;
    if (lo == 22) {
      if (write) {
        keys = tmp;
      } else {
        tmp = keys & 1;
        keys /= 2;
        return tmp;
      }
    }
    return 0;}

  case 6: case 7: // $6000...$7fff PRG RAM
    addr &= 8191;
    return write ? prgram[addr] = val : prgram[addr];

  default: // $8000...$ffff ROM
    // handle mapper writes
    if (write)
      switch (rombuf[6] >> 4) {
      case 7: // mapper 7
        mirror = !(val / 16);
        prg[0] = val % 8 * 2;
        prg[1] = prg[0] + 1;
        break;

      case 4: {int i; // mapper 4
        uint8_t addr1 = addr & 1;
        switch (hi >> 1) {
          case 4: // Bank select/bank data
            *(addr1 ? &mmc3_chrprg[mmc3_bits & 7] : &mmc3_bits) = val;
            tmp = mmc3_bits >> 5 & 4;
            for (i = 4; i--;) {
              chr[0 + i + tmp] = mmc3_chrprg[i / 2] & ~!(i % 2) | i % 2;
              chr[4 + i - tmp] = mmc3_chrprg[2 + i];
            }
            tmp = mmc3_bits >> 5 & 2;
            prg[0 + tmp] = mmc3_chrprg[6];
            prg[1] = mmc3_chrprg[7];
            prg[3] = rombuf[4] * 2 - 1;
            prg[2 - tmp] = prg[3] - 1;
            break;
          case 5: // Mirroring
            if (!addr1) {
              mirror = 2 + val % 2;
            }
            break;
          case 6:  // IRQ Latch
            if (!addr1) {
              mmc3_latch = val;
            }
            break;
          case 7:  // IRQ Enable
            mmc3_irq = addr1;
            break;
        }
        break;
      }

      case 3: // mapper 3
        chr[0] = val % 4 * 2;
        chr[1] = chr[0] + 1;
        break;

      case 2: // mapper 2
        prg[0] = val & 31;
        break;

      case 1: // mapper 1
        if (val & 0x80) {
          mmc1_bits = 5;
          mmc1_data = 0;
          mmc1_ctrl |= 12;
        } else if (mmc1_data = mmc1_data / 2 | val << 4 & 16, !--mmc1_bits) {
          mmc1_bits = 5;
          tmp = addr >> 13;
          *(tmp == 4 ? mirror = mmc1_data & 3, &mmc1_ctrl
          : tmp == 5 ? &chrbank0
          : tmp == 6 ? &chrbank1
                     : &prgbank) = mmc1_data;

          // Update CHR banks.
          chr[0] = chrbank0 & ~!(mmc1_ctrl & 16);
          chr[1] = mmc1_ctrl & 16 ? chrbank1 : chrbank0 | 1;

          // Update PRG banks.
          tmp = mmc1_ctrl / 4 % 4 - 2;
          prg[0] = !tmp ? 0 : tmp == 1 ? prgbank : prgbank & ~1;
          prg[1] = !tmp ? prgbank : tmp == 1 ? rombuf[4] - 1 : prgbank | 1;
        }
      }
    return rom[(prg[hi - 8 >> prgbits - 12] & (rombuf[4] << 14 - prgbits) - 1)
                   << prgbits |
               addr & (1 << prgbits) - 1];
  }

  return ~0;
}

// Read a byte at address `PCH:PCL` and increment PC.
uint8_t read_pc() {
  val = mem(_PCL, _PCH, 0, 0);
  !++_PCL && ++_PCH;
  return val;
}

// Set N (negative) and Z (zero) flags of `P` register, based on `val`.
uint8_t set_nz(uint8_t val) { return P = P & 125 | val & 128 | !val * 2; }

static int quit = 0;


//uint32_t frame_buffer[SCREEN_W * SCREEN_H];

struct Frame {
    int width;
    int height;
    uint32_t *pixels;
};

struct Frame frame = { SCREEN_W, SCREEN_H, frame_buffer };

VOID main(PVOID Buffer, ULONG Length) {
    
//Enable PS/2 Keyboard
	while ((__inbyte(0x64) & 2) != 0)
		;
	__outbyte(0x64, 0xae);

  DbgPrint("MainInit\n");
  VidResetDisplay(TRUE);
	VidSetTextColor(BV_COLOR_WHITE);
	VidSolidColorFill(0, 0, 639, 479, BV_COLOR_BLUE);


	VidSetScrollRegion(0, 0, 639, 479);
  VidDisplayString("Hello NES Emu in Bugcheck!\n");
  VidDisplayString("Hello NES Emu in Bugcheck!\n");
  // Start PRG0 after 16-byte header.
  rom = rombuf + 16;
  // PRG1 is the last bank. `rombuf[4]` is the number of 16k PRG banks.
  prg[1] = rombuf[4] - 1;
  // CHR0 ROM is after all PRG data in the file. `rombuf[5]` is the number of
  // 8k CHR banks. If it is zero, assume the game uses CHR RAM.
  chrrom = rombuf[5] ? rom + ((prg[1] + 1) << 14) : chrram;
  // CHR1 is the last 4k bank.
  chr[1] = (rombuf[5] || 1) * 2 - 1;
  // Bit 0 of `rombuf[6]` is 0=>horizontal mirroring, 1=>vertical mirroring.
  mirror = 3 - rombuf[6] % 2;
  if (rombuf[6] / 16 == 4) {
    mem(0, 128, 0, 1); // Update to default mmc3 banks
    prgbits--;         // 8kb PRG banks
    chrbits -= 2;      // 1kb CHR banks
  }

  // Start at address in reset vector, at $FFFC.
  _PCL = mem(~3, ~0, 0, 0);
  _PCH = mem(~2, ~0, 0, 0);

  while (!quit) {
    uint8_t opcodelo5;
    keychecker();
    //static MSG message = { 0 };
    //while(PeekMessage(&message, NULL, 0, 0, PM_REMOVE)) { DispatchMessage(&message); }
    //DbgPrint("Emu is running!");
    cycles = nomem = 0;
    if (nmi_irq)
      goto nmi_irq;

    opcode = read_pc();
    opcodelo5 = opcode & 31;
    switch (opcodelo5) {
      case 0:
        if (opcode & 0x80) { // LDY/CPY/CPX imm
          read_pc();
          nomem = 1;
          goto nomemop;
        }

        switch (opcode >> 5) {
        case 0: {
          uint16_t veclo; // BRK or nmi_irq
          !++_PCL && ++_PCH;
          nmi_irq:
            PUSH(_PCH);
          PUSH(_PCL);
          PUSH(P | 32);
          // BRK/IRQ vector is $ffff, NMI vector is $fffa
          veclo = ~1 - (nmi_irq & 4);
          _PCL = mem(veclo, ~0, 0, 0);
          _PCH = mem(veclo + 1, ~0, 0, 0);
          nmi_irq = 0;
          cycles++;
          break;
        }

        case 1: // JSR
            result = read_pc();
            PUSH(_PCH);
            PUSH(_PCL);
            _PCH = read_pc();
            _PCL = result;
            break;

        case 2: // RTI
            P = PULL & ~32;
            _PCL = PULL;
            _PCH = PULL;
            break;

        case 3: // RTS
            _PCL = PULL;
            _PCH = PULL;
            !++_PCL && ++_PCH;
            break;
        }

        cycles += 4;
        break;

      case 16: // BPL, BMI, BVC, BVS, BCC, BCS, BNE, BEQ
        read_pc();
        if (!(P & mask[opcode >> 6]) ^ opcode / 32 & 1) {
          cross = _PCL + (int8_t)val >> 8;
          _PCH += cross;
          _PCL += val;
          cycles += cross ? 2 : 1;
        }
        break;

      case 8: case 24:
        switch (opcode >>= 4) {
        case 0: // PHP
            PUSH(P | 48);
            cycles++;
            break;

        case 2: // PLP
            P = PULL & ~16;
            cycles += 2;
            break;

        case 4: // PHA
            PUSH(A);
            cycles++;
            break;

        case 6: // PLA
            set_nz(A = PULL);
            cycles += 2;
            break;

        case 8: // DEY
            set_nz(--Y);
            break;

        case 9: // TYA
            set_nz(A = Y);
            break;

        case 10: // TAY
            set_nz(Y = A);
            break;

        case 12: // INY
            set_nz(++Y);
            break;

        case 14: // INX
            set_nz(++X);
            break;

        default: // CLC, SEC, CLI, SEI, CLV, CLD, SED
            P = P & ~mask[opcode + 3] | mask[opcode + 4];
            break;
        }
        break;

      case 10: case 26:
        switch (opcode >> 4) {
        case 8: // TXA
            set_nz(A = X);
            break;

        case 9: // TXS
            S = X;
            break;

        case 10: // TAX
            set_nz(X = A);
            break;

        case 11: // TSX
            set_nz(X = S);
            break;

        case 12: // DEX
            set_nz(--X);
            break;

        case 14: // NOP
            break;

        default: // ASL/ROL/LSR/ROR A
            nomem = 1;
            val = A;
            goto nomemop;
        }
        break;

      case 1: // X-indexed, indirect
        read_pc();
        val += X;
        addr_lo = mem(val, 0, 0, 0);
        addr_hi = mem(val + 1, 0, 0, 0);
        cycles += 4;
        goto opcode;

      case 2: case 9: // Immediate
        read_pc();
        nomem = 1;
        goto nomemop;

      case 17: // Zeropage, Y-indexed
        addr_lo = mem(read_pc(), 0, 0, 0);
        addr_hi = mem(val + 1, 0, 0, 0);
        cycles++;
        goto add_x_or_y;

      case 4: case 5: case 6:     // Zeropage               +1
      case 20: case 21: case 22:  // Zeropage, X-indexed    +2
        addr_lo = read_pc();
        cross = opcodelo5 > 6;
        if (cross) {
          addr_lo += (opcode & 214) == 150 ? Y : X;  // LDX/STX use Y
        }
        addr_hi = 0;
        cycles -= !cross;
        goto opcode;

      case 12: case 13: case 14: // Absolute               +2
      case 25:                   // Absolute, Y-indexed    +2/3
      case 28: case 29: case 30: // Absolute, X-indexed    +2/3
        addr_lo = read_pc();
        addr_hi = read_pc();
        if (opcodelo5 < 25) goto opcode;
        add_x_or_y:
          val = opcodelo5 < 28 | opcode == 190 ? Y : X;
        cross = addr_lo + val > 255;
        addr_hi += cross;
        addr_lo += val;
        cycles +=
            ((opcode & 224) == 128 | opcode % 16 == 14 & opcode != 190) | cross;
        opcode:
          cycles += 2;
        if (opcode != 76 & (opcode & 224) != 128) {
          val = mem(addr_lo, addr_hi, 0, 0);
        }

        nomemop:
          result = 0;
        switch (opcode & 227) {
          case 1: set_nz(A |= val); break;  // ORA
          case 33: set_nz(A &= val); break; // AND
          case 65: set_nz(A ^= val); break; // EOR
          case 225: // SBC
            val = ~val;
            // fallthrough
          case 97: // ADC
            sum = A + val + P % 2;
            P = P & ~65 | sum > 255 | ((A ^ sum) & (val ^ sum) & 128) / 2;
            set_nz(A = sum);
            break;

          case 34: // ROL
            result = P & 1;
            // fallthrough
          case 2: // ASL
            result |= val * 2;
            P = P & ~1 | val / 128;
            goto memop;

          case 98: // ROR
            result = P << 7;
            // fallthrough
          case 66: // LSR
            result |= val / 2;
            P = P & ~1 | val & 1;
            goto memop;

          case 194: // DEC
            result = val - 1;
            goto memop;

          case 226: // INC
            result = val + 1;
            // fallthrough

            memop:
              set_nz(result);
            // Write result to A or back to memory.
            nomem ? A = result : (cycles += 2, mem(addr_lo, addr_hi, result, 1));
            break;

          case 32: // BIT
            P = P & 61 | val & 192 | !(A & val) * 2;
            break;

          case 64: // JMP
            _PCL = addr_lo;
            _PCH = addr_hi;
            cycles--;
            break;

          case 96: // JMP indirect
            _PCL = val;
            _PCH = mem(addr_lo + 1, addr_hi, 0, 0);
            cycles++;
            break;

          default: {
            uint8_t opcodehi3 = opcode / 32;
            uint8_t *reg = opcode % 4 == 2 | opcodehi3 == 7 ? &X
                           : opcode % 4 == 1                ? &A
                                                            : &Y;
            if (opcodehi3 == 4) {  // STY/STA/STX
              mem(addr_lo, addr_hi, *reg, 1);
            } else if (opcodehi3 != 5) {  // CPY/CMP/CPX
              P = P & ~1 | *reg >= val;
              set_nz(*reg - val);
            } else {  // LDY/LDA/LDX
              set_nz(*reg = val);
            }
            break;
          }
        }
    }

    // Update PPU, which runs 3 times faster than CPU. Each CPU instruction
    // takes at least 2 cycles.
    for (tmp = cycles * 3 + 6; tmp--;) {

      int temp;

      if (ppumask & 24) { // If background or sprites are enabled.
        if (scany < 240) {
          if (dot - 256 > 63u) {  // dot [0..255,320..340]
            // Draw a pixel to the framebuffer.
            if (dot < 256) {
              // Read color and palette from shift registers.
              uint8_t color = shift_hi >> 14 - fine_x & 2 |
                              shift_lo >> 15 - fine_x & 1,
                      palette = shift_at >> 28 - fine_x * 2 & 12;

              // If sprites are enabled.
              if (ppumask & 16) {
                uint8_t *sprite;
                // Loop through all sprites.
                for (sprite = oam; sprite < oam + 256; sprite += 4) {
                  uint16_t sprite_h = ppuctrl & 32 ? 16 : 8,
                           sprite_x = dot - sprite[3],
                           sprite_y = scany - sprite[0] - 1,
                           sx = sprite_x ^ !(sprite[2] & 64) * 7,
                           sy = sprite_y ^ (sprite[2] & 128 ? sprite_h - 1 : 0);
                  if (sprite_x < 8 && sprite_y < sprite_h) {
                    uint16_t sprite_tile = sprite[1],
                             sprite_addr =
                                 (ppuctrl & 32
                                      // 8x16 sprites
                                      ? sprite_tile % 2 << 12 |
                                            sprite_tile << 4 & -32 | sy * 2 & 16
                                      // 8x8 sprites
                                      : (ppuctrl & 8) << 9 | sprite_tile << 4) |
                                 sy & 7,
                             sprite_color =
                                 *get_chr_byte(sprite_addr + 8) >> sx << 1 & 2 |
                                 *get_chr_byte(sprite_addr) >> sx & 1;
                    // Only draw sprite if color is not 0 (transparent)
                    if (sprite_color) {
                      // Don't draw sprite if BG has priority.
                      if (!(sprite[2] & 32 && color)) {
                        color = sprite_color;
                        palette = 16 | sprite[2] * 4 & 12;
                      }
                      // Maybe set sprite0 hit flag.
                      if (sprite == oam && color)
                        ppustatus |= 64;
                      break;
                    }
                  }
                }
              }

              // Write pixel to framebuffer. Always use palette 0 for color 0.
              // BGR565 palette is used instead of RGBA32 to reduce source code
              // size.
              frame_buffer[scany * 256 + dot] =
    nes_palette[ palette_ram[color ? (palette | color) : 0] ];
            }

            // Update shift registers every cycle.
            if (dot < 336) {
              shift_hi *= 2;
              shift_lo *= 2;
              shift_at *= 4;
            }

            temp = ppuctrl << 8 & 4096 | ntb << 4 | V >> 12;
            switch (dot & 7) {
              case 1: // Read nametable byte.
                ntb = *get_nametable_byte(V);
                break;
              case 3: // Read attribute byte.
                atb = (*get_nametable_byte(V & 0xc00 | 0x3c0 | V >> 4 & 0x38 |
                                           V / 4 & 7) >>
                       (V >> 5 & 2 | V / 2 & 1) * 2) %
                      4 * 0x5555;
                break;
              case 5: // Read pattern table low byte.
                ptb_lo = *get_chr_byte(temp);
                break;
              case 7: { // Read pattern table high byte.
                uint8_t ptb_hi = *get_chr_byte(temp | 8);
                // Increment horizontal VRAM read address.
                V = V % 32 == 31 ? V & ~31 ^ 1024 : V + 1;
                shift_hi |= ptb_hi;
                shift_lo |= ptb_lo;
                shift_at |= atb;
                break;
              }
            }
          }

          // Increment vertical VRAM address.
          if (dot == 256) {
            V = ((V & 7 << 12) != 7 << 12 ? V + 4096
                 : (V & 0x3e0) == 928     ? V & 0x8c1f ^ 2048
                 : (V & 0x3e0) == 0x3e0   ? V & 0x8c1f
                                          : V & 0x8c1f | V + 32 & 0x3e0) &
                    // Reset horizontal VRAM address to T value.
                    ~0x41f |
                T & 0x41f;
          }
        }

        // Check for MMC3 IRQ.
        if ((scany + 1) % 262 < 241 && dot == 261 && mmc3_irq && !mmc3_latch--)
          nmi_irq = 1;

        // Reset vertical VRAM address to T value.
        if (scany == 261 && dot - 280 < 25u)  // dot [280..304]
          V = V & 0x841f | T & 0x7be0;
      }

      if (dot == 1) {
        int i;
        if (scany == 241) {
          // If NMI is enabled, trigger NMI.
          if (ppuctrl & 128)
            nmi_irq = 4;
          ppustatus |= 128;
          // Render frame, skipping the top and bottom 8 pixels (they're often
          // garbage).
          // display video
          for (i = 0; i < NES_WIDTH * NES_HEIGHT; i++) {
            // uint16_t c16 = (frame_buffer + 2048)[i];
            uint16_t c16 = (frame_buffer)[i];
            uint8_t r5 = ((((c16 >> 11) & 0x1F) * 527) + 23) >> 6;
            uint8_t g6 = ((((c16 >> 5) & 0x3F) * 259) + 33) >> 6;
            uint8_t b5 = (((c16 & 0x1F) * 527) + 23) >> 6;
            uint32_t c32 = r5 << 16 | g6 << 8 | b5;
            uint8_t b = (uint8_t)(c32 >> 16U);
            uint8_t g = (uint8_t)(c32 >> 8U);
            uint8_t r = (uint8_t)(c32);
            frame.pixels[i] = (r << 16U) | (g << 8U) | b;
          }
          viddraw();
          // InvalidateRect(window_handle, NULL, FALSE);   //DrawHere
          // UpdateWindow(window_handle);
        }

        // Clear ppustatus.
        if (scany == 261)
          ppustatus = 0;
      }

      // Increment to next dot/scany. 341 dots per scanline, 262 scanlines per
      // frame. Scanline 261 is represented as -1.
      if (++dot == 341) {
        dot = 0;
        scany++;
        scany %= 262;
      }
    }
  }
}

NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject,
	PUNICODE_STRING RegistryPath) {
BOOLEAN Registered;
    RtlZeroMemory(ram, sizeof(ram));
    RtlZeroMemory(vram, sizeof(vram));
    RtlZeroMemory(oam, sizeof(oam));
    RtlZeroMemory(prgram, sizeof(prgram));
    RtlZeroMemory(chrram, sizeof(chrram));
    RtlZeroMemory(palette_ram, sizeof(palette_ram));
    RtlZeroMemory(frame_buffer, sizeof(frame_buffer));

BugcheckCallbackRecord =
    ExAllocatePoolWithTag(NonPagedPool,
                          sizeof(KBUGCHECK_CALLBACK_RECORD),
                          'CSIR');

if (BugcheckCallbackRecord)
{
    RtlZeroMemory(BugcheckCallbackRecord,
                  sizeof(KBUGCHECK_CALLBACK_RECORD));
}
	if (BugcheckCallbackRecord == NULL) return STATUS_INSUFFICIENT_RESOURCES;

	KeInitializeCallbackRecord(BugcheckCallbackRecord);

	Registered = KeRegisterBugCheckCallback(
                    BugcheckCallbackRecord,
                    main,
                    NULL,
                    0,
                    (PUCHAR)"Loonix");
	if (!Registered) {
		ExFreePoolWithTag(BugcheckCallbackRecord, 'BCHK');

		return STATUS_UNSUCCESSFUL;
	}

	return STATUS_SUCCESS;
}

int rgb_to_bv16(uint32_t color) {
    uint8_t r = (color >> 16) & 0xFF;
    uint8_t g = (color >> 8) & 0xFF;
    uint8_t b = color & 0xFF;

    int best_idx = 0;
    int min_dist = 0x7FFFFFFF;
    int i;
    for (i = 0; i < 16; i++) {
        int dr = r - bv_palette[i][0];
        int dg = g - bv_palette[i][1];
        int db = b - bv_palette[i][2];
        int dist = dr * dr + dg * dg + db * db;

        if (dist < min_dist) {
            min_dist = dist;
            best_idx = i;
        }
    }

    return best_idx;
}

void viddraw()
{int x,y;
 for (y=0;y<239;y++)
 {
  for (x=0;x<256;x++)
  {

    VidSolidColorFill(192+x, 120+y, 192+x+1 , 120+y, rgb_to_bv16(frame_buffer[y*256+x]));
  }
 }
}

void keychecker()
{int key = mini_rv32ima_handle_other_csr_read();
  if (key != 0)
  {
    switch(key) {
        case 122:       controller1 |= (1 << 0); break;
        case 120:       controller1 |= (1 << 1); break;
        case 116:   controller1 |= (1 << 2); break;
        case 13: controller1 |= (1 << 3); break;
        case 119:     controller1 |= (1 << 4); break;
        case 115:   controller1 |= (1 << 5); break;
        case 97:   controller1 |= (1 << 6); break;
        case 100:  controller1 |= (1 << 7); break;
        case 113:  quit=1; break;
    }
  }
else
{
  
        controller1 &= ~(1 << 0);
        controller1 &= ~(1 << 2);
        controller1 &= ~(1 << 3);
        controller1 &= ~(1 << 4);
        controller1 &= ~(1 << 5);
        controller1 &= ~(1 << 6);
        controller1 &= ~(1 << 7);
}

}