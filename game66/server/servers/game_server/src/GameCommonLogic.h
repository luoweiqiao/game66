// 游戏逻辑公共函数类
#ifndef __GAME_COMMON_LOGIC_H__
#define __GAME_COMMON_LOGIC_H__

#include <vector>
#include "svrlib.h" // for uint8

class GameCommonLogic
{
public:
	// 添加原区域的排列索引(递归函数) 
	// vAreaIndexs : 要添加元素的数组
	// count : 数组允许保存的最大元素个数
	static void AddAreaIndexLists(vector<uint8> vAreaIndexs, uint32 count);
};

#endif // __GAME_COMMON_LOGIC_H__
