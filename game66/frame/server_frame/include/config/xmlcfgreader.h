/**********************************************************************
*   copyright     : Tencent Innovation Center
*   version       : 0.1
*   author        : peterzhao
*   create        : 2008-10-18
*   description   : xml config file reader
Usage:
<Config>
	<cfg1 attr1="1" attr2="abcd" />
	<cfg2>
		<cfg3 attr1="3" />
		<cfg3>text</cfg3>
	</cfg2>
	<cfg4>6</cfg4>
</Config>

CXmlCfgReader xmlCfgReader;
if(xmlCfgReader.Load("filename"))
{
   int a;
   xmlCfgReader.GetAttrValue("Config.cfg1#attr1", a);
   char szbuf[100];
   xmlCfgReader.GetAttrValue("Config.cfg1#attr1", szbuf, sizeof(szbuf));

   if(xmlCfgReader.SetCurNode("Config.cfg2.cfg3"))
   {
       int b;
	   xmlCfgReader.GetAttrValue("attr1", b);
	   if(xmlCfgReader.GoToBrotherNode("cfg3"))
	   {
			char szbuf2[100];
			xmlCfgReader.GetTextValue("", szbuf2, sizeof(szbuf2));
	   }
   }

   int c;
   xmlCfgReader.GetTextValue("Config.cfg4", c);
}
***********************************************************************/
#ifndef __CFG_READER_H___
#define __CFG_READER_H___

#include "tinyxml/tinyxml.h"
#include <string>

namespace svrlib
{
#define XML_CFG_NODE_NAME_MARK	"."
#define XML_CFG_NODE_ATTR_MARK	"#"

class CXmlCfgReader
{
public:
	CXmlCfgReader(char const *pszCfgFile) ;

	bool	SetCurNode(const char *pszPath, bool bFromRoot=false);
    bool    SetCurNode(TiXmlElement *pElem);
    TiXmlElement * GetCurNode() {   return  m_pCurElem; }
	bool	GoToBrotherNode(const char *pszNodeName);

	unsigned int GetAtrrAsUint(char const * pszPath, unsigned int uiDefault = 0, bool bFromRoot = false) ;
	int GetAtrrAsInt(char const * pszPath, int iDefault = 0, bool bFromRoot = false) ;
	std::string GetAtrrAsString(char const * pszPath, char const * pDefualt = "", bool bFromRoot = false) ;
	
	unsigned int GetTextAsUint(char const * pszPath, unsigned int uiDefault = 0, bool bFromRoot = false) ;
	int GetTextAsInt(char const * pszPath, int iDefault = 0, bool bFromRoot = false) ;
	std::string GetTextAsString(char const * pszPath, char const * pDefualt = "", bool bFromRoot = false) ;
	
private:
	bool	Load(const char *pszCfgFile) ;
	bool	GetAttrValue(const char *pszPath, std::string &strValue, bool bFromRoot = false);
	bool	GetTextValue(const char *pszPath, std::string &strValue, bool bFromRoot = false);
private:
	TiXmlDocument m_XmlDoc ;
	TiXmlElement * m_pCurElem ;
};

} ;
#endif

