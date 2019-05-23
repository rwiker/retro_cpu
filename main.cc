#include "system/c256/c256.h"

#include "jit.h"

#include <stdint.h>

bool fromhex(uint8_t& v, uint8_t ch)
{
	if(ch >= '0' && ch <= '9')
		v = ch - '0';
	else if(ch >= 'a' && ch <= 'f')
		v = ch - 'a' + 10;
	else if(ch >= 'A' && ch <= 'F')
		v = ch - 'A' + 10;
	else
		return false;
	return true;
}

bool WriteHexToMem(uint8_t *mem, size_t memsize, const std::vector<uint8_t>& hex)
{
	const uint8_t *p = hex.data();
	const uint8_t *end = p + hex.size();

	uint32_t addrbase = 0;

	uint8_t data[300];
	data[0] = 0;
	while(p < end) {
		if(*p == ':') {
			p++;
			uint32_t i;
			uint8_t sum = 0;
			for(i = 0; i < (uint32_t)data[0] + 5 && p < end; i++, p += 2) {
				uint8_t lo, hi;
				if(!fromhex(hi, p[0]) || !fromhex(lo, p[1]))
					return false;
				data[i] = lo | (hi << 4);
				sum += data[i];
			}
			if(sum != 0) // Bad checksum!
				return false;
			uint16_t addr = ((uint16_t)data[1] << 8) | data[2];
			uint8_t type = data[3];

			switch(type) {
			case 0:
				if(addrbase + addr + data[0] > memsize)
					return false;
				memcpy(mem + addrbase + addr, &data[4], data[0]);
				break;
			case 1:
				return true;
			case 4:
				if(data[0] != 2)
					return false;
				addrbase = (uint32_t)(((uint16_t)data[4] << 8) | data[5]) << 16;
				break;
			default:
				return false;
			}
		} else if(isspace(*p)) {
			p++;
		} else {
			return false;
		}
	}

	return true;
}

int main()
{
	auto kernel = NativeFile::Open("kernel.hex")->ReadToVector();
	C256 sys(0x400000, "", "");
	if(!WriteHexToMem(sys.rambase(), 0x400000, kernel))
		return 1;
	sys.PowerOn();
	sys.Emulate();

	auto jit = JitCoreFactory::Get()->CreateJit(&sys.cpu, &sys.sys);
	jit->Execute();

	return 0;
}
