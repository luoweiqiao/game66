//
// Created by toney on 2017/6/13.
//

#include "majiang_config.h"

CMaJiangCfg::CMaJiangCfg()
{
    m_playType    = MAJIANG_TYPE_CHANGSHA;
    m_birdCount   = 2;
    m_masterPoint = true;
    m_openGangCards = 2;

    m_openingHu   = true;
    m_canEat      = true;
	m_canEatWind  = false;
	m_canEatWord = false;
    m_onlySelfHu  = false;
    m_qiangGang   = true;
    m_hongZhongCard = false;
    m_superCard   = false;
    m_sevenPair   = true;
    m_hongZhongBao = true;
    m_thirteenYao = false;
    m_jiangjiangHu = false;
    m_openHuType.reset();
    m_onlyChiDaHu  = false;

    m_noHongZhongAddBird = 0; // 无红中加码数
    m_oneBirdAll   = false;   // 一码全中
    m_hitBirdCalcType = 0;  // 中鸟计分方式

}
CMaJiangCfg::~CMaJiangCfg()
{

}

bool CMaJiangCfg::InitConfig(uint16 play)
{
    m_playType = play;
    Json::Reader reader;
    Json::Value  jvalue;

    // 扎鸟数
    m_birdCount = 6;
    // 庄闲算分
    m_masterPoint = true;
    m_hitBirdCalcType = 0;

    if(m_playType == MAJIANG_TYPE_TWO_PEOPLE)//
    {
        m_openingHu = false;
        m_openGangCards = 1;
        m_onlySelfHu = false;
        m_qiangGang = true;
		//m_superCard = m_hongZhongCard = true;
        m_sevenPair = true;
        m_canEat = true;
		m_canEatWind = false;
		m_canEatWord = false;
		m_birdCount = 0;
    }else if(m_playType == MAJIANG_TYPE_CHANGSHA){//长沙
        m_superCard = m_hongZhongBao = false;
        m_jiangjiangHu = true;
        m_openHuType.add(MINHU_LIULIU_SHUN);
        m_openHuType.add(MINHU_QUE_YI_SE);
        m_openHuType.add(MINHU_BANBAN_HU);
        m_openHuType.add(MINHU_SI_XI);
        m_openHuType.add(MINHU_JIE_JIE_GAO);
        m_openHuType.add(MINHU_SAN_TONG);
        m_openHuType.add(MINHU_YIZHIHUA);

    }else if(m_playType == MAJIANG_TYPE_NINGXIANG){//宁乡
        m_superCard = m_hongZhongBao = false;
        m_jiangjiangHu = true;

        m_openHuType.add(MINHU_SI_XI);
        m_openHuType.add(MINHU_BANBAN_HU);
        m_openHuType.add(MINHU_LIULIU_SHUN);
        m_openHuType.add(MINHU_JIE_JIE_GAO);
        m_openHuType.add(MINHU_YIZHIHUA);
        m_openHuType.add(MINHU_SAN_TONG);
        m_openHuType.add(MINHU_YIDIANHONG);
        m_openHuType.add(MINHU_JINTONG_YUNV);
        m_openHuType.add(MINHU_QUE_YI_SE);

        m_onlyChiDaHu = jvalue.get("phbjp",0).asUInt();

    }else if(m_playType == MAJIANG_TYPE_HONGZHONG){//红中
        m_openingHu = false;
        m_openGangCards = 0;
        m_onlySelfHu = true;
        m_qiangGang = true;
        m_superCard = m_hongZhongCard = true;
        m_sevenPair = true;
        m_noHongZhongAddBird = 0;
        m_oneBirdAll = 0;
        m_canEat = false;
    }else if(m_playType == MAJIANG_TYPE_GUOBIAO){//国标
        m_openingHu = false;
        m_openGangCards = 0;
        m_onlySelfHu = false;
        m_qiangGang = true;
        m_superCard = m_hongZhongCard = true;
        m_sevenPair = true;
    }
    else{
        LOG_DEBUG("非法麻将类型:%d",play);
        return false;
    }

    return true;
}
uint16 CMaJiangCfg::playType(){
    return m_playType;
}
//是否支持开杠
bool CMaJiangCfg::supportOpenGang()
{
    if(m_playType==MAJIANG_TYPE_CHANGSHA || m_playType==MAJIANG_TYPE_NINGXIANG){
        return m_openGangCards>0;
    }
    return false;
}

//是否需要听牌
bool CMaJiangCfg::supportTing()
{
	if (m_playType == MAJIANG_TYPE_TWO_PEOPLE)
	{
		return true;
	}
    if(m_playType==MAJIANG_TYPE_CHANGSHA || m_playType==MAJIANG_TYPE_NINGXIANG){
        return m_openGangCards>0;
    }
    return false;
}

//是否支持吃
bool CMaJiangCfg::supportEat()
{
    return m_canEat;
}
bool CMaJiangCfg::supportEatWind()
{
	return m_canEatWind;
}
bool CMaJiangCfg::supportEatWord()
{
	return m_canEatWord;
}
//是否支持通炮
bool CMaJiangCfg::supportTongPao()
{

    return true;
}

//是否支持海底牌
bool CMaJiangCfg::supportTail()
{
    if(m_playType==MAJIANG_TYPE_CHANGSHA || m_playType==MAJIANG_TYPE_NINGXIANG)return true;

    return false;
}

