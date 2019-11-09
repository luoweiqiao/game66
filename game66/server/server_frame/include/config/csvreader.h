/*
**  author: toney
**  create: 2015/12/3
*/

#ifndef __CSV_CONFIG_READER_H__
#define __CSV_CONFIG_READER_H__

#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include "helper/fileStream.h"

class CCsvConfigReader
{
public:
	CCsvConfigReader(void);
	~CCsvConfigReader(void);

public:
	// 打开文件
	bool	OpenFile(const char * filename, bool backup = true);
	// 保存文件
	bool    SaveFile(const char * filename);
	// 获得配置数据的行数
	int     GetRows();
	// 获得配置数据的列数
	int     GetCols();
public:
	// 初始化循环标志
	void    First(int nRow = 0);
	// 移动循环标志
	void    Next();
	// 判断是否结尾
	bool    IsDone();
public:
	// 返回字符串字段
	std::string 	GetFieldValueToString();
	short 			GetFieldValueToShort();
	int 			GetFieldValueToInt();
	double 			GetFieldValueToDouble();
	bool 			GetFieldValueToBool();

	// 根据返回字符串字段
	int				GetRowCount();
	std::string 	GetFieldValueToString(int rownum, 	std::string strcol);
	short 			GetFieldValueToShort(int rownum, 	std::string strcol);
	int 			GetFieldValueToInt(int rownum, 	    std::string strcol);
	double 			GetFieldValueToDouble(int rownum,   std::string strcol);
	bool 			GetFieldValueToBool(int rownum, 	std::string strcol);

protected:
	//直接从文件流读取值
	std::string	    ReadFieldValue(int row, int col);

	//循环遍历读取值
	std::string	    ReadFieldValue();
	std::string	    GetFieldValue(int row, const std::string & strcol);

private:
	CFileStream 							m_FileReader;			// 文件读取器
	bool									backup_;				// 内存备份开关
	std::ifstream							instream_;				// 文件输入流对象
	std::string								filename_;				// 文件名
	int										seek_ofset_;			// 当前文件字节偏移量

	int										currows_;				// 当前内存数据行
	int										curcols_;				// 当前内存数据列

private:
	std::vector< std::vector<std::string> >	data_;					// 内存数据
	int						                rows_;					// 内存数据行
	int						                cols_;					// 内存数据列
	std::map< std::string, int >			indexs_;				// 索引信息
	
};


#endif // __CSV_CONFIG_READER_H__




