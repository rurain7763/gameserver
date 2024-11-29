#include "Global.h"

namespace flaw {
	unsigned char Utils::GetCheckSum(const unsigned char* data, int length) {
        // 1단계: 모든 바이트를 더하기
        int sum = 0;
        for (; length > 0; length--) {
            sum += data[length - 1];
        }

        // 2단계: 캐리 니블을 버림
        sum &= 0xFF;

        // 3단계: 16의 보수 계산
        return (unsigned char)(~sum + 1);
	}
}