//是否需要将牌
bool CMaJiangCfg::isNeedLeader258()
{
	//if (m_playType == MAJIANG_TYPE_CHANGSHA || m_playType == MAJIANG_TYPE_NINGXIANG)
	//{
	//	return true;
	//}

    return false;
}

//是否支持抢杠
bool CMaJiangCfg::supportQiangGang(){
    return m_qiangGang;
}

//是否支持红中牌
bool CMaJiangCfg::hasHongZhongCard(){
    return m_hongZhongCard;
}

//是否支持赖子
bool CMaJiangCfg::supportSuperCard(){
    return m_superCard;
}

//支持7小对
bool CMaJiangCfg::supportSevenPair() {
    return m_sevenPair;
}
//支持3豪华七小对
bool CMaJiangCfg::supportSevenPair3()
{
	if (m_playType == MAJIANG_TYPE_NINGXIANG)
	{
		return true;
	}

    return false;
}
//支持七连对
bool CMaJiangCfg::supportSevenPair7()
{
	if (m_playType == MAJIANG_TYPE_GUOBIAO || m_playType == MAJIANG_TYPE_TWO_PEOPLE)
	{
		return true;
	}

    return false;
}

//是否庄闲算分
bool CMaJiangCfg::useMasterPoint(){
    return m_masterPoint;
}

//是否起手胡
bool CMaJiangCfg::supportOpeningHu(){
    return m_openingHu;
}
//起手胡算分
uint32 CMaJiangCfg::openingHuFen()
{
    if(m_playType == MAJIANG_TYPE_CHANGSHA)return 2;
    if(m_playType == MAJIANG_TYPE_NINGXIANG)return 4;

    return 0;
}
//抓鸟算分
uint32 CMaJiangCfg::hitBirdFen()
{


    return 1;
}
//抓鸟翻倍
uint8  CMaJiangCfg::hitBirdCalcType()
{
    return m_hitBirdCalcType;
}
//杠是否及时算分，也即跟最后是否流局无关（目前只有转转麻将是这样的，长沙麻将和安康都是最后胡牌才算杠分）
bool CMaJiangCfg::supportImmediaGangPoint()
{
    switch(m_playType)
    {
    //case MAJIANG_TYPE_TWO_PEOPLE:
    case MAJIANG_TYPE_HONGZHONG:
        return true;
    default:
        return false;
    }
}
//杠牌是否算分
bool CMaJiangCfg::supportGangPoint()
{
    switch(m_playType)
    {
    case MAJIANG_TYPE_TWO_PEOPLE:
    case MAJIANG_TYPE_HONGZHONG:
        return true;
    default:
        return false;
    }
	return false;
}
//只允许自摸
bool CMaJiangCfg::onlyZimo(){
    return m_onlySelfHu;
}
//平胡不接炮
bool CMaJiangCfg::onlyChiDaHu(){
    return m_onlyChiDaHu;
}


/**
 * 是否允许补杠
 * 假设当前即可杠也可碰，如果玩家选择了碰而不是杠的话：
 * 1 允许补杠的规则：后续随时可以杠这个牌
 * 2 不允许补杠的规则：这局后续再也不能杠这个牌
 * 新归纳：不支持补杠的情况下，只能杠新牌，或杠别人，或暗杠，不能拿手牌杠（除非暗杠）
 */
bool CMaJiangCfg::supportExtraGang() {
    return m_playType==MAJIANG_TYPE_CHANGSHA;
}
//支持十三幺
bool CMaJiangCfg::supportThirteen(){
    return m_thirteenYao;
}
//支持将将胡
bool CMaJiangCfg::supportJiangJiangHu(){
    return m_jiangjiangHu;
}

//扎鸟数
uint16  CMaJiangCfg::GetBirdCount(){
    return m_birdCount;
}
//开杠补张的牌数量
uint16  CMaJiangCfg::GetOpenGangCards(){
    return m_openGangCards;
}
//支持起手胡类型
bool    CMaJiangCfg::IsOpenHuType(BYTE huType)
{
    return m_openHuType.isExist(huType);
}
//听牌后是否能杠
bool    CMaJiangCfg::IsCanGangAfterListen()
{
    if(m_playType == MAJIANG_TYPE_CHANGSHA || m_playType == MAJIANG_TYPE_NINGXIANG )
        return false;

    return true;
}
//获取接受的胡牌类型,胡牌权位
void    CMaJiangCfg::ShowHuType(stActionOper& oper)
{
    if(m_playType == MAJIANG_TYPE_TWO_PEOPLE || m_playType == MAJIANG_TYPE_HONGZHONG)
    {
        oper.reset();
        oper.add(HUTYPE_JI_HU);
    }
}
uint32  CMaJiangCfg::ShowHuRight()
{
    uint32 flag=0xFFFFFFFF;
    if(m_playType == MAJIANG_TYPE_TWO_PEOPLE || m_playType == MAJIANG_TYPE_HONGZHONG)
    {
       flag=0;
       flag |= CHR_ZI_MO;
    }
    return flag;
}
//无红中加码数
uint8   CMaJiangCfg::NoHongZhongAddBird()
{
    return m_noHongZhongAddBird;
}
//一码全中
bool    CMaJiangCfg::IsOneBirdAll()
{
    return m_oneBirdAll;
}













































