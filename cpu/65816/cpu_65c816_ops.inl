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
#define OP_A uint8_t
#define OP_XY uint8_t
{
#define NYI() 0,
// 00
NYI() NYI() NYI() NYI()
NYI() OP(OpOr<AddrDirect<OP_A, OP_XY>>) NYI() NYI()
// 08
NYI() OP(OpOr<AddrImm<OP_A, OP_XY, OP_A>>) NYI() NYI()
NYI() OP(OpOr<AddrAbs<OP_A, OP_XY>>) NYI() OP(OpOr<AddrAbsLong<OP_A, OP_XY>>)
// 10
NYI() NYI() NYI() NYI()
NYI() NYI() NYI() NYI()
// 18
OP(CLC) NYI() NYI() NYI()
NYI() NYI() NYI() NYI()
// 20
NYI() NYI() NYI() NYI()
NYI() NYI() NYI() NYI()
// 28
NYI() NYI() NYI() NYI()
NYI() NYI() NYI() NYI()
// 30
NYI() NYI() NYI() NYI()
NYI() NYI() NYI() NYI()
// 38
NYI() NYI() NYI() NYI()
NYI() NYI() NYI() NYI()
// 40
NYI() NYI() NYI() NYI()
NYI() NYI() NYI() NYI()
// 48
NYI() NYI() NYI() NYI()
NYI() NYI() NYI() NYI()
// 50
NYI() NYI() NYI() NYI()
NYI() NYI() NYI() NYI()
// 58
NYI() NYI() NYI() NYI()
NYI() NYI() NYI() NYI()
// 60
NYI() NYI() NYI() NYI()
NYI() NYI() NYI() NYI()
// 68
NYI() NYI() NYI() NYI()
NYI() NYI() NYI() NYI()
// 70
NYI() NYI() NYI() NYI()
NYI() NYI() NYI() NYI()
// 78
NYI() NYI() NYI() NYI()
NYI() NYI() NYI() NYI()
// 80
NYI() NYI() NYI() NYI()
NYI() NYI() NYI() NYI()
// 88
NYI() NYI() NYI() NYI() 
NYI() NYI() NYI() NYI()
// 90
NYI() NYI() NYI() NYI()
NYI() NYI() NYI() NYI()
// 98
NYI() NYI() NYI() NYI()
NYI() NYI() NYI() NYI()
// A0
NYI() NYI() NYI() NYI()
NYI() NYI() NYI() NYI()
// A8
NYI() NYI() NYI() NYI()
NYI() NYI() NYI() NYI()
// B0
NYI() NYI() NYI() NYI()
NYI() NYI() NYI() NYI()
// B8
NYI() NYI() NYI() NYI()
NYI() NYI() NYI() NYI()
// C0
NYI() NYI() NYI() NYI()
NYI() NYI() NYI() NYI()
// C8
NYI() NYI() NYI() NYI()
NYI() NYI() NYI() NYI()
// D0
NYI() NYI() NYI() NYI()
NYI() NYI() NYI() NYI()
// D8
NYI() NYI() NYI() NYI()
NYI() NYI() NYI() NYI()
// E0
NYI() NYI() NYI() NYI()
NYI() NYI() NYI() NYI()
// E8
NYI() NYI() NYI() NYI()
NYI() NYI() NYI() NYI()
// F0
NYI() NYI() NYI() NYI()
NYI() NYI() NYI() NYI()
// F8
NYI() NYI() NYI() OP(XCE)
NYI() NYI() NYI() NYI()
},
// 6502 Native
{

},
#undef OP_XY
#undef OP_A

#undef NOP
