#include "Global.h"

namespace flaw {
	unsigned char Utils::GetCheckSum(const unsigned char* data, int length) {
        // 1�ܰ�: ��� ����Ʈ�� ���ϱ�
        int sum = 0;
        for (; length > 0; length--) {
            sum += data[length - 1];
        }

        // 2�ܰ�: ĳ�� �Ϻ��� ����
        sum &= 0xFF;

        // 3�ܰ�: 16�� ���� ���
        return (unsigned char)(~sum + 1);
	}
}