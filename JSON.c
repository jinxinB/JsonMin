
// 内部解析状态标记
// 初始状态值
#define JSON_ParseType_InitStart 			0x00
// 是否在处理数据
#define JSON_ParseType_InNameValue 			0x01
// 是否在处理内容
#define JSON_ParseType_IsName 				0x02
// 数据值开始
#define JSON_ParseType_StartValue 			0x04
// 前一项是字符串
#define JSON_ParseType_PreString 			0x08
// 异常标记，包含此标记时为错误状态值，其它标记均无效
#define JSON_ParseType_Error 				0x80
// // 字符串转义序列
// #define JSON_ParseType_EscapeSequence  		0x08
// // 是否在处理数据
// #define JSON_ParseType_InArrayValue 		4

unsigned char JSON_ParseString(sJSONContext* pCtx , unsigned char c);
unsigned char JSON_ParseCommentStart(sJSONContext* pCtx , unsigned char c);
unsigned char JSON_ParseSComment(sJSONContext* pCtx , unsigned char c);
unsigned char JSON_ParseMComment(sJSONContext* pCtx , unsigned char c);
// 当前是否为对象结构中，否则为数组结构中
BOOL JSON_CurrentStackIsObject(sJSONContext* pCtx)
{
	unsigned char pos;
	do
	{
		if( pCtx->stackPoint == 0 )
		{
			break;
		}
		pos = pCtx->stackPoint-1;
		if( cv_BufferBitIsFalse(pCtx->stack , pos) )
		{
			// 对象
			break;
		}
		return FALSE;
	}while(0);
	return TRUE;
}
void JSON_ClearNameValueEndSpace(sJSONContext* pCtx)
{
	unsigned char c;
	while( pCtx->cachePos != 0 ) // 清除后面的空字符
	{
		c = pCtx->pCache[pCtx->cachePos-1];
		if( c == ' ' || c == '\t' || c == '\r' || c == '\n' )
		{
			pCtx->cachePos--;
		}
		else
		{
			break;
		}
	}
}
unsigned char JSON_OnCheckObjArrayStart(sJSONContext* pCtx)
{
	if( pCtx->stackPoint >= (sizeof(pCtx->stack)*8) )
	{
		return JSON_ParseByteType_Error|JSON_ErrT_StackFull;
	}
	if( (pCtx->type&(JSON_ParseType_InNameValue|JSON_ParseType_PreString)) != 0 ) // 名称与起始至少要有冒号分隔
	{
		return JSON_ParseByteType_Error|JSON_ErrT_RuleException;
	}
	return JSON_PBTM_Parseing;
}
BOOL JSON_OnNameValueEnd(sJSONContext* pCtx)
{
	unsigned char type;
	type = pCtx->type;
	pCtx->type = type & (~(JSON_ParseType_InNameValue|JSON_ParseType_PreString|JSON_ParseType_StartValue)); // 清除字符标记
	if( (type&JSON_ParseType_InNameValue) != 0 )
	{
		JSON_ClearNameValueEndSpace(pCtx);
		if( pCtx->cachePos < pCtx->cacheLen )
		{
			pCtx->pCache[pCtx->cachePos] = 0; // 指定当前为名称
		}
	}
	else if( (type&JSON_ParseType_PreString) != 0 )
	{
		if( pCtx->cachePos < pCtx->cacheLen )
		{
			pCtx->pCache[pCtx->cachePos] = '"'; // 指定当前为字符串
		}
	}
	else 
	{
		// 有效内容至少 JSON_ParseType_InNameValue 或 JSON_ParseType_PreString 包含一项
		pCtx->cachePos = 0; // 将内容置空
		pCtx->pCache[0] = 0; // 指定当前为名称
		return FALSE;
	}
	return TRUE;
}
unsigned char JSON_ParseStart(sJSONContext* pCtx , unsigned char c)
{
	if( c == ' ' || c == '\t' || c == '\r' || c == '\n' )
	{
	}
	else if( c == '/' )
	{
		pCtx->pParse = (fJSONContextParse)JSON_ParseCommentStart;
	}
	else if( c == '{' )
	{
		if( (c=JSON_OnCheckObjArrayStart(pCtx)) != JSON_PBTM_Parseing )
		{
			return c;
		}
		pCtx->type &= ~JSON_ParseType_StartValue;
		cv_SetBufferBitFalse(pCtx->stack , pCtx->stackPoint);
		pCtx->stackPoint++;
		return JSON_ParseByteType_ObjectStart;
	}
	else if( c == '[' )
	{
		if( (c=JSON_OnCheckObjArrayStart(pCtx)) != JSON_PBTM_Parseing )
		{
			return c;
		}
		pCtx->type |= JSON_ParseType_StartValue;
		cv_SetBufferBitTrue(pCtx->stack , pCtx->stackPoint);
		pCtx->stackPoint++;
		return JSON_ParseByteType_ArrayStart;
	}
	else if( c == '}' )
	{
		if( pCtx->stackPoint == 0 || !JSON_CurrentStackIsObject(pCtx) )
		{
			return JSON_ParseByteType_Error|JSON_ErrT_RuleException;
		}
		JSON_OnNameValueEnd(pCtx); // 此处之前可能为对象或列表结束状态，不对数据有效性进行判断
		pCtx->stackPoint--;
		return JSON_ParseByteType_ObjectEnd;
	}
	else if( c == ']' )
	{
		if( pCtx->stackPoint == 0 || JSON_CurrentStackIsObject(pCtx) )
		{
			return JSON_ParseByteType_Error|JSON_ErrT_RuleException;
		}
		JSON_OnNameValueEnd(pCtx); // 此处之前可能为对象或列表结束状态，不对数据有效性进行判断
		pCtx->stackPoint--;
		return JSON_ParseByteType_ArrayEnd;
	}
	else if( c == ':' )
	{
		if( !JSON_CurrentStackIsObject(pCtx) || !JSON_OnNameValueEnd(pCtx) )
		{
			return JSON_ParseByteType_Error|JSON_ErrT_RuleException;
		}
		pCtx->type |= JSON_ParseType_StartValue;
		return JSON_ParseByteType_NameEnd;
	}
	else if( c == ',' )
	{
		JSON_OnNameValueEnd(pCtx); // 此处之前可能为对象或列表结束状态，不对数据有效性进行判断
		if( !JSON_CurrentStackIsObject(pCtx) )
		{
			pCtx->type |= JSON_ParseType_StartValue;
		}
		return JSON_ParseByteType_ValueEnd;
	}
	else
	{
		// 字符
		if( (pCtx->type&JSON_ParseType_InNameValue) == 0 )
		{
			// 名称或字符串初始判断当前为名称或数据，用于在对象或数组结束时确定类型
			if( (pCtx->type&JSON_ParseType_StartValue) != 0 )
			{
				// 已经确定当前为数据内容，清除名称标记
				pCtx->type &= ~(JSON_ParseType_StartValue|JSON_ParseType_IsName);
			}
			else
			{
				// 初始内容，根据当前是否为对象来确定当前是否为名称
				if( JSON_CurrentStackIsObject(pCtx) )
				{
					pCtx->type |= JSON_ParseType_IsName;
				}
				else
				{
					pCtx->type &= ~JSON_ParseType_IsName;
				}
			}
			// pCtx->type &= ~JSON_ParseType_PreString; // 清除前面可能的字符串标记，不对 内容重复 进行错误判断
			// pCtx->type |= JSON_ParseType_InNameValue;
			if( c != '"' ) // 首字符串不置名称标记
			{
				pCtx->type |= JSON_ParseType_InNameValue;
			}
			pCtx->cachePos = 0;
		}
		if( (pCtx->type&JSON_ParseType_PreString) != 0 ) // 前面已经有字符串，内容重复
		{
			return JSON_ParseByteType_Error|JSON_ErrT_RuleException;
		}
		if( c == '"' )
		{
			if( (pCtx->type&JSON_ParseType_InNameValue) != 0 ) // 前面已经有内容，内容重复
			{
				return JSON_ParseByteType_Error|JSON_ErrT_RuleException;
			}
			pCtx->cachePos = 0;
			pCtx->pParse = (fJSONContextParse)JSON_ParseString;
			return JSON_ParseByteType_Parseing;
		}
	}
	if( (pCtx->type&JSON_ParseType_InNameValue) != 0 )
	{
		if( pCtx->cachePos >= pCtx->cacheLen )
		{
			return JSON_ParseByteType_Error|JSON_ErrT_CacheFull;
		}
		pCtx->pCache[pCtx->cachePos++] = c;
	}
	return JSON_ParseByteType_Parseing;
}
unsigned char JSON_ParseEscapeSequence(sJSONContext* pCtx , unsigned char c)
{
	unsigned char pos1 , pos2 , len;
	pos1 = pCtx->cachePos;
	len = pCtx->cacheLen;
	if( pos1 >= len )
	{
		return JSON_ParseByteType_Error|JSON_ErrT_CacheFull;
	}
	pos2 = pCtx->pCache[pos1];
	if( pos2 == 0 )
	{
		if( c == 'u' )
		{
			if( (pos1+3) >= len )
			{
				return JSON_ParseByteType_Error|JSON_ErrT_CacheFull;
			}
			pCtx->pCache[pos1] = 1;
			pCtx->pCache[pos1+1] = 0;
		}
		else
		{
			if( c == 'r' )
			{
				pCtx->pCache[pCtx->cachePos++] = '\r';
			}
			else if( c == 'n' )
			{
				pCtx->pCache[pCtx->cachePos++] = '\n';
			}
			else if( c == 't' )
			{
				pCtx->pCache[pCtx->cachePos++] = '\t';
			}
			else if( c == 'b' )
			{
				pCtx->pCache[pCtx->cachePos++] = '\b';
			}
			else if( c == 'f' )
			{
				pCtx->pCache[pCtx->cachePos++] = '\f';
			}
			else // if( c == '\\' || c == '/' || c == '"' )
			{
				pCtx->pCache[pCtx->cachePos++] = c;
			}
			pCtx->pParse = (fJSONContextParse)JSON_ParseString;
		}
		return JSON_ParseByteType_Parseing;
	}
	else if( pos2 == 1 )
	{
		if( (pos1+3) >= len )
		{
			return JSON_ParseByteType_Error|JSON_ErrT_CacheFull;
		}
		if( c >= '0' && c <= '9' )
		{
			c -= '0';
		}
		else if( c >= 'a' && c <= 'f' )
		{
			c -= 'a'-10;
		}
		else if( c >= 'A' && c <= 'F' )
		{
			c -= 'A'-10;
		}
		else
		{
			// pCtx->pCache[pCtx->cachePos++] = c;
			// pCtx->pParse = (fJSONContextParse)JSON_ParseString;
			// return JSON_ParseByteType_Parseing;
			return JSON_ParseByteType_Error|JSON_ErrT_RuleException;
		}
		pos2 = pCtx->pCache[pos1+1];
		pCtx->pCache[pos1+1] = pos2+1;
		if( pos2 == 0 )
		{
			pCtx->pCache[pos1+3] = c<<4;
		}
		else if( pos2 == 1 )
		{
			pCtx->pCache[pos1+3] |= c;
		}
		else if( pos2 == 2 )
		{
			pCtx->pCache[pos1+2] = c<<4;
		}
		else if( pos2 == 3 )
		{
			c |= pCtx->pCache[pos1+2]; // 低位
			if( (pCtx->pCache[pos1]=pCtx->pCache[pos1+3]) == 0 )
			{
				// 单字节字符串格式存储，防止宽字节定义与系统不匹配
				pCtx->pCache[pos1] = c;
				pCtx->cachePos++;
			}
			else
			{
				pCtx->pCache[pos1+1] = c;
				pCtx->cachePos += 2;
			}
			pCtx->pParse = (fJSONContextParse)JSON_ParseString;
		}
		return JSON_ParseByteType_Parseing;
	}
	return JSON_ParseByteType_Error|JSON_ErrT_InternalError;
}
unsigned char JSON_ParseString(sJSONContext* pCtx , unsigned char c)
{
	if( pCtx->cachePos >= pCtx->cacheLen )
	{
		return JSON_ParseByteType_Error|JSON_ErrT_CacheFull;
	}
	if( c == '"' )
	{
		pCtx->type &= ~JSON_ParseType_InNameValue;
		pCtx->type |= JSON_ParseType_PreString;
		pCtx->pParse = (fJSONContextParse)JSON_ParseStart;
	}
	else if( c == '\\' )
	{
		pCtx->pCache[pCtx->cachePos] = 0;
		pCtx->pParse = (fJSONContextParse)JSON_ParseEscapeSequence;
	}
	else
	{
		pCtx->pCache[pCtx->cachePos++] = c;
	}
	return JSON_ParseByteType_Parseing;
}
unsigned char JSON_ParseCommentStart(sJSONContext* pCtx , unsigned char c)
{
	if( c == '/' )
	{
		pCtx->pParse = (fJSONContextParse)JSON_ParseSComment;
	}
	else if( c == '*' )
	{
		pCtx->pParse = (fJSONContextParse)JSON_ParseMComment;
	}
	else
	{
		pCtx->pParse = (fJSONContextParse)JSON_ParseStart;
		return JSON_ParseStart(pCtx , c);
	}
	if( (pCtx->type&JSON_ParseType_InNameValue) != 0 && pCtx->cachePos != 0 )
	{
		pCtx->cachePos--;
	}
	return JSON_ParseByteType_Parseing;
}
unsigned char JSON_ParseSComment(sJSONContext* pCtx , unsigned char c)
{
	if( c == '\r' || c == '\n' )
	{
		pCtx->pParse = (fJSONContextParse)JSON_ParseStart;
	}
	return JSON_ParseByteType_Parseing;
}
unsigned char JSON_ParseMCommentEnd(sJSONContext* pCtx , unsigned char c)
{
	if( c == '/' )
	{
		pCtx->pParse = (fJSONContextParse)JSON_ParseStart;
	}
	else
	{
		pCtx->pParse = (fJSONContextParse)JSON_ParseMComment;
	}
	return JSON_ParseByteType_Parseing;
}
unsigned char JSON_ParseMComment(sJSONContext* pCtx , unsigned char c)
{
	if( c == '*' )
	{
		pCtx->pParse = (fJSONContextParse)JSON_ParseMCommentEnd;
	}
	return JSON_ParseByteType_Parseing;
}
// 初始化解析结构
void JSON_InitContext(sJSONContext* pCtx , unsigned char* pCache , unsigned char cacheLen)
{
	pCtx->type = JSON_ParseType_InitStart;
	pCtx->stackPoint = 0;
	pCtx->cacheLen = cacheLen;
	pCtx->cachePos = 0;
	pCtx->pCache = pCache;
	pCtx->pParse = (fJSONContextParse)JSON_ParseStart;
}
