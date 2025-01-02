
// �ڲ�����״̬���
// ��ʼ״ֵ̬
#define JSON_ParseType_InitStart 			0x00
// �Ƿ��ڴ�������
#define JSON_ParseType_InNameValue 			0x01
// �Ƿ��ڴ�������
#define JSON_ParseType_IsName 				0x02
// ����ֵ��ʼ
#define JSON_ParseType_StartValue 			0x04
// ǰһ�����ַ���
#define JSON_ParseType_PreString 			0x08
// �쳣��ǣ������˱��ʱΪ����״ֵ̬��������Ǿ���Ч
#define JSON_ParseType_Error 				0x80
// // �ַ���ת������
// #define JSON_ParseType_EscapeSequence  		0x08
// // �Ƿ��ڴ�������
// #define JSON_ParseType_InArrayValue 		4

unsigned char JSON_ParseString(sJSONContext* pCtx , unsigned char c);
unsigned char JSON_ParseCommentStart(sJSONContext* pCtx , unsigned char c);
unsigned char JSON_ParseSComment(sJSONContext* pCtx , unsigned char c);
unsigned char JSON_ParseMComment(sJSONContext* pCtx , unsigned char c);
// ��ǰ�Ƿ�Ϊ����ṹ�У�����Ϊ����ṹ��
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
			// ����
			break;
		}
		return FALSE;
	}while(0);
	return TRUE;
}
void JSON_ClearNameValueEndSpace(sJSONContext* pCtx)
{
	unsigned char c;
	while( pCtx->cachePos != 0 ) // �������Ŀ��ַ�
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
	if( (pCtx->type&(JSON_ParseType_InNameValue|JSON_ParseType_PreString)) != 0 ) // ��������ʼ����Ҫ��ð�ŷָ�
	{
		return JSON_ParseByteType_Error|JSON_ErrT_RuleException;
	}
	return JSON_PBTM_Parseing;
}
BOOL JSON_OnNameValueEnd(sJSONContext* pCtx)
{
	unsigned char type;
	type = pCtx->type;
	pCtx->type = type & (~(JSON_ParseType_InNameValue|JSON_ParseType_PreString|JSON_ParseType_StartValue)); // ����ַ����
	if( (type&JSON_ParseType_InNameValue) != 0 )
	{
		JSON_ClearNameValueEndSpace(pCtx);
		if( pCtx->cachePos < pCtx->cacheLen )
		{
			pCtx->pCache[pCtx->cachePos] = 0; // ָ����ǰΪ����
		}
	}
	else if( (type&JSON_ParseType_PreString) != 0 )
	{
		if( pCtx->cachePos < pCtx->cacheLen )
		{
			pCtx->pCache[pCtx->cachePos] = '"'; // ָ����ǰΪ�ַ���
		}
	}
	else 
	{
		// ��Ч�������� JSON_ParseType_InNameValue �� JSON_ParseType_PreString ����һ��
		pCtx->cachePos = 0; // �������ÿ�
		pCtx->pCache[0] = 0; // ָ����ǰΪ����
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
		JSON_OnNameValueEnd(pCtx); // �˴�֮ǰ����Ϊ������б����״̬������������Ч�Խ����ж�
		pCtx->stackPoint--;
		return JSON_ParseByteType_ObjectEnd;
	}
	else if( c == ']' )
	{
		if( pCtx->stackPoint == 0 || JSON_CurrentStackIsObject(pCtx) )
		{
			return JSON_ParseByteType_Error|JSON_ErrT_RuleException;
		}
		JSON_OnNameValueEnd(pCtx); // �˴�֮ǰ����Ϊ������б����״̬������������Ч�Խ����ж�
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
		JSON_OnNameValueEnd(pCtx); // �˴�֮ǰ����Ϊ������б����״̬������������Ч�Խ����ж�
		if( !JSON_CurrentStackIsObject(pCtx) )
		{
			pCtx->type |= JSON_ParseType_StartValue;
		}
		return JSON_ParseByteType_ValueEnd;
	}
	else
	{
		// �ַ�
		if( (pCtx->type&JSON_ParseType_InNameValue) == 0 )
		{
			// ���ƻ��ַ�����ʼ�жϵ�ǰΪ���ƻ����ݣ������ڶ�����������ʱȷ������
			if( (pCtx->type&JSON_ParseType_StartValue) != 0 )
			{
				// �Ѿ�ȷ����ǰΪ�������ݣ�������Ʊ��
				pCtx->type &= ~(JSON_ParseType_StartValue|JSON_ParseType_IsName);
			}
			else
			{
				// ��ʼ���ݣ����ݵ�ǰ�Ƿ�Ϊ������ȷ����ǰ�Ƿ�Ϊ����
				if( JSON_CurrentStackIsObject(pCtx) )
				{
					pCtx->type |= JSON_ParseType_IsName;
				}
				else
				{
					pCtx->type &= ~JSON_ParseType_IsName;
				}
			}
			// pCtx->type &= ~JSON_ParseType_PreString; // ���ǰ����ܵ��ַ�����ǣ����� �����ظ� ���д����ж�
			// pCtx->type |= JSON_ParseType_InNameValue;
			if( c != '"' ) // ���ַ����������Ʊ��
			{
				pCtx->type |= JSON_ParseType_InNameValue;
			}
			pCtx->cachePos = 0;
		}
		if( (pCtx->type&JSON_ParseType_PreString) != 0 ) // ǰ���Ѿ����ַ����������ظ�
		{
			return JSON_ParseByteType_Error|JSON_ErrT_RuleException;
		}
		if( c == '"' )
		{
			if( (pCtx->type&JSON_ParseType_InNameValue) != 0 ) // ǰ���Ѿ������ݣ������ظ�
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
			c |= pCtx->pCache[pos1+2]; // ��λ
			if( (pCtx->pCache[pos1]=pCtx->pCache[pos1+3]) == 0 )
			{
				// ���ֽ��ַ�����ʽ�洢����ֹ���ֽڶ�����ϵͳ��ƥ��
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
// ��ʼ�������ṹ
void JSON_InitContext(sJSONContext* pCtx , unsigned char* pCache , unsigned char cacheLen)
{
	pCtx->type = JSON_ParseType_InitStart;
	pCtx->stackPoint = 0;
	pCtx->cacheLen = cacheLen;
	pCtx->cachePos = 0;
	pCtx->pCache = pCache;
	pCtx->pParse = (fJSONContextParse)JSON_ParseStart;
}
