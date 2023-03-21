#include <stdio.h>
#include <stdlib.h>
void CheckCRC(int& chCurrByte, int chNextByte) {
    //每次调用进行8次循环，处理一个字节得数据
    for (int nMask = 0x80; nMask > 0; nMask >>= 1) {
        if ((chCurrByte & 0x80) != 0)//首位是1：移位，并进行异或运算
        {
            chCurrByte <<= 1;			//向左移一位，移出比特1
            if ((chNextByte & nMask) != 0)//右边补一位输入比特
            {
                chCurrByte |= 1;
            }
            chCurrByte ^= 7;			//首位已经移出，仅对低8位进行异或运算，7的二进制位0000，0111
        }
        else {							//首位为0，只移位，不进行异或运算
            chCurrByte <<= 1;			//向左移一位，移出比特0
            if ((chNextByte & nMask) != 0) {//右边补一位输入比特
                chCurrByte |= 1;
            }
        }
    }
}

int main()
{
    FILE* fp;
    //打开文件
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
        printf("前导码：");
        for (int j = 0; j < 7; j++)
            //找7个连续的Oxaa
        {
            if (ftell(fp) >= nFileEnd)
            {
                // 安全性检测
                printf("没有找到合法的帧\n");
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
            printf("没有找到合法的帧\n");
            fclose(fp);
            exit(0);
        }
        fread(&readByte, 1, 1, fp);
        if (readByte == 0xab)
        {
            printf("\n帧前定界符：%x", readByte);
        }
        //检测剩余文件是否包含完整帧头
    
    if (ftell(fp) + 14 > nFileEnd)
    {
        printf("没有找到完整的帧头，解析终止");
        fclose(fp);
        exit(0);
    }
    //输出目的地址，并校验
    printf("\n目的地址:");
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
    printf("\n源地址:");
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
    printf("\n类型字段:");
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
   // 定位下一个帧，以确定当前帧的结束位置
    //while (bParseCont)
    //{
        for (i = 0; i < 7; i++)
            //找下一个连续的7个0xaa
            if (ftell(fp) >= nFileEnd)
            {
                //到文件末尾，退出循环
                bParseCont = 0;
                //意味着不需要进行前导码的判定
                break;
            }
        //看当前字符是不是0xaa，如果不是，则重新寻找7个连续的0xaa
        fread(&readByte, 1, 1, fp);
        if (readByte != 0xaa)
        {
            i = -1;
        }
        //更新bParseCont: 如果直到文件结束仍没找到上述比特串，将终止主控循环的标记bParseCont置为true
        bParseCont = bParseCont & (ftell(fp) < nFileEnd);
        //判断7个连续的0xaa之后是否为0xab
        fread(&readByte, 1, 1, fp);
        if (readByte == 0xab)
        {
            continue;
        }

        //计算数据字段的长度


        if (nCurrDataLength == bParseCont) {
            //是否到达文件末尾
            nCurrDataLength = ftell(fp) - 8 - 1 - nCurrDataOffset;
        }// 没到文件末尾:下一帧头位置-前导码和定界符长度- CRC校验码长度-数据字段起始位置
        else {
            nCurrDataLength = ftell(fp) - 1 - nCurrDataOffset;
        }
        unsigned char b;
        fread(&b, 1, 1, fp);
        //读入CRC校验码
        int nTmpCRC = nCheck;
        CheckCRC(nCheck, c);
        //最后一步校验
        printf("\nCRC校验：");
        if ((nCheck & 0xff) == 0)
        {
            printf("正确");
        }
        else
        {
            printf("\n(错误)");
            printf("\t应为%02X", nTmpCRC & 0xff);
            bAccept = 0;
        }
        //将帧的接收标记置为false
        // 已到达文件末尾:文件末尾位置- CRC校验码长度-数据字段起始位置
        // Step5-6: 如果数据字段长度不足46字节或数据字段长度超过1500字节，则将帧的接收标记置为false
        if (nCurrDataLength < 46 || nCurrDataLength > 1500)
        {
            bAccept = 0;
        }
        // Step5-7: 输出帧的接收状态
        if (bAccept == 1)
            printf("\n状态: Accept\n");
        else
            printf("\n状态: Discard\n");
        nSN++;
        //帧序号加1
        nCurrDataOffset = ftell(fp) + 22;
    }
        printf("序号：%d", nSN);
        unsigned char string[1500];
        //将数据字段偏移量更新为下一-帧的帧头结束位置: 8 + 14
        printf("\n数据字段:");
        for (int i = 0; i < nCurrDataLength; i++)
        {
            fread(&string, 1, nCurrDataLength, fp);
            printf("%c", string[i]);
        }
        fclose(fp);
        return 0;
    }

