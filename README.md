# JsonMin

用于8位单片机的C语言JSON解析库，比cJSON更轻量(功能只占用10字节空间，数据由用户按需分配空间)

解析代码相对复杂很多，且关键字及数据最大255的长度限制，所以能编译cJSON的系统就不建议用这个库。

## 编译

JSON.c 和 JSON.h 引入项目中就可以使用
JSON_ValueParse.c 是类型及数值扩展解析代码
库代码中引用的 cv_SetBufferBitTrue ，cv_Negative_Float 和 cv_pow_b10 等外部函数，可以参考 JsonTest.c 里的代码实现

**代码测试：** 直接编译 JsonTest.c 文件

## 说明

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
