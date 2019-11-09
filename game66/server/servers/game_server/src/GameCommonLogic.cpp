#include <algorithm>
#include "GameCommonLogic.h"

vector<vector<uint8> > g_vvAreaIndexLists; // 原区域的所有排列索引
// 添加原区域的排列索引(递归函数) 
void GameCommonLogic::AddAreaIndexLists(vector<uint8> vAreaIndexs, uint32 count) {
	if (vAreaIndexs.size() == count) {
		g_vvAreaIndexLists.push_back(vAreaIndexs);
		return;
	}
	for (uint32 i = 0; i < count; ++i) {
		if (std::count(vAreaIndexs.begin(), vAreaIndexs.end(), i) == 0) {
			vAreaIndexs.push_back(i);
			AddAreaIndexLists(vAreaIndexs, count);
			vAreaIndexs.pop_back();
		}
	}
}





















