#include "common_hdr.h"
#include "config_parser.h"

//
#pragma pack(push, 1)
typedef struct _CFG_ITEM {
	char				name[512];
	char				value[512];
} CFG_ITEM, *PCFG_ITEM;
#pragma pack(pop)

CConfigParser*	CConfigParser::_pInstance = 0;
bool			CConfigParser::_destoryed = false;
//////////////////////////////////////////////////////////////////////////
CConfigParser::CConfigParser()
{
	if (!_pInstance)
		m_ConfigList.clear();
}

CConfigParser::~CConfigParser()
{
	if (_pInstance)
		UnloadFile();

	_pInstance = 0;
	_destoryed = true;
}

CConfigParser& CConfigParser::GetInstance()
{
	if (!_pInstance)
	{
		if (_destoryed)
		{
			CreateInstance();
#ifdef _USE_MEM_EXCEPTION_
			try {
				new (_pInstance) CConfigParser;
			}
			catch (std::exception& e){
				LOG_WRITE(("%s():%d %s", __FUNCTION__, __LINE__, e.what()));
			}
#else
			//new (_pInstance) CConfigParser;
			new CConfigParser;
#endif
			atexit(KillInstance);
			_destoryed = false;
		}
		
		if (!_pInstance)
		{
			CreateInstance();
		}
	}
	return *_pInstance;
}

void CConfigParser::CreateInstance()
{
	static CConfigParser _instance;
	_pInstance = &_instance;
}

void CConfigParser::KillInstance()
{
	//throw std::runtime_error("Dead Reference Detected");
	_pInstance->~CConfigParser();
}


//////////////////////////////////////////////////////////////////////////
static bool _IsSkipChar(char Value)
{
	if( Value=='\t' || Value=='\r' || Value=='\n' || Value==' ' )
	{
		return true;
	}
	return false;
}

static char *  _GetTokenStart(char *  pStr)
{
	char * 	pt_ch = nullptr;

	for(pt_ch=pStr; *pt_ch != '\0' && _IsSkipChar(*pt_ch); pt_ch++) ;
	if( *pt_ch == '\0' ) { return nullptr; }

	return pt_ch;
}

static char *  _GetTokenEnd(char *  pStr)
{
	char * 	pt_ch = nullptr;

	for(pt_ch=pStr; *pt_ch != '\0' && !_IsSkipChar(*pt_ch); pt_ch++) ;

	return pt_ch;
}

static bool _SplitAndParseLine(char *  pLine /*IN*/, char ** ppName /*OUT*/, char ** ppValue /*OUT*/)
{
	char * 	pt_ch = nullptr;
	char * 	pt_name = nullptr;
	char * 	pt_value = nullptr;

	// 파라미터 체크
	if( !pLine || !ppName || !ppValue ) { return false; }

	// 주석처리
	if( (pt_ch = strchr(pLine, ';')) != nullptr ) { pt_ch = nullptr; }

	// 아이템이름
	pt_name = _GetTokenStart(pLine); if( !pt_name ) { return false; }
	pt_ch = _GetTokenEnd(pt_name+1); if( !pt_ch || *pt_ch == '\0' ) { return false; }
	*pt_ch = '\0';

	// 아이템값
	pt_value = _GetTokenStart(pt_ch+1); if( !pt_value ) { return false; }
	pt_ch = _GetTokenEnd(pt_value+1); *pt_ch = '\0';

	// 설정
	*ppName = pt_name;
	*ppValue = pt_value;

	return true;
}

//////////////////////////////////////////////////////////////////////////
bool CConfigParser::LoadFile(std::string strFilePath)
{
	FILE	*fp = nullptr;
	char	buf[1024];

	// 파라미터 체크
	if(strFilePath.empty() == true) { return false; }

	// 이전 설정값 리스트 삭제
    if( m_ConfigList.size() ) { UnloadFile(); }

	// 파일 오픈
	fp = fopen(strFilePath.c_str(), "r");
	if( !fp ) { return false; }

	// 파싱!
	while(fgets(buf, sizeof(buf), fp))
	{
		char * 		pt_name = nullptr;
		char * 		pt_value = nullptr;
		PCFG_ITEM	pt_item = nullptr;

		// 이름과 값 얻기
		if( !_SplitAndParseLine(buf, &pt_name, &pt_value) ) { continue; }

		// 할당
		pt_item = (PCFG_ITEM)malloc(sizeof(CFG_ITEM));
		strcpy(pt_item->name, pt_name);
		strcpy(pt_item->value, pt_value);
		m_ConfigList.push_back(pt_item);
	}

	// 파일 닫기
	fclose(fp); 
	fp = nullptr;

	m_strFilePath = strFilePath;
	
	return true;
}

void CConfigParser::UnloadFile()
{	 
    PCFG_ITEM pt_item = nullptr;
	for(auto item=m_ConfigList.begin(); item!=m_ConfigList.end(); ++item)
	{
        pt_item = (PCFG_ITEM)*item;
        if( pt_item )
        {
		    free(pt_item);
            pt_item = nullptr;
        }
	}
	m_ConfigList.clear();
}

//////////////////////////////////////////////////////////////////////////
bool CConfigParser::QueryValue(const char *  pName /*IN*/, char *  pValue /*OUT*/)
{
	// 파라미터 체크
	if( !pName || !pValue ) { return false; }

	// 초기화
	pValue[0] = '\0';

	// 검색
	for(auto item=m_ConfigList.begin(); item!=m_ConfigList.end(); ++item)
	{
		PCFG_ITEM	pt_item = (PCFG_ITEM)*item;	

		// 체크
		if( !strcmp(pt_item->name, pName) )
		{
			strcpy(pValue, pt_item->value);
			return true;
		}
	}

	return false;
}

char *  CConfigParser::GetValue(const char *  pName)
{
	// 파라미터 체크
	if( !pName ) { return nullptr; }

	// 검색
	for(auto item=m_ConfigList.begin(); item!=m_ConfigList.end(); ++item)
	{
		PCFG_ITEM	pt_item = (PCFG_ITEM)*item;	

		// 체크
		if( !strcmp(pt_item->name, pName) )
		{
			return pt_item->value;
		}
	}

	return nullptr;
}


//====================================================================================================
// 설정 장보 다시  로드 
//====================================================================================================
bool CConfigParser::Reload()
{
	UnloadFile();


	return LoadFile(m_strFilePath); 
}



