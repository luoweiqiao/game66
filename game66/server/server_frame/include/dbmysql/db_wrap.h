
#ifndef DB_WRAP_H__
#define DB_WRAP_H__

#include "utility/basicTypes.h"
#include "utility/basicFunctions.h"
#include "utility/stringFunctions.h"
#include <string>
#include "dbmysql/dbmysql.h"
#include "dbmysql/sql_join.h"
#include <vector>

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <iostream>
#include <sstream>
#include <map>


using namespace std;

class MYSQLValue
{
public:
    enum {
        emTYPE_STR   = 0,
        emTYPE_INT8,
        emTYPE_UINT8,
        emTYPE_INT16,
        emTYPE_UINT16,
        emTYPE_INT32,
        emTYPE_UINT32,
        emTYPE_INT64,
        emTYPE_BLOB,
    };

    MYSQLValue() {
        m_dataType = emTYPE_STR;
        m_bNull = true;
    }
    virtual ~MYSQLValue() {}

    //设置数据
    int SetData(const char* pData, int iLen, uint8 dataType = emTYPE_STR)
    {
        if (!pData)
        {
            m_bNull = true;
            return 0;
        }

        m_bNull = false;

        //m_strData.assign(pData, iLen);

		m_strData.resize(iLen + 1);
		memcpy((char*)m_strData.data(), pData, iLen);
		((char*)m_strData.data())[iLen] = '\0';

        m_dataType = dataType;

        return 0;
    }

    int size()
    {
        if (m_bNull)
        {
            return -1;
        }
        return m_strData.size() - 1;
    }

    const char* data()
    {
        if (m_bNull)
        {
            return NULL;
        }
        return m_strData.data();
    }
    uint8 dataType(){
        return m_dataType;
    }

    template <typename T> 
    T as()
    {
        T asVal;
        pri_as(asVal);

        return asVal;
    }

private:
    int pri_as(char *& val)
    {
        val = (char*)data();
        return 0;
    }
    int pri_as(const char *& val)
    {
        val = data();
        return 0;
    }
    int pri_as(char& val)
    {
        int32_t tmp_val;
        int ret = pri_as(tmp_val);
        val = tmp_val;
        return ret;
    }
    int pri_as(unsigned char& val)
    {
        uint32_t tmp_val;
        int ret = pri_as(tmp_val);
        val = tmp_val;
        return ret;
    }
    int pri_as(string& val)
    {
        if (m_bNull)
        {
            val = "";
        }
        val = m_strData.data();
        return 0;
    }
    template <typename T> 
    int pri_as(T& val)
    {
//Add-Begin by dantezhu in 2011-03-15 20:15:19
//要不然如果是空串的话，转成的int是随机值
        if (this->size() <= 0)
        {
            val = 0;
            return 0;
        }
//Add-End
        stringstream ss;
        ss << this->data();
        ss >> val;
        return 0;
    }

private:
    string m_strData;
    uint8  m_dataType;//数据类型

    bool m_bNull;
};


/*************************************************************/
class CDBWrap
{
protected:
	CDBMySql		m_clDatabase;
	char			m_szCommand[16 * 1024];
    map<string, MYSQLValue> m_tmpMapData;
    vector<MYSQLValue> m_tmpVecData;
    
public:
	CDBWrap(){}
	virtual ~CDBWrap(){}

public:
	bool	dbOpen(const char*host,const char*user,const char*passwd,const char*db =NULL,unsigned int port = 0);
	void	dbClose();
    CDBMySql* GetDBMysql();

	// 执行一条sql语句
	bool	ExeSql(string strSql);
	// 获得sql语句的结果数量
	uint32	GetResNumExeSql(string strSql);
    // 获得影响行数
    uint32  GetAffectedNumExeSql(string strSql);

	// 格式化执行sql语句
	bool	ExeSqlCmd(const char*pCmd,...);
	uint32  GetResNumExeSqlCmd(const char*pCmd,...);
    uint32  GetAffectedNumExeSqlCmd(const char*pCmd,...);

    // 返回列名对应的值
    int     Query(const char* strSql, vector<map<string, MYSQLValue> > &vecData);
    // 返回顺序的值
    int     Query(const char* strSql, vector< vector<MYSQLValue> > &vecData);
    // 插入数据
    uint32  Insert(const char* tblName,SQLJoin& data);
    // 更新数据
    int     Update(const char* tblName,SQLJoin& data,SQLJoin& where);
    // 插入或者更新
    int     UpdateOrInsert(const char* tblName,SQLJoin& data);
    // 删除数据
    int     Delete(const char* tblName,SQLJoin& where);

    // 插入数据
    string  GetInsertSql(const char* tblName,SQLJoin& data);
    // 更新数据
    string  GetUpdateSql(const char* tblName,SQLJoin& data,SQLJoin& where);
    // 插入或者更新
    string  GetUpdateOrInsertSql(const char* tblName,SQLJoin& data);
    // 删除数据
    string  GetDeleteSql(const char* tblName,SQLJoin& where);
    // select
    string  GetSelectSql(const char* tblName,SQLJoin& fileds,SQLJoin& where);

};




#endif // DB_WRAP_H__



























