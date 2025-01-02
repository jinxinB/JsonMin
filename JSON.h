
/*
	��ʽ�ο��� ECMA-404 RFC7159
	
	����˵����
	JSON������Ҫ��С�����ݿռ䣺����Ҫ�����Ľṹ���ݣ��Լ���������������󻺴�(�������ƻ�����������󳤶ȵĻ���)
	�������ֽڷ�ʽ�����������룬����״̬����ʽ���н���
	���������ݽ���ʱ����״̬���жϲ�ȡ�������ݣ��ٸ�����Ҫ�����ݽ��д洢��
	��˴����������Ҫʹ��״̬���ķ�ʽ�жϲ��洢JSON��Ϣ��ͬʱҲ��Ҫ��JSON���Ƽ��������ݸ�ʽ����У��
	
	���ɵ����ݽ�����
	֧�ִ󲿷���Ч�ķ��ֽڷ������ͣ���true,false,null�����ֽ�������
	���Ը��� JSON_IsKeywordNumToken ��ȷ����ǰ�����Ƿ�Ϊ�ؼ��ֻ���ֵ���ͣ���ʹ�� JSON_CurrentTokenType ��һ���ж������Ƿ���Ч
	
	JSON ��̬�������裺
	��������ṹ sJSONContext ctx; �� �������ݻ��� unsigned char Cache[CacheSize];
	ͨ������ JSON_InitContext(&ctx , Cache , sizeof(Cache)); ��ʼ��
	��ѭ������ ret = ctx.pParse(&ctx , c); ���ж��䷵��ֵȷ������״̬
	���� JSON_PBTM_Parseing ��ǰ�ַ�Ϊ�ǹؼ��ַ�������������
	���ذ��� JSON_ParseByteType_Error ���ʱ ״ֵ̬��7λΪ JSON_ErrT_XXX ����״̬�룬��ʹ�� JSON_ParseByteType_IsError ������ж�
	���ذ��� JSON_PBTM_Token ���ʱ ctx.pCache �� ctx.cachePos ��Ч��cachePosΪ���ݳ��ȣ�pCacheΪ���ݻ���ָ��
	�� JSON_PBTM_Parseing ״̬ʱ ��ͨ�� ctx.stackPoint �жϵ�ǰ�ṹǶ�ײ��� ������ͨ�� JSON_CurrentStackIsObject(&ctx) �����жϵ�ǰ�ṹ�Ƿ�Ϊ���������
	����������״̬��Ҳ����ֱ�ӱȽ�״ֵ̬�����жϣ��磺 JSON_ParseByteType_ObjectStart , JSON_ParseByteType_NameEnd ��
	
*/

// �������̱��
// ParseByteTypeMask
// ��ǰ������
#define JSON_PBTM_Parseing 					0x00
// ��ǰΪ��ǽڵ�
#define JSON_PBTM_Token 					0x01
// �ڵ����״̬����״̬ cache ������Ч
#define JSON_PBTM_End 						0x02
// ��ǰΪ�ṹ�ڵ㣨��������飩
#define JSON_PBTM_Struct 					0x04
// ���ݽڵ�Ϊ���������ṹ�ڵ�Ϊ����
#define JSON_PBTM_NameOrObj 				0x08
// �����쳣
#define JSON_PBTM_Error 					0x80

// ��������
// �ڲ�����
#define JSON_ErrT_InternalError 			0x00
// ��ջ��
#define JSON_ErrT_StackFull 				0x01
// ������
#define JSON_ErrT_CacheFull 				0x02
// ���������쳣
#define JSON_ErrT_RuleException 			0x03

