// 대소문자 구별한다.

#pragma once

#include <list>

#define CONFIGPARSER				CConfigParser::GetInstance()
#define GET_CONFIG_VALUE(n)			(CONFIGPARSER.GetValue(n)) ? (CONFIGPARSER.GetValue(n)) : n##_VALUE

typedef std::list<void *> CConfigList;

//////////////////////////////////////////////////////////////////////////
class CConfigParser  
{
public:
	static CConfigParser& GetInstance();

protected:
	CConfigParser();
	CConfigParser(const CConfigParser&) {}
	//CConfigParser operator=(const CConfigParser&) {}
	~CConfigParser();

private:
	static void 			CreateInstance();
	static void 			KillInstance();

public:
	bool 					LoadFile(std::string strFilePath);
	void 					UnloadFile();
	bool					Reload(); 
	
	bool 					QueryValue(const char *  pName /*IN*/, char *  pValue /*OUT*/);
	char * 					GetValue(const char *  pName);

private:
	static CConfigParser*	_pInstance;
	static bool				_destoryed;

	CConfigList				m_ConfigList;
	std::string				m_strFilePath; 
	
};
