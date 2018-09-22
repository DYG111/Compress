#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include "huffman-tree2.hpp"
#include "file-compress2.h"

#define _SIZE_ 1024
#define _FILE_NAME_SIZE_ 128

namespace FileCompress2 {
	struct CodeInfo
	{
		CodeInfo()
			: code()
			, cnt(0)
		{}

		friend bool operator>(const CodeInfo &left, const CodeInfo &right)
		{
			return left.cnt > right.cnt;
		}

		friend bool operator!=(const CodeInfo &left, const CodeInfo &right)
		{
			return left.cnt != right.cnt;
		}

		friend CodeInfo operator+(const CodeInfo &left, const CodeInfo &right)
		{
			CodeInfo ret;
			ret.cnt = left.cnt + right.cnt;
			return ret;
		}

		unsigned char ch;	// �ַ�
		std::string code;	// ����
		long long cnt;		// ���ִ���
	};
	CodeInfo info[256];
	void UnCompressCore(FILE *input, FILE *output, HuffmanTreeNode2<CodeInfo> * pRoot)
	{
		assert(NULL != input);
		assert(NULL != output);

		unsigned char ReadBuf[_SIZE_];
		unsigned char WriteBuf[_SIZE_];
		memset(WriteBuf, '\0', _SIZE_);

		size_t n;
		size_t w_idx = 0;
		size_t pos = 0;
		HuffmanTreeNode2<CodeInfo> * pCur = pRoot;
		long long file_len = pRoot->_weight.cnt;
		do
		{
			memset(ReadBuf, '\0', _SIZE_);
			n = fread(ReadBuf, 1, _SIZE_, input);

			// ת��ReadBuf��WriteBuf
			size_t r_idx = 0;
			for (; r_idx < n; r_idx++)
			{
				// ת�������ֽ�
				unsigned char ch = ReadBuf[r_idx];
				for (; pos < 8; pos++, ch <<= 1)
				{
					if ((ch & 0x80) == 0x80)
					{
						pCur = pCur->pRight;
					}
					else
					{
						pCur = pCur->pLeft;
					}

					if (NULL == pCur->pLeft && NULL == pCur->pRight)
					{
						WriteBuf[w_idx++] = pCur->_weight.ch;
						pCur = pRoot;
						if (w_idx == _SIZE_)
						{
							fwrite(WriteBuf, 1, w_idx, output);
							memset(WriteBuf, '\0', _SIZE_);
							w_idx = 0;
						}
						file_len--;
					} // if
					if (file_len == 0)
						break;
				} // for
				if (pos == 8)
					pos = 0;

			} // for

		} while (n > 0);

		if (w_idx < _SIZE_ && w_idx > 0)
			fwrite(WriteBuf, 1, w_idx, output);
	}
	void GetLine(FILE *src, unsigned char *buf, int size)
	{
		assert(src);

		size_t n = 0;
		int index = 0;
		while (fread(buf + index, 1, 1, src) > 0)
		{
			if (index != 0 && buf[index] == '\n')
				break;

			index++;
		}
		buf[index] = '\0';
	}
	void GetHead(FILE *src, std::string &Postfix)
	{
		assert(src);

		// ��ȡ��׺��
		unsigned char buf[_FILE_NAME_SIZE_];
		int size = _FILE_NAME_SIZE_;
		GetLine(src, buf, size);
		Postfix += (char *)buf;

		// ��ȡ����
		GetLine(src, buf, size);
		int line = atoi((char *)buf);

		// ��ȡ�������ַ����ִ���
		while (line--)
		{
			GetLine(src, buf, size);
			info[*buf].cnt = atoi((char *)(buf + 2));
		}
	}



	//void GetFileName(const std::string &FilePath, std::string &output)
	//{
	//	// test.txt
	//	// f:\\ab\\c\\test.txt

	//	size_t begin = FilePath.find_last_of("\\");
	//	if (begin == std::string::npos)
	//	{
	//		begin = -1;
	//	}
	//	size_t end = FilePath.find_last_of(".");
	//	if (end == std::string::npos)
	//	{
	//		end = FilePath.length();
	//	}
	//	output = FilePath.substr(begin + 1, end - begin - 1);
	//}

