#include <stdio.h>
#include <stdlib.h>
void CheckCRC(int& chCurrByte, int chNextByte) {
    //ÿ�ε��ý���8��ѭ��������һ���ֽڵ�����
    for (int nMask = 0x80; nMask > 0; nMask >>= 1) {
        if ((chCurrByte & 0x80) != 0)//��λ��1����λ���������������
        {
            chCurrByte <<= 1;			//������һλ���Ƴ�����1
            if ((chNextByte & nMask) != 0)//�ұ߲�һλ�������
            {
                chCurrByte |= 1;
            }
            chCurrByte ^= 7;			//��λ�Ѿ��Ƴ������Ե�8λ����������㣬7�Ķ�����λ0000��0111
        }
        else {							//��λΪ0��ֻ��λ���������������
            chCurrByte <<= 1;			//������һλ���Ƴ�����0
            if ((chNextByte & nMask) != 0) {//�ұ߲�һλ�������
                chCurrByte |= 1;
            }
        }
    }
}

int main()
{
    FILE* fp;
    //���ļ�
    fp = fopen("input", "rb+");
    if (fp == NULL)
    {
        perror("Error opening file");
    }
    int nFileEnd;
    int nCurrDataLength = 0;
    int bAccept = 1;
    unsigned char readByte;
    unsigned char c;
    int bParseCont = 1;
    int i;
    int nCheck;
    int nSN = 1;
    fseek(fp, 0, SEEK_END);
    nFileEnd = ftell(fp);
    rewind(fp);

    while (bParseCont)
    {
        nSN++;
        printf("ǰ���룺");
        for (int j = 0; j < 7; j++)
            //��7��������Oxaa
        {
            if (ftell(fp) >= nFileEnd)
            {
                // ��ȫ�Լ��
                printf("û���ҵ��Ϸ���֡\n");
                fclose(fp);
                exit(0);
            }
            fread(&readByte, 1, 1, fp);
            if (readByte != 0xaa)
            {
                j = -1;
            }
        }
        printf("%02x", readByte);
        if (ftell(fp) >= nFileEnd)
        {
            printf("û���ҵ��Ϸ���֡\n");
            fclose(fp);
            exit(0);
        }
        fread(&readByte, 1, 1, fp);
        if (readByte == 0xab)
        {
            printf("\n֡ǰ�������%x", readByte);
        }
        //���ʣ���ļ��Ƿ��������֡ͷ
    
    if (ftell(fp) + 14 > nFileEnd)
    {
        printf("û���ҵ�������֡ͷ��������ֹ");
        fclose(fp);
        exit(0);
    }
    //���Ŀ�ĵ�ַ����У��
    printf("\nĿ�ĵ�ַ:");
    for (int i = 0; i < 6; i++)
    {
        fread(&c, 1, 1, fp);
        if (i == 5)
            printf("%02X", c);
        else
            printf("%02X-", c);
        if (i == 0)
        {
            nCheck = c;
        }
        else
        {
            CheckCRC(nCheck, c);
        }
    }
    printf("\nԴ��ַ:");
    for (int i = 0; i < 6; i++)
    {
        fread(&c, 1, 1, fp);
        if (i == 5)
            printf("%02X", c);
        else
            printf("%02X-", c);
        if (i == 0)
        {
            nCheck = c;
        }
        else
        {
            CheckCRC(nCheck, c);
        }
    }
    printf("\n�����ֶ�:");
    for (int i = 0; i < 2; i++)
    {
        fread(&c, 1, 1, fp);

        printf("%02X ", c);
        if (i == 0)
        {
            nCheck = c;
        }
        else
        {
            CheckCRC(nCheck, c);
        }
    }
    int nCurrDataOffset = ftell(fp);
    // Step5-2:
   // ��λ��һ��֡����ȷ����ǰ֡�Ľ���λ��
    //while (bParseCont)
    //{
        for (i = 0; i < 7; i++)
            //����һ��������7��0xaa
            if (ftell(fp) >= nFileEnd)
            {
                //���ļ�ĩβ���˳�ѭ��
                bParseCont = 0;
                //��ζ�Ų���Ҫ����ǰ������ж�
                break;
            }
        //����ǰ�ַ��ǲ���0xaa��������ǣ�������Ѱ��7��������0xaa
        fread(&readByte, 1, 1, fp);
        if (readByte != 0xaa)
        {
            i = -1;
        }
        //����bParseCont: ���ֱ���ļ�������û�ҵ��������ش�������ֹ����ѭ���ı��bParseCont��Ϊtrue
        bParseCont = bParseCont & (ftell(fp) < nFileEnd);
        //�ж�7��������0xaa֮���Ƿ�Ϊ0xab
        fread(&readByte, 1, 1, fp);
        if (readByte == 0xab)
        {
            continue;
        }

        //���������ֶεĳ���


        if (nCurrDataLength == bParseCont) {
            //�Ƿ񵽴��ļ�ĩβ
            nCurrDataLength = ftell(fp) - 8 - 1 - nCurrDataOffset;
        }// û���ļ�ĩβ:��һ֡ͷλ��-ǰ����Ͷ��������- CRCУ���볤��-�����ֶ���ʼλ��
        else {
            nCurrDataLength = ftell(fp) - 1 - nCurrDataOffset;
        }
        unsigned char b;
        fread(&b, 1, 1, fp);
        //����CRCУ����
        int nTmpCRC = nCheck;
        CheckCRC(nCheck, c);
        //���һ��У��
        printf("\nCRCУ�飺");
        if ((nCheck & 0xff) == 0)
        {
            printf("��ȷ");
        }
        else
        {
            printf("\n(����)");
            printf("\tӦΪ%02X", nTmpCRC & 0xff);
            bAccept = 0;
        }
        //��֡�Ľ��ձ����Ϊfalse
        // �ѵ����ļ�ĩβ:�ļ�ĩβλ��- CRCУ���볤��-�����ֶ���ʼλ��
        // Step5-6: ��������ֶγ��Ȳ���46�ֽڻ������ֶγ��ȳ���1500�ֽڣ���֡�Ľ��ձ����Ϊfalse
        if (nCurrDataLength < 46 || nCurrDataLength > 1500)
        {
            bAccept = 0;
        }
        // Step5-7: ���֡�Ľ���״̬
        if (bAccept == 1)
            printf("\n״̬: Accept\n");
        else
            printf("\n״̬: Discard\n");
        nSN++;
        //֡��ż�1
        nCurrDataOffset = ftell(fp) + 22;
    }
        printf("��ţ�%d", nSN);
        unsigned char string[1500];
        //�������ֶ�ƫ��������Ϊ��һ-֡��֡ͷ����λ��: 8 + 14
        printf("\n�����ֶ�:");
        for (int i = 0; i < nCurrDataLength; i++)
        {
            fread(&string, 1, nCurrDataLength, fp);
            printf("%c", string[i]);
        }
        fclose(fp);
        return 0;
    }

