
#include <string.h>
#include <stdio.h>

#define cv_GetTestBit(id) 						(1<<(id))
#define cv_SetBufferBitTrue(buf , id) 			(buf)[(id)>>3] |= cv_GetTestBit((id)&0x07)
#define cv_SetBufferBitFalse(buf , id) 			(buf)[(id)>>3] &= ~(cv_GetTestBit((id)&0x07))
#define cv_BufferBitIsTrue(buf , id) 			(((buf)[(id)>>3]&(cv_GetTestBit((id)&0x07))) != 0)
#define cv_BufferBitIsFalse(buf , id) 			(((buf)[(id)>>3]&(cv_GetTestBit((id)&0x07))) == 0)

/*
#include <math.h>
#define cv_Negative_Float(fVal) 				fVal = -(fVal)
#define cv_pow_b10(e) 							powf(10.0f , (float)e)
*/
#define cv_Negative_Float(fVal) 				((unsigned char*)&fVal)[3] ^= 0x80
#define cv_pow_b10(e)							cv_powfi(10.0f , e)
float cv_powfi(float x , unsigned char n)
{
	unsigned char i;
	float y;
	y = 1.0f;
	i = 1;
	while(1)
	{
		if( (n&i) != 0 )
		{
			y *= x;
			n &= ~i;
		}
		if( n == 0 )
			break;
		x *= x;
		i <<= 1;
	}
	return y;
}

#define BOOL 									unsigned char
#define TRUE 									1
#define FALSE 									0

#include "JSON.h"
#include "JSON.c"
#include "JSON_ValueParse.c"

// context
unsigned char g_uValType;
unsigned char g_uValState;
BOOL (*gpJSONParseFun)(const char type);
sJSONContext g_ctx;
unsigned char g_Cache[200];
// data
unsigned char g_uCmd;
char g_strParam1[64];
signed long g_iParam2;
float g_fParam3;

#define ValType_Non 							0
#define ValType_Command 						1
#define ValType_Param1 							2
#define ValType_Param2 							3
#define ValType_Param3 							4

#define ValState_Command 						0x01
#define ValState_Param1 						0x02
#define ValState_Param2 						0x04
#define ValState_Param3 						0x08


/*
{
	Cmd:"A1" , 
	Param:
	{
		P1:"" ,
		P2:1 ,
		P3:2.2
	}
}
*/
BOOL JsonTest_ParseUnknow(const char type);
BOOL JsonTest_ParseStart(const char type);
BOOL JsonTest_ParseParam(const char type);
BOOL JsonTest_ParseValue(const char type);