	void GetPostfixName(const std::string &FilePath, std::string &output)
	{
		size_t begin = FilePath.find_last_of(".");
		if (begin != std::string::npos)
			output = FilePath.substr(begin, FilePath.length() - begin);
	}
	void FillCode(const HuffmanTreeNode2<CodeInfo> *pRoot)
	{
		if (NULL != pRoot)
		{
			FillCode(pRoot->pLeft);
			FillCode(pRoot->pRight);

			if (NULL == pRoot->pLeft && NULL == pRoot->pRight)
			{
				const HuffmanTreeNode2<CodeInfo> *tmp = pRoot;
				std::string code;
				const HuffmanTreeNode2<CodeInfo> *pParent = tmp->pParent;
				while (NULL != pParent)
				{
					if (pParent->pLeft == tmp)
					{
						code += "0";
					}
					else
					{
						code += "1";
					}
					tmp = pParent;
					pParent = tmp->pParent;
				}
				std::reverse(code.begin(), code.end());
				info[pRoot->_weight.ch].code = code;
			}
		}
	}
	void FillInfo(FILE *src)
	{
		assert(src);

		int i = 0;
		// ����ַ�
		for (; i < 256; ++i)
		{
			info[i].ch = i;
		}

		// �����ִ���
		unsigned char buf[_SIZE_];
		size_t n;
		do
		{
			n = fread(buf, 1, _SIZE_, src);
			size_t idx = 0;
			while (idx < n)
			{
				info[buf[idx++]].cnt++;
			}
		} while (n > 0);

		// ������
		CodeInfo invalid;
		invalid.cnt = 0;
		HuffmanTree2<CodeInfo> hfm(info, 256, invalid);

		FillCode(hfm.GetRoot());
	}
	void SaveCode(FILE *dst, const std::string &FilePath)
	{
		// ѹ���ļ�ͷ��ʽ:
		// ��չ��
		// ��������
		// ������ ...

		assert(NULL != dst);

		std::string output;
		GetPostfixName(FilePath, output);
		output += "\n";

		std::string code;
		int cnt = 0;
		unsigned char buf[33];
		for (int i = 0; i < 256; ++i)
		{
			if (info[i].cnt != 0)
			{
				cnt++;
				code += info[i].ch;
				code += ",";
				sprintf((char *)buf, "%lld", info[i].cnt);
				//_itoa((int)(info[i].cnt), (char *)buf, 10);
				code += (char *)buf;	//////////////////////////////////////////
				code += "\n";
			}
		}

		sprintf((char *)buf, "%d", cnt);
		//_itoa(cnt, (char *)buf, 10);
		output += (char *)buf;		////////////////////////
		output += "\n";
		output += code;

		fwrite(output.c_str(), 1, output.size(), dst);
	}
	void CompressCore(FILE *src, FILE *dst, const std::string &FilePath)
	{
		assert(NULL != src);
		assert(NULL != dst);

		fseek(src, 0, SEEK_SET);

		unsigned char buf[_SIZE_];
		unsigned char out[_SIZE_];
		int out_idx = 0;
		size_t n;
		int pos = 0;
		unsigned char ch = 0;

		SaveCode(dst, FilePath);

		// ������
		do
		{
			// ����ȡÿ���ֽ�ת��
			memset(buf, '\0', _SIZE_);
			n = fread(buf, 1, _SIZE_, src);
			size_t idx = 0;
			while (idx < n)
			{
				// ת�������ֽ�
				const std::string &CurCode = info[buf[idx++]].code;
				size_t len = CurCode.length();
				size_t i_len = 0;
				while (i_len < len)
				{
					for (; pos < 8 && i_len < len; pos++)
					{
						ch <<= 1;
						if (CurCode[i_len++] == '1')
						{
							ch |= 1;
						}
					}

					// �Ȼ��浽out
					if (8 == pos)
					{
						out[out_idx++] = ch;
						pos = 0;
						ch = 0;

						// ������ļ�
						if (_SIZE_ == out_idx)
						{
							fwrite(out, 1, out_idx, dst);
							out_idx = 0;
						}
					}
				} //while
			} // while
		} while (n > 0);

		// ����ʣ���λ
		if (8 > pos && 0 < pos)
		{
			int j = 0;
			while (j++ < 8 - pos)
				ch <<= 1;
			out[out_idx++] = ch;
		}

		// ����ʣ����ֽ�
		if (out_idx > 0)
			fwrite(out, 1, out_idx, dst);
	}

	void Compress(const std::string &InputFilePath, const std::string &OutputFilePath)
	{
		FILE *input = fopen(InputFilePath.c_str(), "rb");
		if (NULL == input)
		{
			std::cout << InputFilePath << " Not Found !" << std::endl;
			exit(1);
		}

		FillInfo(input);

		//std::string CompressFileName;
		//GetFileName(FilePath, CompressFileName);
		//CompressFileName += ".gl";

		FILE *output = fopen(OutputFilePath.c_str(), "wb");
		if (NULL == output)
		{
			std::cout << OutputFilePath << " Can Not Be Create !" << std::endl;
			exit(2);
		}

		CompressCore(input, output, InputFilePath);

		fclose(input);
		fclose(output);
	}

	void UnCompress(const std::string &InputFilePath, const std::string &OutputFilePath)
	{
		FILE *input = fopen(InputFilePath.c_str(), "rb");
		if (NULL == input)
		{
			std::cout << InputFilePath << " Not Found !" << std::endl;
			exit(3);
		}

		// ����ͷ����Ϣ
		std::string Postfix;
		GetHead(input, Postfix);

		// ��������ļ�
		//size_t begin = InputFilePath.find_first_of("\\");
		//if (begin == std::string::npos)
		//	begin = -1;
		//size_t end = InputFilePath.find_last_of(".");
		//if (end == std::string::npos)
		//	end = InputFilePath.length();
		//std::string FileName = InputFilePath.substr(begin + 1, end - begin - 1);
		//FileName += Postfix;
		FILE *output = fopen(OutputFilePath.c_str(), "wb");
		if (NULL == output)
		{
			std::cout << OutputFilePath << " Can Not Open !" << std::endl;
			exit(4);
		}

		int i = 0;
		// ����ַ�
		for (; i < 256; ++i)
		{
			info[i].ch = i;
		}

		CodeInfo invalid;
		invalid.cnt = 0;
		HuffmanTree2<CodeInfo> hfm(info, 256, invalid);

		UnCompressCore(input, output, hfm.GetRoot());

		fclose(input);
		fclose(output);
	}

}