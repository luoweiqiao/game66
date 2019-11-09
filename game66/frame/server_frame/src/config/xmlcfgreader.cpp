#include "config/xmlcfgreader.h"
#include <string>
#include "utility/stringutility.h"

using namespace svrlib;

CXmlCfgReader::CXmlCfgReader(char const *pszCfgFile)
   : m_pCurElem(NULL)
{
	Load(pszCfgFile) ;
}

bool CXmlCfgReader::Load(const char *pszCfgFile)
{
	if(m_XmlDoc.LoadFile(pszCfgFile))
	{
		m_pCurElem = m_XmlDoc.RootElement();
		return true;
	}
	return false;
}

bool CXmlCfgReader::SetCurNode(TiXmlElement *pElem)
{
	assert(pElem != NULL);
	m_pCurElem = pElem;
	return true;
}

bool CXmlCfgReader::SetCurNode(const char *pszPath, bool bFromRoot)
{
	if(NULL==pszPath || (bFromRoot && NULL == m_pCurElem))	
	{
		return false ;
	}

	std::string strPath = pszPath;
	TiXmlElement *pElem = bFromRoot ? m_XmlDoc.RootElement() : m_pCurElem;
	std::vector<std::string> vectNames;
	CStringUtility::SplitString(strPath, XML_CFG_NODE_NAME_MARK, vectNames);
	if (vectNames.size() == 0)
	{
		pElem = pElem->FirstChildElement(strPath.c_str());
		if (NULL == pElem)
		{
			return false;
		}
	}
	else
	{
		for (unsigned int i = 0; i < vectNames.size(); i++)
		{
			pElem = pElem->FirstChildElement(vectNames[i].c_str());
			if (NULL == pElem)
			{
				return false;
			}
		}
	}
	m_pCurElem = pElem;
	return true;
}

bool CXmlCfgReader::GoToBrotherNode(const char *pszNodeName)
{
	assert(m_pCurElem != NULL);
	TiXmlElement *pElem = m_pCurElem->NextSiblingElement(pszNodeName);
	if (pElem != NULL)
	{
		m_pCurElem = pElem;
		return true;
	}
	return false;
}

bool CXmlCfgReader::GetAttrValue(const char *pszPath, std::string &strValue, bool bFromRoot/*=false*/)
{
	if(NULL==pszPath || ((!bFromRoot) && (NULL == m_pCurElem)))	
	{
		return false ;
	}
	std::string strPath = pszPath;
	TiXmlElement *pElem = bFromRoot ? m_XmlDoc.RootElement() : m_pCurElem ;

	std::vector<std::string> vectStrs ;
	CStringUtility::SplitString(strPath, XML_CFG_NODE_ATTR_MARK, vectStrs);

	char* pszValueStr = NULL;
	if (vectStrs.size() == 0)
	{
		pszValueStr=const_cast<char*>(pElem->Attribute(strPath.c_str()));
	}
	else
	{
		if (vectStrs.size() == 1)
		{
			pszValueStr = const_cast<char*>(pElem->Attribute(vectStrs[0].c_str()));
		}
		else if (vectStrs.size() == 2)
		{
			std::string strNodePath = vectStrs[0];
			std::string strAttrName = vectStrs[1];
			vectStrs.clear();
			CStringUtility::SplitString(strNodePath, XML_CFG_NODE_NAME_MARK, vectStrs);
			if (vectStrs.size() == 0)
			{
				pElem = pElem->FirstChildElement(strNodePath.c_str());
				if (NULL == pElem)
				{
					return false;
				}
			}
			else
			{
				for(size_t i=0; i<vectStrs.size(); i++)
				{
					pElem = pElem->FirstChildElement(vectStrs[i].c_str());
					if (NULL == pElem)
					{
						return false;
					}
				}
			}
			pszValueStr = const_cast<char*>(pElem->Attribute(strAttrName.c_str()));
		}
		else
		{
			return false;
		}
	}

	if (NULL == pszValueStr)
	{
		return false;
	}
	strValue = pszValueStr;
	return true;
}

bool CXmlCfgReader::GetTextValue(const char *pszPath, std::string &strValue, bool bFromRoot)
{
	if(NULL == pszPath || (!bFromRoot && NULL==m_pCurElem))
	{
		return false;
	}

	std::string strPath = pszPath;
	TiXmlElement *pElem = bFromRoot ? m_XmlDoc.RootElement():m_pCurElem;
	if (strPath != "")
	{
		std::vector<std::string> vectStrs;
		CStringUtility::SplitString(strPath, XML_CFG_NODE_NAME_MARK, vectStrs);
		if (vectStrs.size() == 0)
		{
			pElem = pElem->FirstChildElement(strPath.c_str());
			if (NULL == pElem)
			{
				return false;
			}
		}
		else
		{
			for(size_t i = 0; i < vectStrs.size(); i++)
			{
				pElem = pElem->FirstChildElement(vectStrs[i].c_str());
				if (NULL == pElem)
				{
					return false;
				}
			}
		}
	}

	char *pszValueStr = NULL;
	TiXmlNode *pNode = pElem->FirstChild();
	if (pNode != NULL)
	{
		TiXmlText *pText = pNode->ToText();
		if(pText != NULL)
		{
			pszValueStr = const_cast<char*>(pText->Value());
		}
	}

	if (NULL == pszValueStr)
	{
		return false;
	}
	strValue = pszValueStr;
	return true;
}


unsigned int CXmlCfgReader::GetAtrrAsUint(char const * pszPath, unsigned int uiDefault, bool bFromRoot)
{
	bFromRoot = bFromRoot;
	
	std::string strRes = GetAtrrAsString(pszPath) ;
	if (strRes.empty())
	{
		return uiDefault ;
	}
	return strtoul(strRes.c_str(), 0, 10) ;
}

int CXmlCfgReader::GetAtrrAsInt(char const * pszPath, int iDefault, bool bFromRoot) 
{
	bFromRoot = bFromRoot;

	std::string strRes = GetAtrrAsString(pszPath) ;
	if (strRes.empty())
	{
		return iDefault ;
	}
	return atoi(strRes.c_str()) ;
}

std::string CXmlCfgReader::GetAtrrAsString(char const * pszPath, char const * pDefualt, bool bFromRoot)
{
	std::string strRes ;
	if (!GetAttrValue(pszPath, strRes, bFromRoot))
	{
		strRes = pDefualt ;
	}
	return strRes ;
}


unsigned int CXmlCfgReader::GetTextAsUint(char const * pszPath, unsigned int uiDefault, bool bFromRoot)
{
	bFromRoot = bFromRoot;
	
	std::string strRes = GetTextAsString(pszPath) ;
	if (strRes.empty())
	{
		return uiDefault ;
	}
	return strtoul(strRes.c_str(), 0, 10) ;
}

int CXmlCfgReader::GetTextAsInt(char const * pszPath, int iDefault, bool bFromRoot)
{
	bFromRoot = bFromRoot;
	
	std::string strRes = GetTextAsString(pszPath) ;
	if (strRes.empty())
	{
		return iDefault ;
	}
	return atoi(strRes.c_str()) ;
}

std::string CXmlCfgReader::GetTextAsString(char const * pszPath, char const * pDefualt, bool bFromRoot)
{
	std::string strRes ;
	if (!GetTextValue(pszPath, strRes, bFromRoot))
	{
		strRes = pDefualt ;
	}
	return strRes ;
}


