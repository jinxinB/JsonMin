
// 获取当前标记类型
unsigned char JSON_CurrentTokenType(sJSONContext* pCtx)
{
	unsigned char i;
	unsigned char c;
	unsigned char state;
	if( pCtx->cachePos == 0 || pCtx->cachePos >= pCtx->cacheLen || pCtx->pCache[pCtx->cachePos] != 0 )
	{
		return JSON_TokenType_String;
	}
	do
	{
		i = 0;
		c = pCtx->pCache[0];
		if( c == '-' )
		{
			if( pCtx->cachePos == 1 )
			{
				break;
			}
			i = 1;
			c = pCtx->pCache[1];
		}
		if( c >= '0' && c <= '9' )
		{
			state = 0;
			while( (++i) < pCtx->cachePos )
			{
				c = pCtx->pCache[i];
				if( c < '0' || c > '9' )
				{
					if( c == '.' )
					{
						if( state != 0 ) // e 后不允许出现小数点
						{
							break;
						}
						if( (++i) >= pCtx->cachePos )
						{
							break;
						}
						c = pCtx->pCache[i];
						if( c < '0' || c > '9' )
						{
							break;
						}
						state = 1;
					}
					else if( c == 'e' || c == 'E' )
					{
						if( state == 2 ) // 不允许出现重复e
						{
							break;
						}
						if( (++i) >= pCtx->cachePos )
						{
							break;
						}
						c = pCtx->pCache[i];
						if( c == '+' || c == '-' )
						{
							if( (++i) >= pCtx->cachePos )
							{
								break;
							}
							c = pCtx->pCache[i];
						}
						if( c < '0' || c > '9' )
						{
							break;
						}
						state = 2;
					}
					else
					{
						break;
					}
				}
			}
			if( i < pCtx->cachePos )
			{
				break;
			}
			return JSON_TokenType_Number;
		}
		if( i != 0 )
		{
			break;
		}
		if( c == 't' )
		{
			if( pCtx->cachePos == 4 && pCtx->pCache[1] == 'r' && pCtx->pCache[2] == 'u' && pCtx->pCache[3] == 'e' )
			{
				return JSON_TokenType_KW_True;
			}
		}
		else if( c == 'f' )
		{
			if( pCtx->cachePos == 5 && pCtx->pCache[1] == 'a' && pCtx->pCache[2] == 'l' && pCtx->pCache[3] == 's' && pCtx->pCache[4] == 'e' )
			{
				return JSON_TokenType_KW_False;
			}
		}
		else if( c == 'n' )
		{
			if( pCtx->cachePos == 4 && pCtx->pCache[1] == 'u' && pCtx->pCache[2] == 'l' && pCtx->pCache[3] == 'l' )
			{
				return JSON_TokenType_KW_Null;
			}
		}
	}while(0);
	return JSON_TokenType_Unknow;
}
// 获取数值浮点信息
void JSON_GetNumValueInfo(sJSONContext* pCtx , sJSON_NumberInfo* pValInfo)
{
	unsigned char i;
	unsigned char c;
	unsigned char divCnt;
	unsigned char mulCnt;
	unsigned char dataCnt;
	BOOL bDot;
	BOOL bExp;
	do
	{
		if( pCtx->cachePos == 0 )
		{
			break;
		}
		c = pCtx->pCache[0];
		if( c == '-' )
		{
			if( pCtx->cachePos == 1 )
			{
				break;
			}
			i = 1;
			c = pCtx->pCache[1];
			pValInfo->bBaseSig = TRUE;
		}
		else
		{
			i = 0;
			pValInfo->bBaseSig = FALSE;
		}
		bDot = FALSE;
		bExp = FALSE;
		divCnt = 0;
		mulCnt = 0;
		dataCnt = 0;
		pValInfo->bExpSig = FALSE;
		pValInfo->exp = 0;
		pValInfo->base = 0;
		while( 1 )
		{
			if( bExp )
			{
				if( c >= '0' && c <= '9' )
				{
					if( dataCnt < 5 )
					{
						pValInfo->exp = (pValInfo->exp*10) + (c-'0');
						if( pValInfo->exp != 0 )
						{
							dataCnt++;
						}
					}
					else
					{
						pValInfo->exp = 65535;
					}
				}
				else
				{
					break;
				}
			}
			else
			{
				if( c >= '0' && c <= '9' )
				{
					if( dataCnt < 10 )
					{
						pValInfo->base = (pValInfo->base*10) + (c-'0');
						if( bDot )
						{
							divCnt++;
						}
						dataCnt++;
					}
					else
					{
						if( !bDot )
						{
							mulCnt++;
						}
					}
				}
				else if( c == '.' )
				{
					if( bDot )
					{
						break;
					}
					bDot = TRUE;
				}
				else if( c == 'e' || c == 'e' )
				{
					if( (i+1) >= pCtx->cachePos )
					{
						break;
					}
					dataCnt = 0;
					c = pCtx->pCache[i+1];
					if( c == '-' )
					{
						pValInfo->bExpSig = TRUE;
					}
					else if( c >= '0' && c <= '9' )
					{
						pValInfo->exp = c - '0';
						dataCnt++;
					}
					else if( c != '+' )
					{
						break;
					}
					i++;
					bExp = TRUE;
				}
				else
				{
					break;
				}
			}
			if( (++i) >= pCtx->cachePos )
			{
				break;
			}
			c = pCtx->pCache[i];
		}
		if( i < pCtx->cachePos )
		{
			break;
		}
		if( mulCnt != 0 )
		{
			if( pValInfo->bExpSig )
			{
				if( pValInfo->exp > (unsigned short)mulCnt )
				{
					pValInfo->exp -= (unsigned short)mulCnt;
				}
				else
				{
					pValInfo->bExpSig = FALSE;
					pValInfo->exp = (unsigned short)mulCnt - pValInfo->exp;
				}
			}
			else
			{
				pValInfo->exp += mulCnt;
			}
		}
		if( divCnt != 0 )
		{
			if( pValInfo->bExpSig )
			{
				pValInfo->exp += divCnt;
			}
			else
			{
				if( pValInfo->exp >= (unsigned short)divCnt )
				{
					pValInfo->exp -= (unsigned short)divCnt;
				}
				else
				{
					pValInfo->bExpSig = TRUE;
					pValInfo->exp = (unsigned short)divCnt - pValInfo->exp;
				}
			}
		}
		return;
	}while(0);
	memset(pValInfo , 0 , sizeof(sJSON_NumberInfo));
	
}
// 获取数值
float JSON_GetFloatValue(sJSONContext* pCtx)
{
	float fVal;
	sJSON_NumberInfo ValInfo;
	JSON_GetNumValueInfo(pCtx , &ValInfo);
	if( ValInfo.exp != 0 )
	{
		if( ValInfo.bExpSig )
		{
			if( ValInfo.exp >= 45 )
			{
				return 0.0f;
			}
			fVal = ((float)ValInfo.base) * cv_pow_b10(-(signed char)ValInfo.exp);
		}
		else
		{
			if( ValInfo.exp >= 38 )
			{
				// 溢出返回有效值
				*(unsigned long*)&fVal = 0x7F7FFFFF;
				if( ValInfo.bBaseSig )
				{
					cv_Negative_Float(fVal);
				}
				return fVal;
			}
			fVal = ((float)ValInfo.base) * cv_pow_b10((signed char)ValInfo.exp);
		}
		if( ValInfo.bBaseSig )
		{
			cv_Negative_Float(fVal);
		}
		return fVal;
	}
	return (float)ValInfo.base;
}
// 获取数值整数部分
signed long JSON_GetIntegerValue(sJSONContext* pCtx)
{
	signed char exp10;
	float fVal;
	sJSON_NumberInfo ValInfo;
	JSON_GetNumValueInfo(pCtx , &ValInfo);
	do
	{
		if( ValInfo.exp != 0 )
		{
			if( ValInfo.exp > 10 )
			{
				if( ValInfo.bExpSig )
				{
					return 0;
				}
				break; // 超出整数极值
			}
			fVal = (float)ValInfo.base;
			exp10 = (signed char)ValInfo.exp;
			if( ValInfo.bExpSig )
			{
				exp10 = -exp10;
			}
			fVal *= cv_pow_b10(exp10);
			if( fVal >= 2147483648.0f )
			{
				break; // 超出整数极值
			}
			ValInfo.base = (unsigned long)fVal;
		}
		if( ValInfo.base >= 0x80000000 )
		{
			break;
		}
		if( ValInfo.bBaseSig )
		{
			return -((signed long)ValInfo.base);
		}
		else
		{
			return (signed long)ValInfo.base;
		}
	}while(0);
	if( ValInfo.bBaseSig )
	{
		return 0x80000000;
	}
	else
	{
		return 0x7FFFFFFF;
	}
}
