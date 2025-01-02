
/*
	格式参考： ECMA-404 RFC7159
	
	功能说明：
	JSON解析需要很小的数据空间：仅需要上下文结构数据，以及单项内容数据最大缓存(单项名称或数据允许最大长度的缓存)
	解析按字节方式进行数据输入，程序按状态机方式进行解析
	处理代码根据解析时返回状态，判断并取数据内容（再根据需要对数据进行存储）
	因此处理代码仍需要使用状态机的方式判断并存储JSON信息，同时也需要对JSON名称及数据内容格式进行校验
	
	宽松的数据解析：
	支持大部分无效的非字节符串类型（简化true,false,null及数字解析处理）
	可以根据 JSON_IsKeywordNumToken 宏确定当前数据是否为关键字或数值类型，并使用 JSON_CurrentTokenType 进一步判断内容是否有效
	
	JSON 动态解析步骤：
	定义解析结构 sJSONContext ctx; 及 解析数据缓存 unsigned char Cache[CacheSize];
	通过调用 JSON_InitContext(&ctx , Cache , sizeof(Cache)); 初始化
	并循环调用 ret = ctx.pParse(&ctx , c); 并判断其返回值确定解析状态
	返回 JSON_PBTM_Parseing 当前字符为非关键字符正在正常处理
	返回包含 JSON_ParseByteType_Error 标记时 状态值低7位为 JSON_ErrT_XXX 错误状态码，可使用 JSON_ParseByteType_IsError 宏进行判断
	返回包含 JSON_PBTM_Token 标记时 ctx.pCache 及 ctx.cachePos 有效，cachePos为内容长度，pCache为内容缓存指针
	非 JSON_PBTM_Parseing 状态时 可通过 ctx.stackPoint 判断当前结构嵌套层数 并可以通过 JSON_CurrentStackIsObject(&ctx) 返回判断当前结构是否为对象表类型
	除处理及错误状态外也可以直接比较状态值进行判断，如： JSON_ParseByteType_ObjectStart , JSON_ParseByteType_NameEnd 等
	
*/

// 解析过程标记
// ParseByteTypeMask
// 当前处理中
#define JSON_PBTM_Parseing 					0x00
// 当前为标记节点
#define JSON_PBTM_Token 					0x01
// 节点结束状态，此状态 cache 数据有效
#define JSON_PBTM_End 						0x02
// 当前为结构节点（对象或数组）
#define JSON_PBTM_Struct 					0x04
// 数据节点为属性名，结构节点为对象
#define JSON_PBTM_NameOrObj 				0x08
// 处理异常
#define JSON_PBTM_Error 					0x80

// 错误内容
// 内部错误
#define JSON_ErrT_InternalError 			0x00
// 堆栈满
#define JSON_ErrT_StackFull 				0x01
// 缓存满
#define JSON_ErrT_CacheFull 				0x02
// 解析规则异常
#define JSON_ErrT_RuleException 			0x03

// 解析过程类型
// 正在处理，且当前非关键字符，不需要进行数据判断
#define JSON_ParseByteType_Parseing 		JSON_PBTM_Parseing
// 当前为对象名称结束，pCache为对象名
#define JSON_ParseByteType_NameEnd 			(JSON_PBTM_Token|JSON_PBTM_NameOrObj|JSON_PBTM_End)
// 当前为数据内容结束，pCache为数据内容
#define JSON_ParseByteType_ValueEnd 		(JSON_PBTM_Token|JSON_PBTM_End)
// 当前为对象结构开始
#define JSON_ParseByteType_ObjectStart 		(JSON_PBTM_Token|JSON_PBTM_Struct|JSON_PBTM_NameOrObj)
// 当前为数组结构开始
#define JSON_ParseByteType_ArrayStart 		(JSON_PBTM_Token|JSON_PBTM_Struct)
// 当前为对象结构结束，pCache为前面一层对象表最后对象的数据内容
#define JSON_ParseByteType_ObjectEnd 		(JSON_PBTM_Token|JSON_PBTM_Struct|JSON_PBTM_NameOrObj|JSON_PBTM_End)
// 当前为数组结构结束，pCache为前面一层数组最后一项的数据内容
#define JSON_ParseByteType_ArrayEnd 		(JSON_PBTM_Token|JSON_PBTM_Struct|JSON_PBTM_End)
// 异常状态标记，不可以直接判断是否相等（返回值还包含错误内容）
#define JSON_ParseByteType_Error 			JSON_PBTM_Error
// 异常状态判断
#define JSON_ParseByteType_IsError(type) 	(((type)&JSON_PBTM_Error) != 0)
// 异常状态判断，type 必须为无符号状态值类型
#define JSON_ParseByteType_IsErrorU(type) 	((type) >= JSON_PBTM_Error)

// 当前缓存数据是否为关键字或数值
#define JSON_IsKeywordNumToken(ctx) 	((ctx).cachePos >= (ctx).cacheLen || (ctx).pCache[(ctx).cachePos] == 0)

// 最大堆栈尺寸（允许对象嵌套深度：尺寸*8）
#ifndef JSON_MaxStackSize
#define JSON_MaxStackSize 		4
#endif

typedef unsigned char (*fJSONContextParse)(void* pCtx , unsigned char c);
// 参数查找结构
typedef struct _sJSONContext_
{
	// 解析状态
	unsigned char type;
	// 数据栈指针位置
	unsigned char stackPoint;
	// 缓存总长度
	unsigned char cacheLen;
	// 缓存使用位置
	unsigned char cachePos;
	// 缓存指针
	unsigned char* pCache;
	// 解析函数指针
	fJSONContextParse pParse;
	// 数据栈 每1位对应一个数据栈类型（0对象，1数组）
	unsigned char stack[JSON_MaxStackSize];
} sJSONContext;

// 初始化解析结构
void JSON_InitContext(sJSONContext* pCtx , unsigned char* pCache , unsigned char cacheLen);

// 当前是否为对象结构中，否则为数组结构中
BOOL JSON_CurrentStackIsObject(sJSONContext* pCtx);

// 数值解析功能  JSON_ValueParse.c
// 标记类型
// 字符串 ""
#define JSON_TokenType_String 			0
// 数字 +1.2e-3
#define JSON_TokenType_Number 			1
// 关键字 true
#define JSON_TokenType_KW_True 			2
// 关键字 false
#define JSON_TokenType_KW_False 		3
// 关键字 null
#define JSON_TokenType_KW_Null 			4
// 未知
#define JSON_TokenType_Unknow 			255
// 获取当前标记类型
unsigned char JSON_CurrentTokenType(sJSONContext* pCtx);

// 数值解析信息
typedef struct _sJSON_ValueInfo_
{
	// 指数 是否有符号数
	BOOL bExpSig;
	// 底数 是否有符号数
	BOOL bBaseSig;
	// 指数
	unsigned short exp;
	// 底数
	unsigned long base;
} sJSON_NumberInfo;
// 获取数值浮点信息
void JSON_GetNumValueInfo(sJSONContext* pCtx , sJSON_NumberInfo* pValInfo);
// 获取数值
float JSON_GetFloatValue(sJSONContext* pCtx);
// 获取数值整数部分
signed long JSON_GetIntegerValue(sJSONContext* pCtx);
