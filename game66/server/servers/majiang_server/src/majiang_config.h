//
// Created by toney on 2017/6/13.
//

#ifndef SERVER_MAJIANG_CONFIG_H
#define SERVER_MAJIANG_CONFIG_H

#include "svrlib.h"
#include "majiang_logic.h"
#include "json/json.h"

using namespace game_majiang;

class CMaJiangCfg
{
public:
    CMaJiangCfg();
    ~CMaJiangCfg();

public:
    bool InitConfig(uint16 play);

    uint16 playType();
    //是否支持开杠
    bool supportOpenGang();
    //是否需要听牌
    bool supportTing();
    //是否支持吃
    bool supportEat();
	bool supportEatWind();
	bool supportEatWord();

    //是否支持通炮
    bool supportTongPao();
    //是否支持海底牌
    bool supportTail();
    //是否需要将牌
    bool isNeedLeader258();
    //是否支持抢杠
    bool supportQiangGang();
    //是否支持红中牌
    bool hasHongZhongCard();
    //是否支持赖子
    bool supportSuperCard();
    //支持7小对
    bool supportSevenPair();
    //支持3豪华七小对
    bool supportSevenPair3();
    //支持七连对
    bool supportSevenPair7();

    //是否庄闲算分
    bool useMasterPoint();
    //是否起手胡
    bool supportOpeningHu();
    //起手胡算分
    uint32 openingHuFen();
    //抓鸟算分
    uint32 hitBirdFen();
    //抓鸟计分类型
    uint8   hitBirdCalcType();

    //杠是否及时算分，也即跟最后是否流局无关（目前只有转转麻将是这样的，长沙麻将和安康都是最后胡牌才算杠分）
    bool supportImmediaGangPoint();
    //杠牌是否算分
    bool supportGangPoint();
    //只允许自摸
    bool onlyZimo();
    //平胡不接炮
    bool onlyChiDaHu();

    /**
     * 是否允许补杠
     * 假设当前即可杠也可碰，如果玩家选择了碰而不是杠的话：
     * 1 允许补杠的规则：后续随时可以杠这个牌
     * 2 不允许补杠的规则：这局后续再也不能杠这个牌
     * 新归纳：不支持补杠的情况下，只能杠新牌，或杠别人，或暗杠，不能拿手牌杠（除非暗杠）
     */
    bool supportExtraGang();
    //支持十三幺
    bool supportThirteen();
    //支持将将胡
    bool supportJiangJiangHu();

    //扎鸟数
    uint16  GetBirdCount();
    //开杠补张的牌数量
    uint16  GetOpenGangCards();
    //支持起手胡类型
    bool    IsOpenHuType(BYTE huType);
    //听牌后是否能杠
    bool    IsCanGangAfterListen();
    //获取接受的胡牌类型,胡牌权位
    void    ShowHuType(stActionOper& oper);
    uint32  ShowHuRight();

    //无红中加码数
    uint8   NoHongZhongAddBird();
    //一码全中
    bool    IsOneBirdAll();

protected:
    uint16  m_playType;           // 玩法(长沙,转转)
    uint16  m_birdCount;          // 扎鸟数量
    bool    m_masterPoint;        // 庄闲分
    uint16  m_openGangCards;      // 开杠补张的牌数量
    bool    m_openingHu;          // 起牌胡

    bool    m_onlySelfHu;         // 只允许自摸胡
    bool    m_onlyChiDaHu;        // 只吃大胡
    bool    m_canEat;             // 能否吃牌
	bool    m_canEatWind;         // 能否吃风牌
	bool    m_canEatWord;         // 能否吃字牌

    bool    m_qiangGang;          // 可抢杠胡
    bool    m_hongZhongCard;      // 是否有红中牌
    bool    m_superCard;          // 癞子
    bool    m_sevenPair;          // 是否可以胡七对
    bool    m_hongZhongBao;       // 红中宝牌(固定红中作为宝牌)
    bool    m_thirteenYao;        // 十三幺
    bool    m_jiangjiangHu;       // 将将胡
    stActionOper   m_openHuType;         // 支持的起手胡类型

    uint8   m_noHongZhongAddBird; // 无红中加码数
    bool    m_oneBirdAll;         // 一码全中
    uint8   m_hitBirdCalcType;    // 中鸟计分方式

};













































#endif //SERVER_MAJIANG_CONFIG_H
