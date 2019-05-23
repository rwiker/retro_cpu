#ifndef X64_EMITTER_H_
#define X64_EMITTER_H_

enum {
	RAX,
	RCX,
	RDX,
	RBX,
	RSP,
	RBP,
	RSI,
	RDI,
	R8,
	R9,
	R10,
	R11,
	R12,
	R13,
	R14,
	R15
};

#define MODRM(mod, reg, rm) (((mod) << 6) | ((reg) << 3) | (rm))

class X64Emitter
{
public:
	X64Emitter(uint8_t *ptr, uint32_t size) : ptr(ptr), end(ptr + size) {}

	void zero_reg(uint32_t reg);
	void mov_rr_32(uint32_t rd, uint32_t rs);
	void mov_rm_32(uint32_t rd, uint32_t rs, uint32_t offset);
	void mov_mr_32(uint32_t rd, uint32_t rs, uint32_t offset)
	{

	}
	void mov_r_imm_32(uint32_t rd, uint32_t imm);
	void cmp_m_8(uint32_t rs, uint32_t offset, uint8_t imm)
	{
		*ptr++ = 0x80;
		*ptr++ = MODRM(1, 7, rs & 7);
		*(uint32_t*)ptr = offset;
		ptr += 4;
		*ptr++ = imm;
	}
	void mov_m_imm_8(uint32_t rs, uint32_t offset, uint8_t imm)
	{
		*ptr++ = 0xC6;
		uint8_t mod;
		if(offset == 0) {
			mod = 0;
		} else if(offset <= 0xFF) {
			mod = 1;
		} else {
			mod = 2;
		}
		*ptr++ = MODRM(mod, 0, rs & 7);
		if(offset == 0) {
		} else if(offset <= 0xFF) {
			*ptr++ = offset;
		} else {
			*(uint32_t*)ptr = offset;
			ptr += 4;
		}
		*ptr++ = imm;
	}

private:
	uint8_t *ptr;
	uint8_t *end;
};



#endif
