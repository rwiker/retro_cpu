// Native 8A/8XY
#define OP_A uint8_t
#define OP_XY uint8_t
{
#include "cpu_65c816_ops_native.inl"
},
#undef OP_XY
#undef OP_A
// Native 16A/8XY
#define OP_A uint16_t
#define OP_XY uint8_t
{
#include "cpu_65c816_ops_native.inl"
},
#undef OP_XY
#undef OP_A
// Native 8A/16XY
#define OP_A uint8_t
#define OP_XY uint16_t
{
#include "cpu_65c816_ops_native.inl"
},
#undef OP_XY
#undef OP_A
// Native 16A/16XY
#define OP_A uint16_t
#define OP_XY uint16_t
{
#include "cpu_65c816_ops_native.inl"
},
#undef OP_XY
#undef OP_A
// Emulation mode
// Wrapping rules:
// Stack wraps to be 0x0001XX
// Iff DL == 0, direct page accesses wrap at 0x00DDXX
// Direct, direct,x, direct,y, (direct,x), (direct),y are affected.
// Instruction must be "old" - extant on a 6502, so something like PEI is not affected
#define OP_A uint8_t
#define OP_XY uint8_t
{
// 00
OP(BRK) OP(OpOr<AddrDirectIndirectX<OP_A, OP_XY, true>>) OP(COP) OP(OpOr<AddrStack<OP_A, OP_XY>>)
OP(OpTsb<AddrDirect<OP_A, OP_XY, true>>) OP(OpOr<AddrDirect<OP_A, OP_XY, true>>) OP(OpAsl<AddrDirect<OP_A, OP_XY, true>>) OP(OpOr<AddrDirectIndirectLong<OP_A, OP_XY>>)
// 08
OP(PHP) OP(OpOr<AddrImm<OP_A, OP_XY, OP_A>>) OP(OpAsl<AddrA<OP_A, OP_XY>>) OP(PHD)
OP(OpTsb<AddrAbs<OP_A, OP_XY>>) OP(OpOr<AddrAbs<OP_A, OP_XY>>) OP(OpAsl<AddrAbs<OP_A, OP_XY>>) OP(OpOr<AddrAbsLong<OP_A, OP_XY>>)
// 10
OP(BPL) OP(OpOr<AddrDirectIndirectY<OP_A, OP_XY, true, true>>) OP(OpOr<AddrDirectIndirect<OP_A, OP_XY>>) OP(OpOr<AddrStackY<OP_A, OP_XY>>)
OP(OpTrb<AddrDirect<OP_A, OP_XY, true>>) OP(OpOr<AddrDirectX<OP_A, OP_XY, true>>) OP(OpAsl<AddrDirectX<OP_A, OP_XY, true>>) OP(OpOr<AddrDirectIndirectLongY<OP_A, OP_XY>>)
// 18
OP(CLC) OP(OpOr<AddrAbsY<OP_A, OP_XY>>) OP(INA<OP_A, OP_XY>) OP(TCS<true>)
OP(OpTrb<AddrAbs<OP_A, OP_XY>>) OP(OpOr<AddrAbsX<OP_A, OP_XY>>) OP(OpAsl<AddrAbsX<OP_A, OP_XY, false>>) OP(OpOr<AddrAbsLongX<OP_A, OP_XY>>)
// 20
OP(JSR_ABS) OP(OpAnd<AddrDirectIndirectX<OP_A, OP_XY, true>>) OP(JSR_LONG) OP(OpAnd<AddrStack<OP_A, OP_XY>>)
OP(OpBit<AddrDirect<OP_A, OP_XY, true>>) OP(OpAnd<AddrDirect<OP_A, OP_XY, true>>) OP(OpRol<AddrDirect<OP_A, OP_XY, true>>) OP(OpAnd<AddrDirectIndirectLong<OP_A, OP_XY>>)
// 28
OP(PLP) OP(OpAnd<AddrImm<OP_A, OP_XY, OP_A>>) OP(OpRol<AddrA<OP_A, OP_XY>>) OP(PLD)
OP(OpBit<AddrAbs<OP_A, OP_XY>>) OP(OpAnd<AddrAbs<OP_A, OP_XY>>) OP(OpRol<AddrAbs<OP_A, OP_XY>>) OP(OpAnd<AddrAbsLong<OP_A, OP_XY>>)
// 30
OP(BMI) OP(OpAnd<AddrDirectIndirectY<OP_A, OP_XY, true, true>>) OP(OpAnd<AddrDirectIndirect<OP_A, OP_XY>>) OP(OpAnd<AddrStackY<OP_A, OP_XY>>)
OP(OpBit<AddrDirectX<OP_A, OP_XY, true>>) OP(OpAnd<AddrDirectX<OP_A, OP_XY, true>>) OP(OpRol<AddrDirectX<OP_A, OP_XY, true>>) OP(OpAnd<AddrDirectIndirectLongY<OP_A, OP_XY>>)
// 38
OP(SEC) OP(OpAnd<AddrAbsY<OP_A, OP_XY>>) OP(DEA<OP_A, OP_XY>) OP(TSC<OP_A, OP_XY>)
OP(OpBit<AddrAbsX<OP_A, OP_XY>>) OP(OpAnd<AddrAbsX<OP_A, OP_XY>>) OP(OpRol<AddrAbsX<OP_A, OP_XY, false>>) OP(OpAnd<AddrAbsLongX<OP_A, OP_XY>>)
// 40
OP(RTI) OP(OpXor<AddrDirectIndirectX<OP_A, OP_XY, true>>) OP(OpWDM) OP(OpXor<AddrStack<OP_A, OP_XY>>)
OP(MVP) OP(OpXor<AddrDirect<OP_A, OP_XY, true>>) OP(OpLsr<AddrDirect<OP_A, OP_XY, true>>) OP(OpXor<AddrDirectIndirectLong<OP_A, OP_XY>>)
// 48
OP(PHA<OP_A>) OP(OpXor<AddrImm<OP_A, OP_XY, OP_A>>) OP(OpLsr<AddrA<OP_A, OP_XY>>) OP(PHK)
OP(JMP_ABS) OP(OpXor<AddrAbs<OP_A, OP_XY>>) OP(OpLsr<AddrAbs<OP_A, OP_XY>>) OP(OpXor<AddrAbsLong<OP_A, OP_XY>>)
// 50
OP(BVC) OP(OpXor<AddrDirectIndirectY<OP_A, OP_XY, true, true>>) OP(OpXor<AddrDirectIndirect<OP_A, OP_XY>>) OP(OpXor<AddrStackY<OP_A, OP_XY>>)
OP(MVN) OP(OpXor<AddrDirectX<OP_A, OP_XY, true>>) OP(OpLsr<AddrDirectX<OP_A, OP_XY, true>>) OP(OpXor<AddrDirectIndirectLongY<OP_A, OP_XY>>)
// 58
OP(CLI) OP(OpXor<AddrAbsY<OP_A, OP_XY>>) OP(PHY<OP_XY>) OP(TCD<OP_A, OP_XY>)
OP(JMP_LONG) OP(OpXor<AddrAbsX<OP_A, OP_XY>>) OP(OpLsr<AddrAbsX<OP_A, OP_XY, false>>) OP(OpXor<AddrAbsLongX<OP_A, OP_XY>>)
// 60
OP(RTS) OP(OpAdc<AddrDirectIndirectX<OP_A, OP_XY, true>>) OP(PER) OP(OpAdc<AddrStack<OP_A, OP_XY>>)
OP(STZ<AddrDirect<OP_A, OP_XY, true>>) OP(OpAdc<AddrDirect<OP_A, OP_XY, true>>) OP(OpRor<AddrDirect<OP_A, OP_XY, true>>) OP(OpAdc<AddrDirectIndirectLong<OP_A, OP_XY>>)
// 68
OP(PLA<OP_A>) OP(OpAdc<AddrImm<OP_A, OP_XY, OP_A>>) OP(OpRor<AddrA<OP_A, OP_XY>>) OP(RTL)
OP(JMP_INDABS) OP(OpAdc<AddrAbs<OP_A, OP_XY>>) OP(OpRor<AddrAbs<OP_A, OP_XY>>) OP(OpAdc<AddrAbsLong<OP_A, OP_XY>>)
// 70
OP(BVS) OP(OpAdc<AddrDirectIndirectY<OP_A, OP_XY, true, true>>) OP(OpAdc<AddrDirectIndirect<OP_A, OP_XY>>) OP(OpAdc<AddrStackY<OP_A, OP_XY>>)
OP(STZ<AddrDirectX<OP_A, OP_XY, true>>) OP(OpAdc<AddrDirectX<OP_A, OP_XY, true>>) OP(OpRor<AddrDirectX<OP_A, OP_XY, true>>) OP(OpAdc<AddrDirectIndirectLongY<OP_A, OP_XY>>)
// 78
OP(SEI) OP(OpAdc<AddrAbsY<OP_A, OP_XY>>) OP(PLY<OP_XY>) OP(TDC<OP_A, OP_XY>)
OP(JMP_INDABSX) OP(OpAdc<AddrAbsX<OP_A, OP_XY>>) OP(OpRor<AddrAbsX<OP_A, OP_XY, false>>) OP(OpAdc<AddrAbsLongX<OP_A, OP_XY>>)
// 80
OP(BRA) OP(STA<AddrDirectIndirectX<OP_A, OP_XY, true>>) OP(BRL) OP(STA<AddrStack<OP_A, OP_XY>>)
OP(STY<AddrDirect<OP_A, OP_XY, true>>) OP(STA<AddrDirect<OP_A, OP_XY, true>>) OP(STX<AddrDirect<OP_A, OP_XY, true>>) OP(STA<AddrDirectIndirectLong<OP_A, OP_XY>>)
// 88
OP(DEY<OP_A, OP_XY>) OP(OpBitImm<AddrImm<OP_A, OP_XY, OP_A>>) OP(TXA<OP_A, OP_XY>) OP(PHB)
OP(STY<AddrAbs<OP_A, OP_XY>>) OP(STA<AddrAbs<OP_A, OP_XY>>) OP(STX<AddrAbs<OP_A, OP_XY>>) OP(STA<AddrAbsLong<OP_A, OP_XY>>)
// 90
OP(BCC) OP(STA<AddrDirectIndirectY<OP_A, OP_XY, false, true>>) OP(STA<AddrDirectIndirect<OP_A, OP_XY>>) OP(STA<AddrStackY<OP_A, OP_XY>>)
OP(STY<AddrDirectX<OP_A, OP_XY, true>>) OP(STA<AddrDirectX<OP_A, OP_XY, true>>) OP(STX<AddrDirectY<OP_A, OP_XY, true>>) OP(STA<AddrDirectIndirectLongY<OP_A, OP_XY>>)
// 98
OP(TYA<OP_A, OP_XY>) OP(STA<AddrAbsY<OP_A, OP_XY, false>>) OP(TXS<true>) OP(TXY<OP_A, OP_XY>)
OP(STZ<AddrAbs<OP_A, OP_XY>>) OP(STA<AddrAbsX<OP_A, OP_XY, false>>) OP(STZ<AddrAbsX<OP_A, OP_XY, false>>) OP(STA<AddrAbsLongX<OP_A, OP_XY>>)
// A0
OP(LDY<AddrImm<OP_A, OP_XY, OP_XY>>) OP(LDA<AddrDirectIndirectX<OP_A, OP_XY, true>>) OP(LDX<AddrImm<OP_A, OP_XY, OP_XY>>) OP(LDA<AddrStack<OP_A, OP_XY>>)
OP(LDY<AddrDirect<OP_A, OP_XY, true>>) OP(LDA<AddrDirect<OP_A, OP_XY, true>>) OP(LDX<AddrDirect<OP_A, OP_XY, true>>) OP(LDA<AddrDirectIndirectLong<OP_A, OP_XY>>)
// A8
OP(TAY<OP_A, OP_XY>) OP(LDA<AddrImm<OP_A, OP_XY, OP_A>>) OP(TAX<OP_A, OP_XY>) OP(PLB)
OP(LDY<AddrAbs<OP_A, OP_XY>>) OP(LDA<AddrAbs<OP_A, OP_XY>>) OP(LDX<AddrAbs<OP_A, OP_XY>>) OP(LDA<AddrAbsLong<OP_A, OP_XY>>)
// B0
OP(BCS) OP(LDA<AddrDirectIndirectY<OP_A, OP_XY, true, true>>) OP(LDA<AddrDirectIndirect<OP_A, OP_XY>>) OP(LDA<AddrStackY<OP_A, OP_XY>>)
OP(LDY<AddrDirectX<OP_A, OP_XY, true>>) OP(LDA<AddrDirectX<OP_A, OP_XY, true>>) OP(LDX<AddrDirectY<OP_A, OP_XY, true>>) OP(LDA<AddrDirectIndirectLongY<OP_A, OP_XY>>)
// B8
OP(CLV) OP(LDA<AddrAbsY<OP_A, OP_XY>>) OP(TSX<OP_A, OP_XY>) OP(TYX<OP_A, OP_XY>)
OP(LDY<AddrAbsX<OP_A, OP_XY>>) OP(LDA<AddrAbsX<OP_A, OP_XY>>) OP(LDX<AddrAbsY<OP_A, OP_XY>>) OP(LDA<AddrAbsLongX<OP_A, OP_XY>>)
// C0
OP(OpCpy<AddrImm<OP_A, OP_XY, OP_XY>>) OP(OpCmp<AddrDirectIndirectX<OP_A, OP_XY, true>>) OP(REP) OP(OpCmp<AddrStack<OP_A, OP_XY>>)
OP(OpCpy<AddrDirect<OP_A, OP_XY, true>>) OP(OpCmp<AddrDirect<OP_A, OP_XY, true>>) OP(OpDecMem<AddrDirect<OP_A, OP_XY, true>>) OP(OpCmp<AddrDirectIndirectLong<OP_A, OP_XY>>)
// C8
OP(INY<OP_A, OP_XY>) OP(OpCmp<AddrImm<OP_A, OP_XY, OP_A>>) OP(DEX<OP_A, OP_XY>) OP(WAI)
OP(OpCpy<AddrAbs<OP_A, OP_XY>>) OP(OpCmp<AddrAbs<OP_A, OP_XY>>) OP(OpDecMem<AddrAbs<OP_A, OP_XY>>) OP(OpCmp<AddrAbsLong<OP_A, OP_XY>>)
// D0
OP(BNE) OP(OpCmp<AddrDirectIndirectY<OP_A, OP_XY, true, true>>) OP(OpCmp<AddrDirectIndirect<OP_A, OP_XY>>) OP(OpCmp<AddrStackY<OP_A, OP_XY>>)
OP(PEI) OP(OpCmp<AddrDirectX<OP_A, OP_XY, true>>) OP(OpDecMem<AddrDirectX<OP_A, OP_XY, true>>) OP(OpCmp<AddrDirectIndirectLongY<OP_A, OP_XY>>)
// D8
OP(CLD) OP(OpCmp<AddrAbsY<OP_A, OP_XY>>) OP(PHX<OP_XY>) OP(STP)
OP(JMP_INDLONG) OP(OpCmp<AddrAbsX<OP_A, OP_XY>>) OP(OpDecMem<AddrAbsX<OP_A, OP_XY, false>>) OP(OpCmp<AddrAbsLongX<OP_A, OP_XY>>)
// E0
OP(OpCpx<AddrImm<OP_A, OP_XY, OP_XY>>) OP(OpSbc<AddrDirectIndirectX<OP_A, OP_XY, true>>) OP(SEP) OP(OpSbc<AddrStack<OP_A, OP_XY>>)
OP(OpCpx<AddrDirect<OP_A, OP_XY, true>>) OP(OpSbc<AddrDirect<OP_A, OP_XY, true>>) OP(OpIncMem<AddrDirect<OP_A, OP_XY, true>>) OP(OpSbc<AddrDirectIndirectLong<OP_A, OP_XY>>)
// E8
OP(INX<OP_A, OP_XY>) OP(OpSbc<AddrImm<OP_A, OP_XY, OP_A>>) OP(OpNOP) OP(XBA)
OP(OpCpx<AddrAbs<OP_A, OP_XY>>) OP(OpSbc<AddrAbs<OP_A, OP_XY>>) OP(OpIncMem<AddrAbs<OP_A, OP_XY>>) OP(OpSbc<AddrAbsLong<OP_A, OP_XY>>)
// F0
OP(BEQ) OP(OpSbc<AddrDirectIndirectY<OP_A, OP_XY, true, true>>) OP(OpSbc<AddrDirectIndirect<OP_A, OP_XY>>) OP(OpSbc<AddrStackY<OP_A, OP_XY>>)
OP(PEA) OP(OpSbc<AddrDirectX<OP_A, OP_XY, true>>) OP(OpIncMem<AddrDirectX<OP_A, OP_XY, true>>) OP(OpSbc<AddrDirectIndirectLongY<OP_A, OP_XY>>)
// F8
OP(SED) OP(OpSbc<AddrAbsY<OP_A, OP_XY>>) OP(PLX<OP_XY>) OP(XCE)
OP(JSR_INDABSX) OP(OpSbc<AddrAbsX<OP_A, OP_XY>>) OP(OpIncMem<AddrAbsX<OP_A, OP_XY, false>>) OP(OpSbc<AddrAbsLongX<OP_A, OP_XY>>)
},
// 6502 Native
#define NYI() 0,
{
// 00
OP(BRK) OP(OpOr<AddrDirectIndirectX<OP_A, OP_XY, true>>) NYI() NYI()
NYI() OP(OpOr<AddrDirect<OP_A, OP_XY, true>>) OP(OpAsl<AddrDirect<OP_A, OP_XY, true>>) NYI()
// 08
OP(PHP) OP(OpOr<AddrImm<OP_A, OP_XY, OP_A>>) OP(OpAsl<AddrA<OP_A, OP_XY>>) NYI()
NYI() OP(OpOr<AddrAbs<OP_A, OP_XY>>) OP(OpAsl<AddrAbs<OP_A, OP_XY>>) NYI()
// 10
OP(BPL) OP(OpOr<AddrDirectIndirectY<OP_A, OP_XY, true, true>>) NYI() NYI()
NYI() OP(OpOr<AddrDirectX<OP_A, OP_XY, true>>) OP(OpAsl<AddrDirectX<OP_A, OP_XY, true>>) NYI()
// 18
OP(CLC) OP(OpOr<AddrAbsY<OP_A, OP_XY>>) NYI() NYI()
NYI() OP(OpOr<AddrAbsX<OP_A, OP_XY>>) OP(OpAsl<AddrAbsX<OP_A, OP_XY, false>>) NYI()
// 20
OP(JSR_ABS) OP(OpAnd<AddrDirectIndirectX<OP_A, OP_XY, true>>) NYI() NYI()
OP(OpBit<AddrDirect<OP_A, OP_XY, true>>) OP(OpAnd<AddrDirect<OP_A, OP_XY, true>>) OP(OpRol<AddrDirect<OP_A, OP_XY, true>>) NYI()
// 28
OP(PLP) OP(OpAnd<AddrImm<OP_A, OP_XY, OP_A>>) OP(OpRol<AddrA<OP_A, OP_XY>>) NYI()
OP(OpBit<AddrAbs<OP_A, OP_XY>>) OP(OpAnd<AddrAbs<OP_A, OP_XY>>) OP(OpRol<AddrAbs<OP_A, OP_XY>>) NYI()
// 30
OP(BMI) OP(OpAnd<AddrDirectIndirectY<OP_A, OP_XY, true, true>>) NYI() NYI()
OP(OpBit<AddrDirectX<OP_A, OP_XY, true>>) OP(OpAnd<AddrDirectX<OP_A, OP_XY, true>>) OP(OpRol<AddrDirectX<OP_A, OP_XY, true>>) NYI()
// 38
OP(SEC) OP(OpAnd<AddrAbsY<OP_A, OP_XY>>) NYI() NYI()
NYI() OP(OpAnd<AddrAbsX<OP_A, OP_XY>>) OP(OpRol<AddrAbsX<OP_A, OP_XY, false>>) NYI()
// 40
OP(RTI) OP(OpXor<AddrDirectIndirectX<OP_A, OP_XY, true>>) NYI() NYI()
NYI() OP(OpXor<AddrDirect<OP_A, OP_XY, true>>) OP(OpLsr<AddrDirect<OP_A, OP_XY, true>>) NYI()
// 48
OP(PHA<OP_A>) OP(OpXor<AddrImm<OP_A, OP_XY, OP_A>>) OP(OpLsr<AddrA<OP_A, OP_XY>>) NYI()
OP(JMP_ABS) OP(OpXor<AddrAbs<OP_A, OP_XY>>) OP(OpLsr<AddrAbs<OP_A, OP_XY>>) NYI()
// 50
OP(BVC) OP(OpXor<AddrDirectIndirectY<OP_A, OP_XY, true, true>>) NYI() NYI()
NYI() OP(OpXor<AddrDirectX<OP_A, OP_XY, true>>) OP(OpLsr<AddrDirectX<OP_A, OP_XY, true>>) NYI()
// 58
OP(CLI) OP(OpXor<AddrAbsY<OP_A, OP_XY>>) NYI() NYI()
NYI() OP(OpXor<AddrAbsX<OP_A, OP_XY>>) OP(OpLsr<AddrAbsX<OP_A, OP_XY, false>>) NYI()
// 60
OP(RTS) OP(OpAdc<AddrDirectIndirectX<OP_A, OP_XY, true>>) NYI() NYI()
NYI() OP(OpAdc<AddrDirect<OP_A, OP_XY, true>>) OP(OpRor<AddrDirect<OP_A, OP_XY, true>>) NYI()
// 68
OP(PLA<OP_A>) OP(OpAdc<AddrImm<OP_A, OP_XY, OP_A>>) OP(OpRor<AddrA<OP_A, OP_XY>>) NYI()
OP(JMP_INDABS_LEGACY) OP(OpAdc<AddrAbs<OP_A, OP_XY>>) OP(OpRor<AddrAbs<OP_A, OP_XY>>) NYI()
// 70
OP(BVS) OP(OpAdc<AddrDirectIndirectY<OP_A, OP_XY, true, true>>) NYI() NYI()
NYI() OP(OpAdc<AddrDirectX<OP_A, OP_XY, true>>) OP(OpRor<AddrDirectX<OP_A, OP_XY, true>>) NYI()
// 78
OP(SEI) OP(OpAdc<AddrAbsY<OP_A, OP_XY>>) NYI() NYI()
NYI() OP(OpAdc<AddrAbsX<OP_A, OP_XY>>) OP(OpRor<AddrAbsX<OP_A, OP_XY, false>>) NYI()
// 80
NYI() OP(STA<AddrDirectIndirectX<OP_A, OP_XY, true>>) NYI() NYI()
OP(STY<AddrDirect<OP_A, OP_XY, true>>) OP(STA<AddrDirect<OP_A, OP_XY, true>>) OP(STX<AddrDirect<OP_A, OP_XY, true>>) NYI()
// 88
OP(DEY<OP_A, OP_XY>) OP(OpBitImm<AddrImm<OP_A, OP_XY, OP_A>>) OP(TXA<OP_A, OP_XY>) OP(PHB)
OP(STY<AddrAbs<OP_A, OP_XY>>) OP(STA<AddrAbs<OP_A, OP_XY>>) OP(STX<AddrAbs<OP_A, OP_XY>>) OP(STA<AddrAbsLong<OP_A, OP_XY>>)
// 90
OP(BCC) OP(STA<AddrDirectIndirectY<OP_A, OP_XY, false, true>>) NYI() NYI()
OP(STY<AddrDirectX<OP_A, OP_XY, true>>) OP(STA<AddrDirectX<OP_A, OP_XY, true>>) OP(STX<AddrDirectY<OP_A, OP_XY, true>>) NYI()
// 98
OP(TYA<OP_A, OP_XY>) OP(STA<AddrAbsY<OP_A, OP_XY, false>>) OP(TXS<true>) NYI()
NYI() OP(STA<AddrAbsX<OP_A, OP_XY, false>>) NYI() NYI()
// A0
OP(LDY<AddrImm<OP_A, OP_XY, OP_XY>>) OP(LDA<AddrDirectIndirectX<OP_A, OP_XY, true>>) OP(LDX<AddrImm<OP_A, OP_XY, OP_XY>>) NYI()
OP(LDY<AddrDirect<OP_A, OP_XY, true>>) OP(LDA<AddrDirect<OP_A, OP_XY, true>>) OP(LDX<AddrDirect<OP_A, OP_XY, true>>) NYI()
// A8
OP(TAY<OP_A, OP_XY>) OP(LDA<AddrImm<OP_A, OP_XY, OP_A>>) OP(TAX<OP_A, OP_XY>) NYI()
OP(LDY<AddrAbs<OP_A, OP_XY>>) OP(LDA<AddrAbs<OP_A, OP_XY>>) OP(LDX<AddrAbs<OP_A, OP_XY>>) NYI()
// B0
OP(BCS) OP(LDA<AddrDirectIndirectY<OP_A, OP_XY, true, true>>) NYI() NYI()
OP(LDY<AddrDirectX<OP_A, OP_XY, true>>) OP(LDA<AddrDirectX<OP_A, OP_XY, true>>) OP(LDX<AddrDirectY<OP_A, OP_XY, true>>) NYI()
// B8
OP(CLV) OP(LDA<AddrAbsY<OP_A, OP_XY>>) OP(TSX<OP_A, OP_XY>) NYI()
OP(LDY<AddrAbsX<OP_A, OP_XY>>) OP(LDA<AddrAbsX<OP_A, OP_XY>>) OP(LDX<AddrAbsY<OP_A, OP_XY>>) NYI()
// C0
OP(OpCpy<AddrImm<OP_A, OP_XY, OP_XY>>) OP(OpCmp<AddrDirectIndirectX<OP_A, OP_XY, true>>) NYI() NYI()
OP(OpCpy<AddrDirect<OP_A, OP_XY, true>>) OP(OpCmp<AddrDirect<OP_A, OP_XY, true>>) OP(OpDecMem<AddrDirect<OP_A, OP_XY, true>>) NYI()
// C8
OP(INY<OP_A, OP_XY>) OP(OpCmp<AddrImm<OP_A, OP_XY, OP_A>>) OP(DEX<OP_A, OP_XY>) NYI()
OP(OpCpy<AddrAbs<OP_A, OP_XY>>) OP(OpCmp<AddrAbs<OP_A, OP_XY>>) OP(OpDecMem<AddrAbs<OP_A, OP_XY>>) NYI()
// D0
OP(BNE) OP(OpCmp<AddrDirectIndirectY<OP_A, OP_XY, true, true>>) NYI() NYI()
NYI() OP(OpCmp<AddrDirectX<OP_A, OP_XY, true>>) OP(OpDecMem<AddrDirectX<OP_A, OP_XY, true>>) NYI()
// D8
OP(CLD) OP(OpCmp<AddrAbsY<OP_A, OP_XY>>) NYI() NYI()
NYI() OP(OpCmp<AddrAbsX<OP_A, OP_XY>>) OP(OpDecMem<AddrAbsX<OP_A, OP_XY, false>>) NYI()
// E0
OP(OpCpx<AddrImm<OP_A, OP_XY, OP_XY>>) OP(OpSbc<AddrDirectIndirectX<OP_A, OP_XY, true>>) NYI() NYI()
OP(OpCpx<AddrDirect<OP_A, OP_XY, true>>) OP(OpSbc<AddrDirect<OP_A, OP_XY, true>>) OP(OpIncMem<AddrDirect<OP_A, OP_XY, true>>) NYI()
// E8
OP(INX<OP_A, OP_XY>) OP(OpSbc<AddrImm<OP_A, OP_XY, OP_A>>) OP(OpNOP) NYI()
OP(OpCpx<AddrAbs<OP_A, OP_XY>>) OP(OpSbc<AddrAbs<OP_A, OP_XY>>) OP(OpIncMem<AddrAbs<OP_A, OP_XY>>) NYI()
// F0
OP(BEQ) OP(OpSbc<AddrDirectIndirectY<OP_A, OP_XY, true, true>>) NYI() NYI()
NYI() OP(OpSbc<AddrDirectX<OP_A, OP_XY, true>>) OP(OpIncMem<AddrDirectX<OP_A, OP_XY, true>>) NYI()
// F8
OP(SED) OP(OpSbc<AddrAbsY<OP_A, OP_XY>>) NYI() NYI()
NYI() OP(OpSbc<AddrAbsX<OP_A, OP_XY>>) OP(OpIncMem<AddrAbsX<OP_A, OP_XY, false>>) NYI()
},
#undef OP_XY
#undef OP_A

#undef NYI