BOOL JsonTest_ParseUnknow(const char type)
{
	if( type == JSON_ParseByteType_ObjectStart && g_ctx.stackPoint == (0+1) )
	{
		gpJSONParseFun = JsonTest_ParseStart;
	}
	return TRUE;
}
BOOL JsonTest_ParseStart(const char type)
{
	if( type == JSON_ParseByteType_NameEnd && g_ctx.stackPoint == 1 )
	{
		if( g_ctx.cachePos == 3 && memcmp(g_ctx.pCache , "Cmd" , 3) == 0 )
		{
			g_uValType = ValType_Command;
			gpJSONParseFun = JsonTest_ParseValue;
		}
		if( g_ctx.cachePos == 5 && memcmp(g_ctx.pCache , "Param" , 5) == 0 )
		{
			gpJSONParseFun = JsonTest_ParseParam;
		}
	}
	if( g_ctx.stackPoint == 0 )
	{
		gpJSONParseFun = JsonTest_ParseUnknow;
	}
	return TRUE;
}
BOOL JsonTest_ParseParam(const char type)
{
	if( type == JSON_ParseByteType_NameEnd && g_ctx.stackPoint == 2 )
	{
		if( g_ctx.cachePos == 2 && g_ctx.pCache[0] == 'P' )
		{
			if( g_ctx.pCache[1] == '1' )
			{
				g_uValType = ValType_Param1;
				gpJSONParseFun = JsonTest_ParseValue;
			}
			else if( g_ctx.pCache[1] == '2' )
			{
				g_uValType = ValType_Param2;
				gpJSONParseFun = JsonTest_ParseValue;
			}
			else if( g_ctx.pCache[1] == '3' )
			{
				g_uValType = ValType_Param3;
				gpJSONParseFun = JsonTest_ParseValue;
			}
		}
	}
	return TRUE;
}
BOOL JsonTest_ParseValue(const char type)
{
	unsigned char vt;
	if( type != JSON_ParseByteType_ValueEnd && type != JSON_ParseByteType_ObjectEnd )
	{
		printf("Err: No Value\r\n");
		return FALSE;
	}
	vt = JSON_CurrentTokenType(&g_ctx);
	gpJSONParseFun = JsonTest_ParseParam;
	switch( g_uValType )
	{
	case ValType_Command:
		if( vt == JSON_TokenType_String )
		{
			if( g_ctx.cachePos == 2 && g_ctx.pCache[0] == 'A' && g_ctx.pCache[1] >= '0' && g_ctx.pCache[1] <= '9' )
			{
				g_uValState |= ValState_Command;
				g_uCmd = g_ctx.pCache[1] - '0';
			}
		}
		gpJSONParseFun = JsonTest_ParseStart;
		break;
	case ValType_Param1:
		if( vt == JSON_TokenType_String )
		{
			if( g_ctx.cachePos <= sizeof(g_strParam1) )
			{
				g_uValState |= ValState_Param1;
				memcpy(g_strParam1 , g_ctx.pCache , g_ctx.cachePos);
				if( g_ctx.cachePos < sizeof(g_strParam1) )
				{
					g_strParam1[g_ctx.cachePos] = 0;
				}
				else
				{
					g_strParam1[sizeof(g_strParam1)-1] = 0;
				}
			}
		}
		break;
	case ValType_Param2:
		if( vt == JSON_TokenType_Number )
		{
			g_uValState |= ValState_Param2;
			g_iParam2 = JSON_GetIntegerValue(&g_ctx);
		}
		break;
	case ValType_Param3:
		if( vt == JSON_TokenType_Number )
		{
			g_uValState |= ValState_Param3;
			g_fParam3 = JSON_GetFloatValue(&g_ctx);
		}
		break;
	default:
		printf("Err: Unknow ValType\r\n");
		return FALSE;
	}
	return TRUE;
}
void ParseJson(const char* pJson)
{
	unsigned char type;
	// g_uValType = ValType_Non;
	g_uValState = 0;
	gpJSONParseFun = JsonTest_ParseUnknow;
	JSON_InitContext(&g_ctx , g_Cache , sizeof(g_Cache));
	while( *pJson != 0 )
	{
		type = g_ctx.pParse(&g_ctx , *(pJson++));
		if( type == JSON_ParseByteType_Parseing )
		{
			continue;
		}
		if( JSON_ParseByteType_IsError(type) )
		{
			switch( type&(~JSON_PBTM_Error) )
			{
			case JSON_ErrT_InternalError:
				printf("Err: InternalError\r\n");
				break;
			case JSON_ErrT_StackFull:
				printf("Err: StackFull\r\n");
				break;
			case JSON_ErrT_CacheFull:
				printf("Err: CacheFull\r\n");
				break;
			case JSON_ErrT_RuleException:
				printf("Err: RuleException\r\n");
				break;
			default:
				printf("Err: Unknow Err\r\n");
				break;
			}
			return;
		}
		if( !gpJSONParseFun(type) )
		{
			return;
		}
	}
	if( (g_uValState&ValState_Command) != 0 )
	{
		printf("Cmd: A%d\r\n" , g_uCmd);
	}
	else
	{
		printf("Cmd: Non\r\n");
	}
	if( (g_uValState&ValState_Param1) != 0 )
	{
		printf("P1: %s\r\n" , g_strParam1);
	}
	else
	{
		printf("P1: Non\r\n");
	}
	if( (g_uValState&ValState_Param2) != 0 )
	{
		printf("P2: %d\r\n" , g_iParam2);
	}
	else
	{
		printf("P2: Non\r\n");
	}
	if( (g_uValState&ValState_Param3) != 0 )
	{
		printf("P3: %g\r\n" , g_fParam3);
	}
	else
	{
		printf("P3: Non\r\n");
	}
}
int main()
{
	ParseJson("{\"Cmd\":\"A1\",\"Param\":{\"P1\":\"Test\",\"P2\":123,\"P3\":2.245}}");
}