// ������������
// ���ڴ����ҵ�ǰ�ǹؼ��ַ�������Ҫ���������ж�
#define JSON_ParseByteType_Parseing 		JSON_PBTM_Parseing
// ��ǰΪ�������ƽ�����pCacheΪ������
#define JSON_ParseByteType_NameEnd 			(JSON_PBTM_Token|JSON_PBTM_NameOrObj|JSON_PBTM_End)
// ��ǰΪ�������ݽ�����pCacheΪ��������
#define JSON_ParseByteType_ValueEnd 		(JSON_PBTM_Token|JSON_PBTM_End)
// ��ǰΪ����ṹ��ʼ
#define JSON_ParseByteType_ObjectStart 		(JSON_PBTM_Token|JSON_PBTM_Struct|JSON_PBTM_NameOrObj)
// ��ǰΪ����ṹ��ʼ
#define JSON_ParseByteType_ArrayStart 		(JSON_PBTM_Token|JSON_PBTM_Struct)
// ��ǰΪ����ṹ������pCacheΪǰ��һ���������������������
#define JSON_ParseByteType_ObjectEnd 		(JSON_PBTM_Token|JSON_PBTM_Struct|JSON_PBTM_NameOrObj|JSON_PBTM_End)
// ��ǰΪ����ṹ������pCacheΪǰ��һ���������һ�����������
#define JSON_ParseByteType_ArrayEnd 		(JSON_PBTM_Token|JSON_PBTM_Struct|JSON_PBTM_End)
// �쳣״̬��ǣ�������ֱ���ж��Ƿ���ȣ�����ֵ�������������ݣ�
#define JSON_ParseByteType_Error 			JSON_PBTM_Error
// �쳣״̬�ж�
#define JSON_ParseByteType_IsError(type) 	(((type)&JSON_PBTM_Error) != 0)
// �쳣״̬�жϣ�type ����Ϊ�޷���״ֵ̬����
#define JSON_ParseByteType_IsErrorU(type) 	((type) >= JSON_PBTM_Error)

// ��ǰ���������Ƿ�Ϊ�ؼ��ֻ���ֵ
#define JSON_IsKeywordNumToken(ctx) 	((ctx).cachePos >= (ctx).cacheLen || (ctx).pCache[(ctx).cachePos] == 0)

// ����ջ�ߴ磨�������Ƕ����ȣ��ߴ�*8��
#ifndef JSON_MaxStackSize
#define JSON_MaxStackSize 		4
#endif

typedef unsigned char (*fJSONContextParse)(void* pCtx , unsigned char c);
// �������ҽṹ
typedef struct _sJSONContext_
{
	// ����״̬
	unsigned char type;
	// ����ջָ��λ��
	unsigned char stackPoint;
	// �����ܳ���
	unsigned char cacheLen;
	// ����ʹ��λ��
	unsigned char cachePos;
	// ����ָ��
	unsigned char* pCache;
	// ��������ָ��
	fJSONContextParse pParse;
	// ����ջ ÿ1λ��Ӧһ������ջ���ͣ�0����1���飩
	unsigned char stack[JSON_MaxStackSize];
} sJSONContext;

// ��ʼ�������ṹ
void JSON_InitContext(sJSONContext* pCtx , unsigned char* pCache , unsigned char cacheLen);

// ��ǰ�Ƿ�Ϊ����ṹ�У�����Ϊ����ṹ��
BOOL JSON_CurrentStackIsObject(sJSONContext* pCtx);

// ��ֵ��������  JSON_ValueParse.c
// �������
// �ַ��� ""
#define JSON_TokenType_String 			0
// ���� +1.2e-3
#define JSON_TokenType_Number 			1
// �ؼ��� true
#define JSON_TokenType_KW_True 			2
// �ؼ��� false
#define JSON_TokenType_KW_False 		3
// �ؼ��� null
#define JSON_TokenType_KW_Null 			4
// δ֪
#define JSON_TokenType_Unknow 			255
// ��ȡ��ǰ�������
unsigned char JSON_CurrentTokenType(sJSONContext* pCtx);

// ��ֵ������Ϣ
typedef struct _sJSON_ValueInfo_
{
	// ָ�� �Ƿ��з�����
	BOOL bExpSig;
	// ���� �Ƿ��з�����
	BOOL bBaseSig;
	// ָ��
	unsigned short exp;
	// ����
	unsigned long base;
} sJSON_NumberInfo;
// ��ȡ��ֵ������Ϣ
void JSON_GetNumValueInfo(sJSONContext* pCtx , sJSON_NumberInfo* pValInfo);
// ��ȡ��ֵ
float JSON_GetFloatValue(sJSONContext* pCtx);
// ��ȡ��ֵ��������
signed long JSON_GetIntegerValue(sJSONContext* pCtx);
