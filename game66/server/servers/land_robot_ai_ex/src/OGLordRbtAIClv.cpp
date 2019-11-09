//#include <windows.h>
#include <vector>
//#include <TCHAR.H>
#include <string.h>
#include <iterator>

#include "OGLordRbtAIClv.h"
#define TRUE true
#define FALSE false
using namespace std;
#define NEW_AI_START_LEVEL 1

int cacl_control_poker_num(GAMEEx *game)
{
    int ctrl_num = CONTROL_POKER_NUM;
    POKERS all;
    ::add_poker((POKERS*)game->self_suit->h, (POKERS*)game->self_suit->opps, &all);
    if (all.total <= 20)
        ctrl_num = 4;
    else if (all.total <= 10)
        ctrl_num = 2;
    return ctrl_num;
}

// ========================================================
// 新机器人接口 start

bool COGLordRbtAIClv::RbtInInitCard(int argSeat, std::vector<int> argHandCard)
{
    if(argSeat > 2 || argSeat < 0)
    {
        m_nLastErrorCode = RBT_ERR_ERRSEAT;
        return FALSE;
    }
    for(int i = 0 ; i < argHandCard.size() ; i++)
    {
        if(argHandCard[i] < 0 || argHandCard[i] > 53)
        {
            m_nLastErrorCode = RBT_ERR_SETCARD_HAND;
            return FALSE;
        }
    }
    ResetData();
    SetSeat(argSeat);
    send_poker(argHandCard);

    for (int k=0; k<17; k++)
    {
        robot->card[0][k]= argHandCard[k];
    }
    lord_score = -1;    
    aiSeat = argSeat;
    // 与NEW_AI_START_LEVEL冲突，注释后走非NEW_AI_START_LEVEL逻辑
    //if (level <= 1)
    //{
    //    initCard(robot, robot->card[0], NULL, NULL);
    //}
    return TRUE;
}

#define FIX_TIME 30
#define DEFAULT_TIME 100
bool COGLordRbtAIClv::RbtInSetTimeDelay(int timeDelay)
{
	if(timeDelay<0)
	    timeDelay = 0;
    int timeGap = DEFAULT_TIME - timeDelay;
	if (timeGap < 30) timeGap = 30;
    robot->game.delayPerGame = (timeGap -30)*100 / (DEFAULT_TIME - FIX_TIME);
    return true;
}

bool COGLordRbtAIClv::RbtInSetLevel(int argLevel)
{
    level = argLevel;
    robot->game.known_others_poker = level>=2;
    robot->game.use_best_for_undefined_case = level>=2;
    robot->game.search_level = level>=3?1:0;

    //robot->game.level = level;

    return true;
}

bool COGLordRbtAIClv::RbtInNtfCardInfo( std::vector<std::vector<int>> argHandCard)
{
    int upPlayer=(aiSeat+2)%3;
    for (int k=0; k<17; k++)
    {
        robot->card[1][k]= argHandCard[upPlayer][k];
    }
    int downPlayer=(aiSeat+1)%3;
    for (int k=0; k<17; k++)
    {
        robot->card[2][k]= argHandCard[downPlayer][k];
    }
            
    //注意玩家位置
    initCard(robot,robot->card[0],robot->card[1],robot->card[2]);
    return true;
}

// 
bool COGLordRbtAIClv::RbtInCallScore(int argSeat, int argCallScore)
{
    if(argSeat > 2 || argSeat < 0)
    {
        m_nLastErrorCode = RBT_ERR_ERRSEAT;
        return FALSE;
    }
	maxScore = max_ddz(maxScore, argCallScore);
    return TRUE;
}

// 请求叫分策略
bool COGLordRbtAIClv::RbtOutGetCallScore(int &argCallScore, int & delay)
{
    argCallScore = 0;

   if(level >= NEW_AI_START_LEVEL)
   {
    lord_score=RobotshowCard(robot);
    if((lord_score&0xf)>=2)
        argCallScore=lord_score;
    else if((lord_score&0xf)<1)
        argCallScore =0;
    else
        //argCallScore = rand()%2?0:( ((rand()%100)>10)?3:(rand()%3)); //模拟人不理智的情况？
        argCallScore = rand() % 2 ? 0 : (((rand() % 100)>60) ? 1 : (rand() % 3));

   }

   if (level < NEW_AI_START_LEVEL){
       if (argCallScore == 3 || IsCallLord()){
           delay = 1000 + rand() % 1000;
           argCallScore = 3;
       }
       else{
           delay = 500 + rand() % 500;
           //argCallScore = 0;
       }
   }

   if (argCallScore <= maxScore)
   {
       argCallScore = 0;
   }

    return TRUE;
}

// 输入玩家明牌信息 
bool COGLordRbtAIClv::RbtInShowCard( int argShowSeat,                    // 明牌玩家座位
                                std::vector<int> argHandCard        // 玩家手牌
                                ){
                                return true;
}

// 输出明牌策略 
bool COGLordRbtAIClv::RbtOutShowCard(bool &showCard, int &delay){
return false;
}                // 是否明牌 true:明牌 false:不明牌



// 收到确定地主信息
bool COGLordRbtAIClv::RbtInSetLord(int argLordSeat, std::vector<int> argReceiceCard)
{
     if(level>=NEW_AI_START_LEVEL)
     {
//       int  argLordSeat =argLordSeat;
//       CHECK_ARGS( argLordSeat >=0 && argLordSeat <=3);

      int pot[10];
      for (int k=0; k<3; k++)
          pot[k]=argReceiceCard[k];
      
      if(argLordSeat == aiSeat)
          beLord(robot,pot,2);
      if(argLordSeat==((aiSeat+1)%3))
          beLord(robot,pot,0);
      if(argLordSeat==((aiSeat+2)%3))
          beLord(robot,pot,1);
     }
   


    if(argLordSeat > 2 || argLordSeat < 0)
    {
        m_nLastErrorCode = RBT_ERR_ERRSEAT;
        return FALSE;
    }
    for(int i = 0 ; i < argReceiceCard.size() ; i++)
    {
        if(argReceiceCard[i] < 0 || argReceiceCard[i] > 53)
        {
            m_nLastErrorCode = RBT_ERR_TAKEOUT_CARD;
            return FALSE;
        }
    }

    SetLord(argLordSeat);
    SetCurPlayer(argLordSeat);
    if (m_nSeat == argLordSeat)
    {// 这里机器人逻辑好像有问题,只是给地主发送了手牌信息并添加,没有找到通知两个农民底牌内容的函数. note
        send_pot( argReceiceCard);
    }

    return TRUE;
}

// 通知机器人其他玩家出牌
bool COGLordRbtAIClv::RbtInTakeOutCard(int argSeat, vector<int> argCards)
{
    // 校验数据
    if(argSeat > 2 || argSeat < 0)
    {
        m_nLastErrorCode = RBT_ERR_ERRSEAT;
        return FALSE;
    }
    for(int i = 0 ; i < argCards.size() ; i++)
    {
        if(argCards[i] < 0 || argCards[i] > 53)
        {
            m_nLastErrorCode = RBT_ERR_TAKEOUT_CARD;
            return FALSE;
        }
    }
    
    OnMesTakeOutCard(argCards,argSeat);
    SetCurPlayer( (argSeat+1)%3 );


     if(level >= NEW_AI_START_LEVEL)
    {
     
          int out1[26],out2[5];
          int i;
          for(i=0;i<argCards.size();++i)
              out1[i]=argCards[i];
          
          out1[argCards.size()]=-1;
          out2[0] = -1; //laizi
                           
          if (argSeat == aiSeat)
          {
              getPlayerTakeOutCards(robot, out1, out2, CUR_PLAYER);
          }
          if(argSeat==((aiSeat+1)%3))
            getPlayerTakeOutCards(robot,out1,out2,0);  // 0:down player
          if(argSeat==((aiSeat+2)%3))
            getPlayerTakeOutCards(robot,out1,out2,1);  // 1:up player          
    }


    return TRUE;
}

// 向机器人请求出牌策略
bool COGLordRbtAIClv::RbtOutGetTakeOutCard(vector<int> &argCards, int & delay_time)
{

if(level >= NEW_AI_START_LEVEL)
{
       int out1[26],out2[5];
       int pass1=takeOut(robot,out1,out2, &delay_time);
       int i,pos=0;

    PRINTF(0,"==============ROBOT takeout: ");

    for ( i=0; i<26; ++i)
    {    
         if(out1[i]!=-1)
        {
         //   gsWriteLog(L"%c",poker_to_char(int_2_poker(out1[i])));
         }
         else
         {
              PRINTF(0,", ");
                break;
         }
    }
       
    //gsWriteLog("\nRbtOutGetTakeOutCard robot%d vecCards: ",aiSeat );
    for ( i=0; i<26; ++i)
    {    
         if(out1[i]!=-1)
        {
              print_one_poker(int_2_poker(out1[i]));
                 argCards.push_back(out1[i]);
         }
         else
         {
                //vecCards.push_back(out1[i]);
                PRINTF(0,"-1");
                break;
         }
       }
    
    //delay_time = 1+ rand()% 4; //todo update

    return true;
}



    argCards.clear();

    takeout_poker(argCards);
    
    //delay_time = 1+ rand()% 4; //todo update

    return TRUE;
}

// 重置机器人时设置机器人的座位和地主的座位
bool COGLordRbtAIClv::RbtInSetSeat(int argMySeat, int argLordSeat)
{
    // 校验数据
    if(argMySeat > 2 || argMySeat < 0 || argLordSeat > 2 || argLordSeat < 0)
    {
        m_nLastErrorCode = RBT_ERR_ERRSEAT;
        return FALSE;
    }

    SetLord(argLordSeat);
    SetSeat(argMySeat);

    return TRUE;
}

// 重置机器人时设置手牌和底牌
bool COGLordRbtAIClv::RbtInSetCard(vector<int> argInitCard, vector<int> argReceiveCard)
{
    // 校验数据
    for(int i = 0 ; i < argInitCard.size(); i++)
    {
        if (argInitCard[i] < 0 || argInitCard[i] > 53)
        {
            m_nLastErrorCode = RBT_ERR_SETCARD_HAND;
            return FALSE;
        }
    }
    for (int i = 0 ; i < argReceiveCard.size(); i++)
    {
        if (argReceiveCard[i] < 0 || argReceiveCard[i] > 53 )
        {
            m_nLastErrorCode = RBT_ERR_SETCARD_RECEIVE;
            return FALSE;
        }
    }

    send_poker(argInitCard);
    send_pot(argReceiveCard);

    return FALSE;
}

bool COGLordRbtAIClv::RbtInTakeOutRecord(vector<int> argTakeOutSeat ,vector<vector<int>> argTakeOutRecord)
{
    // 校验数据
    int nSize = argTakeOutSeat.size();
    if (nSize!=argTakeOutRecord.size())
    {
        m_nLastErrorCode = RBT_ERR_RECORED_UNMATCH;
        return FALSE;
    }
    for(int i=0 ;i< nSize ; i++)
    {
        if (argTakeOutSeat[i] > 2 || argTakeOutSeat[i] < 0 )
        {
            m_nLastErrorCode = RBT_ERR_RECORED_SEAT;
            return FALSE;
        }
        vector<int> tempCards = (argTakeOutRecord[i]);
        for (int j = 0 ; j < tempCards.size() ; i++)
        {
            if (tempCards[j] < 0 || tempCards[j] > 53)
            {
                m_nLastErrorCode = RBT_ERR_RECORED_CARD;
                return FALSE;
            }
        }
    }

    return FALSE;
}

bool COGLordRbtAIClv::RbtResetData()
{
    //从构造函数里
    m_nSeat = 0;
    m_bRobot = true;

    good_farmer_0 = 60;
    good_farmer_1 = 30;

    game = &m_game;
    ResetData();

    //{
    robot = &m_robot;
    // char filename[100];
    // sprintf(filename,"robot_%d.txt",robot);
    robot->logfile = NULL;
    debug_level = -1;
    robot->game.search_level = 0;
    robot->game.pointer_init = 0;
    init_robot_setting(robot);
    robot->game.hun = -1; //not laizi
    aiSeat = 0;
    level = 0;

    maxScore = 0;
    // robot->logfile = fopen(filename,"wb");  
    //}
    
    //ResetData();
    return TRUE;
}

// string COGLordRbtAIClv::RbtOutGetLastError()
// {
//     return "";
// }

// 新机器人接口 end
// ========================================================
// 旧版机器人客户端AI逻辑代码 start


COGLordRbtAIClv::COGLordRbtAIClv(void)
{
    m_nSeat = 0;
    m_bRobot = true;

    good_farmer_0 = 60;
    good_farmer_1 = 30;

    game = &m_game;
    ResetData();

//{
   robot=&m_robot;
  // char filename[100];
  // sprintf(filename,"robot_%d.txt",robot);
   robot->logfile = NULL;
   debug_level = -1;
   robot->game.search_level = 0;
   robot->game.pointer_init = 0;
   init_robot_setting(robot);
   robot->game.hun = -1; //not laizi
   aiSeat=0;   
   level=0;

   maxScore = 0;
  // robot->logfile = fopen(filename,"wb");  
//}
}

COGLordRbtAIClv::~COGLordRbtAIClv(void)
{
}

void COGLordRbtAIClv::ResetData( void )
{
    m_nLastPlayerNum = -1 ;
    for( int i= 0;i < 3;i++ )
        m_nRemainNum[i] = 17;

    memset(game, 0,sizeof(GAMEEx));

    game->suits.h = &game->p;
    game->suits.opps =&game->opp;
    game->suits.combos =&game->combos[0];
    game->suits.summary =&game->combos_summary;
    game->suits.oppDown_num = 17;
    game->suits.oppUp_num = 17;
    game->suits.oppSingleKing = false;
    game->suits.kingOut = false;
    game->suits.farmer_only_2 = false;
    game->suits.likeType[0] = 0;
    game->suits.likeType[1] = 0;
    game->suits.likeType[2] = 0;
    game->firstPlayer = -1;
    game->playerCounter = 0;

    game->self_suit=&game->suits;

    full_poker(&game->all);

    vector <int>::iterator Iter;

    for( Iter = m_handCard.begin(); Iter != m_handCard.end(); )
    {
        Iter = m_handCard.erase(Iter);
    }
    m_handCard.clear();
} 

void COGLordRbtAIClv::SetSuit( SUITS * suit)
{
    game->self_suit = suit;
    game->self_suit->lower_control_poker=
        game->lowest_bigPoker = get_lowest_controls(&game->all, cacl_control_poker_num(game));

    sort_poker(game->self_suit->h);

    sub_poker(&game->all,game->self_suit->h,game->self_suit->opps);
} 

void COGLordRbtAIClv::SetLord( int nNum )
{
    m_nLastPlayerNum = nNum;
    m_nRemainNum[nNum] = 20;
    m_nLord = nNum;

    game->cur_playernum = nNum;

    if ( m_nSeat == m_nLord )
    {
		game->self_suit->id = LORD_old;
        game->bLord = true;
    }
    else
    {
        int nUpdown = m_nSeat+1;
        nUpdown = nUpdown < 3 ? nUpdown : 0;
        if ( nNum == nUpdown )  // up 
        {
			game->self_suit->id = UPFARMER_old;

        }
        else // down
        {
			game->self_suit->id = DOWNFARMER_old;
        }
    }
    game->lord=nNum+1;
}

//说明当前活动用户
void COGLordRbtAIClv::SetCurPlayer( int nNum )
{
    game->cur_playernum = nNum ;
}

void COGLordRbtAIClv::SetSeat(int nSeat)
{
    // 校验???
    if (nSeat > 3 || nSeat < 0)
    {
        return;
    }
    m_nSeat = nSeat;
}

GAMEEx * COGLordRbtAIClv::GetGame( void )
{
    return game;
}

SUITS * COGLordRbtAIClv::GetSuit( void )
{
    return game->self_suit;
}

//changed By LiXiang
bool COGLordRbtAIClv::IsCallLord( void )
{
    bool bRet = false;
    int control= calc_controls(game->self_suit->h,game->self_suit->opps, 6 );
    COMBOS_SUMMARY *s = game->self_suit->summary;
    for ( int i=0;i< s->bomb_num;i++)
    {
        if ( s->bomb[i]->low < game->self_suit->lower_control_poker ) 
        {
            control += 10;
        }
    }
    POKER * h = game->self_suit->h;
    if ( control <= 10  ) 
    {
        /*手牌有大王（或小王）和一个2 且 手牌套数>3*/
        if((h->hands[LIT_JOKER]==1|| h->hands[BIG_JOKER]==1) 
            && h->hands[P2]==1 
            && ( s->real_total_num - s->biggest_num + s->combo_with_2_controls )>3)
        {
            game->lord = m_nSeat + 1;
            bRet = true;
        }
    }
    else
    {
        /*手牌有大王（或小王）和两个2且无炸弹 且 手牌套数>3*/
        if((h->hands[LIT_JOKER]==1|| h->hands[BIG_JOKER]==1) && h->hands[P2]==2 && s->bomb_num==0)
        {
            if(s->real_total_num - s->biggest_num + s->combo_with_2_controls>3)
            {
                game->lord = m_nSeat + 1;
                bRet = true;
            }
        }
        else 
        {
            game->lord = m_nSeat + 1;
            bRet = true;
        }
    }
    return bRet;
}
//去掉本手自己出的牌，不需要转换牌值
void COGLordRbtAIClv::RemoveCard( vector <int> &   curCard  )
{
    int nWhat = 0;
    for (int i = 0; i < curCard.size(); i++)//更新手中牌
    {
        for (int j = 0; j < m_handCard.size(); j++)
        {
            if ( m_handCard[j] == curCard[i])
            {
                m_handCard.erase(m_handCard.begin()+j);
                break;
            }
        }
    }
}

void COGLordRbtAIClv::OnTakeOut( int nPlayerNum ,COMBOHAND * cur )
{
    //喜好牌 
    if(nPlayerNum == game->firstPlayer)
    {
        game->playerCounter++;
    }
    else
    {
        game->playerCounter = 1;
        game->firstPlayer = nPlayerNum;
    }
    if(game->playerCounter > 3)
    {

        if((game->pre.type == SINGLE && game->pre.low < Pq)
            ||(game->pre.type != PAIRS && game->pre.low < P8))
        {
            game->self_suit->likeType[nPlayerNum] = game->pre.type;
        }
        else
        {
            game->self_suit->likeType[nPlayerNum] = 0;
        }
    }

    m_nLastPlayerNum = nPlayerNum;
    if ( m_nSeat == nPlayerNum )
    {
        remove_combo_poker(game->self_suit->h,cur,NULL);
        //if (!m_bRobot)
        search_combos_in_suit(game->self_suit->h,game->self_suit->opps,game->self_suit);
    }
    else
    {
        remove_combo_poker(game->self_suit->opps, cur,NULL);
    }

    memcpy(&game->prepre,&game->pre,sizeof(COMBOHAND));
    memcpy(&game->pre,cur,sizeof(COMBOHAND));
    game->prepre_playernum = game->pre_playernum;
    game->pre_playernum = nPlayerNum;

    m_nRemainNum[nPlayerNum]  -= get_combo_number( cur );

    remove_combo_poker(&game->all, cur,NULL);
    game->self_suit->lower_control_poker=
        game->lowest_bigPoker = get_lowest_controls(&game->all, cacl_control_poker_num(game));

    //changed by fsy 确定地主报双中是否有王，逻辑为不拆火箭
    if(cur->low == LIT_JOKER || cur->low == BIG_JOKER)
    {

        if(!game->self_suit->oppSingleKing){
            if(game->all.hands[LIT_JOKER]>0 || game->all.hands[BIG_JOKER]>0){

                if(game->self_suit->h->hands[LIT_JOKER] == 0 
                    && game->self_suit->h->hands[ BIG_JOKER]==0 ) //外面有王,手中无王
                {
                    game->self_suit->oppSingleKing = true;
                    game->self_suit->kingOut = true;
                }    
            }
        }else {
            game->self_suit->oppSingleKing = false;
        }
    }

    if ( m_nSeat != nPlayerNum )
    {
        int nUpdown = m_nSeat+1;
        nUpdown = nUpdown < 3 ? nUpdown : 0;
        if ( nPlayerNum == nUpdown )  //down
        {
            game->self_suit->oppDown_num = m_nRemainNum[nPlayerNum]; //game->player_suit[nPlayerNum]->h->total;
        }
        else //up
        {
            game->self_suit->oppUp_num =  m_nRemainNum[nPlayerNum] ;// game->player_suit[nPlayerNum]->h->total;
        }
    }

    if ( m_bRobot)
    {
        game->self_suit->max_control.single = calc_controls(game->self_suit->h, game->self_suit->opps, cacl_control_poker_num(game));
        update_summary(game->self_suit->summary,game->self_suit->h,
            game->self_suit->opps,game->self_suit->combos,15,
            game->self_suit->oppDown_num,game->self_suit->oppUp_num,game->lowest_bigPoker);
    }

    if ( m_nSeat == nPlayerNum )
    {
        if (check_poker_suit(game->self_suit->h,game->self_suit)==false)
        {
            search_combos_in_suit(game->self_suit->h,game->self_suit->opps,
                game->self_suit);
        }
    }
}

//select a combo and save it to cur
int COGLordRbtAIClv::select_a_combo(  COMBOHAND * cur)
{
    POKER * h = game->self_suit->h;
    POKER * opps = game->self_suit->opps ;
    COMBOHAND* pre = &game->pre ;

    bool first = ( m_nLastPlayerNum == m_nSeat ) ? true : false;

    int pass = 1;
    memset(cur,0,sizeof(COMBOHAND));

    if (  m_bRobot ) 
    {
        if ( game->bLord )//is lord
        {
            if (first) //zhudong chupai
            {
                lord_select_a_combo(game->self_suit,cur);
                pass=0;
            }
            else  //beidong chupai
            {
                pass = lord_select_a_combo_2(game->self_suit,cur,pre);
            }

        }
        else// farmer's trun
        {
            SUITS * suit= game->self_suit;
            COMBOS_SUMMARY * s= suit->summary;
			int isup = game->self_suit->id == UPFARMER_old;
            int is_first_half_game = is_game_first_half(m_nSeat);
            int is_good_farmer = ( ((s->combo_total_num -  s->extra_bomb_ctrl)*10 - suit->max_control.single)) < (is_first_half_game? good_farmer_0: good_farmer_1);
            if ( m_nRemainNum[m_nLord] ==1 )
            {            //refine for lords only has one poker..
                if (s->singles_num <= 1 || s->singles[1]->low >= suit->opps->end)
                    is_good_farmer = 1;
                else {
                    search_combos_in_suits_for_min_single_farmer(suit->h,suit->combos,suit); //should done in other place, since you should know it..
                    if (s->singles_num <= 1 || s->singles[1]->low >= suit->opps->end)
                        is_good_farmer = 1;
                    else
                        is_good_farmer = 0;
                }
            }

            is_good_farmer =
                suit->good_framer = is_good_farmer;
            if (first) //zhudong chupai
            {
                if (isup )
                {
                    upfarmer_play_first(cur,is_good_farmer,suit);
                }
                else  // down farmer
                {
                    downfarmer_play_first(cur,is_good_farmer,suit);
                }
                pass=0;
            }
            else  //beidong chupai
            {
                if ( m_nRemainNum[m_nLord] ==1 )
                {
                    pass = farmer_play_when_lord_has_1_poker_only(cur,pre,suit,isup,is_good_farmer);                    
                }//pre player is lord.)
                //if( isup ) //upfarmer
                else if( m_nRemainNum[m_nLord] ==2 )
                {
                    pass = farmer_play_when_lord_has_2_poker_only(cur,pre,suit,isup,is_good_farmer);
                }
                if(pass)
                {
                    if (is_good_farmer) //战斗农民
                    {
                        /*
                        上家非单牌，必管
                        下家飞机、连对、或8张以上顺子，必管
                        */
                        if ( (game->pre_playernum == m_nLord ) //pre player is lord.
                            &&
                            ( (pre->type >= 3311) //飞机
                            ||(pre->type == PAIRS_SERIES)
                            ||(pre->type == SINGLE_SERIES && pre->len>8 )
                            ||(pre->type == PAIRS && pre->low == Pa)  // X=AA
							|| (game->self_suit->id == UPFARMER_old
                            && pre->type != SINGLE )
                            ) //lord has only one poker

                            )
                        {//必管..
                            pass = must_play_bigger(cur, suit,pre,isup,is_good_farmer,1);
                        }
                        else if ((game->pre_playernum == m_nLord ) //pre player is lord.
                            && (pre->type == SINGLE && pre->low>=P2 && pre->low<BIG_JOKER))
                        {
                            pass=must_play_for_poker2(cur, suit,pre,1);
                            if ( pass ==1)
                                pass=lord_select_a_combo_2(game->self_suit,cur,pre);
                        }
                        else if ( game->pre_playernum != m_nLord )
                            pass = farmer_select_after_farmer(game->self_suit,cur,pre);
                        else if ( game->pre_playernum == m_nLord && isup )
                        {//上家战斗农民，进入地主冲锋农民必管逻辑
                            pass = must_play_bigger(cur, suit,pre,isup,is_good_farmer,1);
                        }
                        else
                        {
                            pass = lord_select_a_combo_2(game->self_suit,cur,pre);
                        }
                    }            //good farmer
                    else if (isup) //up bad farmer
                    {
#pragma region up bad farmer
                        if ( game->pre_playernum == m_nLord //pre player is lord.
                            &&
                            ( pre->type!=SINGLE )    )
                        {
                            pass = must_play_bigger(cur, suit,pre,isup,is_good_farmer,1);
                        }
                        else if ( 1//(game->pre_playernum == m_nLord ) //pre player is lord.
                            && (pre->type == SINGLE ))
                        {
                            int geying = char_to_poker('J');
                            int geying2 = char_to_poker('7');
                            if ( ( is_game_first_half(m_nSeat)
                                && pre->low < geying && pre->low < game->lowest_bigPoker ) || ( pre->low < geying2 ) )
                            {
                                //int geying = game->first_half? char_to_poker('Q'): char_to_poker('T');
                                pass=1;
                                for (int i=char_to_poker('A'); i>=geying; i-- )   //i)    寻找>X的格应区的单牌，如果有，则打出最大的
                                {
                                    if ( suit->h->hands[i]==1)
                                    {
                                        cur->type=SINGLE;
                                        cur->low =i;
                                        cur->len = 1;
                                        suit->h->hands[i]=0;
                                        suit->need_research = 1;
                                        search_combos_in_suit(suit->h,suit->opps,suit);
                                        suit->h->hands[i]++;
                                        pass = 0;
                                        break;
                                    }
                                }
                                if (pass == 1)
                                {
                                    for (int i=char_to_poker('A'); i>=geying; i-- ) //寻找>X的格应区的对子，拆最大的一个，否则
                                    {
                                        if ( suit->h->hands[i]==2)
                                        {
                                            cur->type=SINGLE;
                                            cur->low =i;
                                            cur->len = 1;
                                            suit->h->hands[i]=1;
                                            suit->need_research = 1;
                                            search_combos_in_suit(suit->h,suit->opps,suit);
                                            suit->h->hands[i]++;
                                            pass = 0;
                                            break;
                                        }
                                    }
                                }

                                //    寻找>X的其他的非大小王的单牌，如果有，则打出最大的（含大牌控制）
                                if (pass==1)// && (game->pre_playernum == m_nLord )) //only for preplayer is lord..
                                {
                                    if ( suit->h->hands[P2]==1)
                                    {
                                        cur->type=SINGLE;
                                        cur->low =P2;
                                        cur->len = 1;
                                        suit->h->hands[P2]--;
                                        search_combos_in_suit(suit->h,suit->opps,suit);
                                        suit->h->hands[P2]++;
                                        suit->need_research = 1;
                                        pass = 0;

                                        //zhp maybe need to add break; 2011/3/31
                                    }
                                }

                                if (pass==1)
                                {//rarely here..
                                    for (int i=geying; i>=pre->low+1; i-- )
                                    {
                                        if ( suit->h->hands[i]==1)
                                        {
                                            cur->type=SINGLE;
                                            cur->low =i;
                                            suit->h->hands[i]--;
                                            cur->len = 1;
                                            search_combos_in_suit(suit->h,suit->opps,suit);
                                            suit->need_research =1 ;
                                            suit->h->hands[i]++;
                                            pass = 0;
                                            break;
                                        }
                                    }
                                }

                            }
                            else if ((game->pre_playernum == m_nLord )&& pre->low==P2  )
                            {
                                pass = must_play_for_poker2(cur, suit,pre,1);
                            }
                            else if ((game->pre_playernum == m_nLord )&& pre->low >=geying && pre->low <= Pa )
                            {
                                //寻找最小的能管住X的牌。如果不存在或为大小王，则过牌，否则打出
                                pass = 1;
                                for (int i=pre->low+1; i<=P2; i++ )
                                {
                                    if ( suit->h->hands[i]>=1)
                                    {
                                        cur->type=SINGLE;
                                        cur->low =i;
                                        cur->len = 1;
                                        suit->h->hands[i]--;
                                        search_combos_in_suit(suit->h,suit->opps,suit);
                                        suit->need_research =1 ;
                                        suit->h->hands[i]++;
                                        pass = 0;
                                        break;
                                    }
                                }
                            }
                            else  if (game->pre_playernum == m_nLord )
                                pass = must_play_bigger(cur, suit,pre,isup,is_good_farmer,1);
                            else
                                pass = 1;
                        }    //geying
                        else  if (game->pre_playernum == m_nLord )
                            pass = must_play_bigger(cur, suit,pre,isup,is_good_farmer,1);
                        else //preplayer is the other farmer..
                        {
                            pass = 1;// lord_select_a_combo_2(game->player_suit[m_nSeat],cur,pre,game);
                        }
#pragma endregion up bad farmer
                    }
                    else //bad down farmer
                    {
                        /*地主出的三张以上套牌，如果能管，必管。
                        地主出的2必用小王管（如果没有小王了，必用大王管）；地主出的小王必用大王管。
                        地主出的单/双牌，上半场如果不顺则不管，下半场必管
                        除此之外同地主逻辑*/

                        if ( (game->pre_playernum == m_nLord ) //pre player is lord.
                            && ( get_combo_number(pre)>=3 ) )
                        {//必管..
                            pass=must_play_bigger(cur,suit,pre,isup,is_good_farmer,1);
                        }
                        else if ((game->pre_playernum == m_nLord ) //pre player is lord.
                            && (pre->type == SINGLE && pre->low>=P2 && pre->low<BIG_JOKER))
                        {
                            pass=must_play_for_poker2(cur,suit,pre,0);
                        }
                        else if ((game->pre_playernum == m_nLord ) //pre player is lord. //pre's type is single or pair
                            )
                        {
                            if (!is_first_half_game)
                                pass=must_play_bigger(cur,suit,pre,isup,is_good_farmer,1);
                            else // same as lord..
                                pass=farmer_shunpai(suit,cur,pre);
                        }
                        else //from the other farmer.
                            pass = 1;
                    }
                }
            }
        }
    }
    else //players' turn
    {
#pragma region players
        char buf[100];
        POKER tmp;
        COMBOHAND *combo,hand;
        combo=&hand;
        while (1) {

            memset(&tmp,0,sizeof(POKER));
            read_poker(buf,&tmp);
            sort_poker(&tmp);
            if (tmp.total==0)
            {
                if (first) {
                    continue;
                }
                pass =1;
                break;
            }
            if (is_sub_poker(&tmp,h))
            {
                if ( is_combo(&tmp,combo))
                {
                    if ( first || check_combo_a_Big_than_b(combo,pre) )
                    {
                        pass=0;
                        memcpy(cur,combo,sizeof(COMBOHAND));// is_combo
                        break;
                    }
                    else
                    {
                        continue;
                    }
                }
                else
                {
                    continue;
                }
            }
            else
            {
                continue;
            }
        }
#pragma endregion players
    }

    if (pass)
    {
    }
    else {
        if (cur->type==NOTHING_old) {
            return -1;
        }
    }
    return pass;
}

int COGLordRbtAIClv::is_game_first_half(int player)
{
    for ( int i=0; i < 3 ;i++ )
    {
        if ( m_nLord == i )
        {
            if ( m_nRemainNum[i] <=10 )
                return 0;
        }
        else if ( m_nRemainNum[i] < 8 )
            return 0;
    }
    return 1;
}

//返回1表示pass
int COGLordRbtAIClv::lord_select_a_combo_2(SUITS * suit ,COMBOHAND* cur, COMBOHAND* pre)//select a combo from suit and save to cur.
{
    int mustPlay = 0;
    int pass = 1;

    COMBOS_SUMMARY *s =suit->summary;
    COMBOHAND *c;
    int current_control = calc_controls(suit->h, suit->opps, cacl_control_poker_num(game)); //move calculate to other place
    suit->max_control.single = current_control;
    current_control += suit->summary->extra_bomb_ctrl*10;
    int current_combo_num = suit->summary->combo_total_num;

    //lord , 地主被动出牌
#pragma region lord 
    if ( game->bLord ) 
    {
        if (m_nRemainNum[game->pre_playernum] == 1  //last play farmer has 1 only
            || (m_nRemainNum[(game->pre_playernum+1)%3] == 1 ) // the down player of last play farmer has 1 only
            || ( pre->type == SINGLE && m_nRemainNum[(game->pre_playernum+2)%3] == 1)
            // the Up player of last play farmer has 1 only  and current is a single.
            )
        {  // lord must play this turn if he can
            mustPlay = 1;
            if (pre->type == SINGLE )
            {
                if (s->singles_num >= 2 )
                {

                    //在当前组合中，如果有两套或以上单牌，则从第二小的开始，找出比pre大的单牌cur，
                    for (int k=1; k<s->singles_num; k++)
                    {
                        if (s->singles[k]->low>pre->low) {
                            memcpy(cur,s->singles[k],sizeof(COMBOHAND));
                            s->singles[k]->type = NOTHING_old;
                            remove_combo_in_suit(s,cur);
                            return 0;
                        }
                    }
                    goto __SEARCH_BOMB_FOR_SINGLE;
                }
                else //play the biggest
                {
                    //如果仅剩的一张单牌绝对大，则打出
                    if (s->singles_num == 1
                        && s->singles[0]->low>pre->low
                        && s->singles[0]->low >= suit->opps->end )
                    {
                        memcpy(cur,s->singles[0],sizeof(COMBOHAND));
                        s->singles[0]->type = NOTHING_old;
                        remove_combo_in_suit(s,cur);
                        return 0;
                    }
__SEARCH_BOMB_FOR_SINGLE:
                    //如果有炸弹，且出完炸弹合手，则出炸弹
                    //changed By fsy 加入危险期
                    if (s->bomb_num >0 &&
                        (suit->max_control.single + (s->extra_bomb_ctrl-1-s->combo_total_num)*10)> -20
                        && get_2nd_min_singles(suit->summary)>=suit->opps->end //有炸弹，且合手
                        && isStrongGood(suit) && !isDangous(suit)) //强合手，且不在危险期 
                    {
                        memcpy(cur,s->bomb[0],sizeof(COMBOHAND));
                        s->bomb[0]->type = NOTHING_old;
                        remove_combo_in_suit(s,cur);
                        return 0;
                    }
                    //出最大的一张牌（可以从套里拆出）
					for (int k = max_ddz(suit->opps->end, pre->low + 1); k <= suit->h->end; k++)
                    {
                        if ( suit->h->hands[k]>0 )
                        {
                            cur->type=SINGLE;
                            cur->low = k;
                            cur->len = 1;
                            //research needed.
                            suit->need_research = 1;
                            POKER t;
                            memcpy(&t,suit->h,sizeof(POKER));
                            remove_combo_poker(&t,cur,NULL);
                            search_combos_in_suit(&t,suit->opps,suit);
                            return 0;
                        }
                    }

					for (int k = max_ddz(0, pre->low + 1); k <= suit->h->end; k++)
                    {
                        if ( suit->h->hands[k]>0 )
                        {
                            cur->type=SINGLE;
                            cur->low = k;
                            cur->len = 1;
                            //research needed.
                            suit->need_research = 1;
                            POKER t;
                            memcpy(&t,suit->h,sizeof(POKER));
                            remove_combo_poker(&t,cur,NULL);
                            search_combos_in_suit(&t,suit->opps,suit);
                            return 0;
                        }
                    }
                }
            }
        }
        //农民报双
        if (m_nRemainNum[game->pre_playernum] == 2  //last play farmer has 1 only
            || (m_nRemainNum[(game->pre_playernum+1)%3] == 2 ) // the down player of last play farmer has 1 only
            || ( pre->type == SINGLE && m_nRemainNum[(game->pre_playernum+2)%3] == 2)
            // the Up player of last play farmer has 1 only  and current is a single.
            )
        {
            pass = lord_play_when_famer_has_2_poker_only( cur, pre, suit);
            if(!pass) return 0;
        }
    }
#pragma endregion lord 

    //顺牌
#pragma region shunpai
    COMBOHAND tmp,*p =&tmp;
    memcpy(p,pre,sizeof(COMBOHAND)); //todo: optimize this
    while ( (c =find_combo(s,p))!=NULL)
    {
        int max_poker_in_c = -1;
        if ( (c->type==SINGLE_SERIES || c->type==PAIRS_SERIES || c->type >=3311 ||
            ((max_poker_in_c=has_control_poker(c,game->lowest_bigPoker))<0))
            && c->control != 1)
        {//如果该牌型不包含当前大牌控制或者当前牌组是顺子,飞机，双顺，且自身不为控制 则出牌
            remove_combo_in_suit(s,c);
            memcpy(cur,c,sizeof(COMBOHAND));
            c->type = NOTHING_old;
            return 0;
        }
        else
        {
            /*******************************
            d)    如果打出A后，出完控制数量>出完套数-2，则出牌A，称为“管牌”。否则，
            e)    如果打出A后，出完控制数>=当前控制数，那么出牌A。否则，
            f)    如果打出A后，出完控制数>=3，那么出牌A。
            g)    令X=A，跳回到顺牌逻辑3) -b)
            *********************************/
            // calculate control..
            POKER  t,opps;
            memcpy(&t,suit->h,sizeof(POKER));
            memcpy(&opps,suit->opps,sizeof(POKER));
            remove_combo_poker(&t,c,NULL);
            //remove one/two poker bigger than max poker in c in opps.
            assume_remove_controls_in_opps(&opps, c,max_poker_in_c);
            int new_control = calc_controls(&t, &opps, cacl_control_poker_num(game));
            add_poker(&t,&opps,&t);
            int new_lower = get_lowest_controls(&t, cacl_control_poker_num(game));
            int baktype= c->type;
            c->type=NOTHING_old;
            update_summary(suit->summary,suit->h,suit->opps,suit->combos,
                20,suit->oppDown_num,suit->oppUp_num,new_lower);
            int combo_num_after = suit->summary->combo_total_num;//  - (c->control ==0);
            new_control+=s->extra_bomb_ctrl*10;

            c->type= baktype;
            update_summary(suit->summary,suit->h,suit->opps,suit->combos,
                20,suit->oppDown_num,suit->oppUp_num,suit->lower_control_poker);

            if ( (new_control > current_control)
                || ( new_control >(combo_num_after-2)*10 )
                ||(new_control>=30)
                )
            {
                remove_combo_in_suit(s,c);
                memcpy(cur,c,sizeof(COMBOHAND));
                c->type = NOTHING_old;
                return 0;
            }
            else
            {
                memcpy(p,c,sizeof(COMBOHAND));
                continue;
            }
        }
    }
#pragma endregion shunpai
    //Chai Pai
#pragma region chaipai
    {
        COMBOHAND tmp1;
        c= &tmp1;
        memcpy(p,pre,sizeof(COMBOHAND));
        int first_time =1;
        int nBigger = 0;
        int nExtraCombo = 0;
        //to be optimized by using new search methods..
        while ( find_a_bigger_combo_in_hands(suit,suit->h,c,p)) //could not handle corner case for  BOMB_old in two combos.
        {
            SUITS suit1;
            POKER h,opps;
            COMBOHAND combo[20]={20*0};
            COMBOS_SUMMARY sum={0},*s;
            suit1.combos=&combo[0];
            suit1.h = &h;
            suit1.opps = &opps;
            suit1.summary =s = &sum;
            suit1.lower_control_poker = suit->lower_control_poker;
            suit1.oppDown_num = suit->oppDown_num;
            suit1.oppUp_num = suit->oppUp_num;
            memcpy(&h,suit->h,sizeof(POKER));

            memcpy(&opps,suit->opps,sizeof(POKER));
            remove_combo_poker(suit1.h,c,NULL);

            search_combos_in_suit(suit1.h,suit1.opps,&suit1);

            bool  bBigger = FALSE;

            if (!search_1_2_for_a_three(p, suit1.h, &suit1, c)) //处理三带一
            {
                p->low = c->low;
                continue;
            }
            else
            {
                if (suit1.need_research)
                    search_combos_in_suit(suit1.h,suit1.opps,&suit1);
            }

            int max_poker_in_c = has_control_poker(c, game->lowest_bigPoker);
            if ( max_poker_in_c < 0 || c->type==SINGLE_SERIES || c->type==PAIRS_SERIES || c->type >=3311)  //不包含大牌控制
            {
                /*
                1)    如果打出(A)，系统判断剩余的牌最优组合后的套数，如果套数比之前的套数-1，则直接打出，
                2)    如果套数和以前一样但打出的一套是冲锋套，则直接打出
                */
                if ( suit1.summary->combo_total_num <  suit->summary->combo_total_num
                    ||(suit1.summary->combo_total_num ==  suit->summary->combo_total_num
                    && is_combo_biggest(suit->opps,c,suit->oppDown_num,suit->oppUp_num,suit->lower_control_poker) )
                    )
                {
                    {
                        memcpy(suit->combos , suit1.combos ,  20* sizeof(COMBOHAND)); //todo refine;
                        memset(suit->summary, 0, sizeof(COMBOS_SUMMARY));
                        sort_all_combos(suit->combos,20,suit->summary,suit->opps,suit->oppDown_num
                            ,suit->oppUp_num,suit->lower_control_poker); //to remove this..
                        memcpy(cur,c,sizeof(COMBOHAND));
                    }

                    return 0;
                }
                else
                {
                    p->low=c->low;
                    continue;
                }
            }
            else
            {   //包含大牌控制
                assume_remove_controls_in_opps(&opps, c,max_poker_in_c);

                int new_control = calc_controls(suit1.h, suit1.opps, cacl_control_poker_num(game));
                suit1.max_control.single = new_control;

                //update the lowest control poker and update the summary
                POKER t;
                add_poker(suit1.h,suit1.opps,&t);
                int new_lower = get_lowest_controls(&t, cacl_control_poker_num(game));
                update_summary(suit1.summary,suit1.h,suit1.opps,suit1.combos,
                    20,suit1.oppDown_num,suit1.oppUp_num,new_lower);

                if ( is_combo_biggest(suit->opps,c,suit->oppDown_num,suit->oppUp_num,suit->lower_control_poker) )
                {
                    //suit1.summary->nOffsetBigger = 3;
                    suit1.summary->nOffsetComboNum = 0;
                    bBigger = true;
                }
                else
                {
                    //suit1.summary->nOffsetBigger = 0;
                    suit1.summary->nOffsetComboNum = 10;
                    bBigger = FALSE;
                }

                if (  CONTROL_SUB_COMBO_NUM_EX(&suit1)  > -20
                    ||  CONTROL_SUB_COMBO_NUM_EX(&suit1) >= CONTROL_SUB_COMBO_NUM_EX(suit)
                    ||  CONTROL_NUM(&suit1)  >=30
                    /*    || ( CONTROL_SUB_COMBO_NUM(&suit1)+10 >= CONTROL_SUB_COMBO_NUM(suit)
                    &&  is_combo_biggest(suit->opps,c,suit->oppDown_num,suit->oppUp_num,suit->lower_control_poker))*/
                    || mustPlay )
                {
                    goto SAVE_CUR_RESULT;
                }
                else
                {
                    p->low=c->low;
                    continue;
                }

SAVE_CUR_RESULT:
                //update suit in hands;
                if ( bBigger )
                {
                    suit1.summary->nOffsetBigger = 3;
                }
                else
                {
                    suit1.summary->nOffsetBigger = 0;
                }


                if ( first_time
                    ||(CONTROL_SUB_COMBO_NUM_EX(&suit1) > CONTROL_SUB_COMBO_NUM_EX(suit) )
                    || ( (CONTROL_SUB_COMBO_NUM_EX(&suit1) == CONTROL_SUB_COMBO_NUM_EX(suit) )
                    && suit1.summary->combo_total_num < suit->summary->combo_total_num)
                    || ( (CONTROL_SUB_COMBO_NUM_EX(&suit1) == CONTROL_SUB_COMBO_NUM_EX(suit) )
                    && suit1.summary->combo_total_num == suit->summary->combo_total_num
                    && suit1.summary->combo_typenum < suit->summary->combo_typenum )
                    ||  ( (CONTROL_SUB_COMBO_NUM_EX(&suit1) == CONTROL_SUB_COMBO_NUM_EX(suit) )
                    && suit1.summary->combo_total_num == suit->summary->combo_total_num
                    && suit1.summary->combo_typenum == suit->summary->combo_typenum
                    && suit1.summary->singles_num < suit->summary->singles_num )
                    )
                {
                    memcpy(suit->combos , suit1.combos ,  20* sizeof(COMBOHAND)); //todo refine;
                    memset(suit->summary, 0, sizeof(COMBOS_SUMMARY));
                    sort_all_combos(suit->combos,20,suit->summary,suit->opps,suit->oppDown_num
                        ,suit->oppUp_num,suit->lower_control_poker); //to remove this..
                    first_time = 0;
                    memcpy(cur,c,sizeof(COMBOHAND));
                }
                p->low = c->low;
            }
        }
        return first_time;
    }
#pragma endregion chaipai
    return 0;
}

void COGLordRbtAIClv::lord_select_a_combo(SUITS * suit ,COMBOHAND* c)//select c from suit
{
    COMBOS_SUMMARY *s =suit->summary;
    COMBOHAND* t =NULL ;
    int pass = 1;
    if ( game->bLord ) //lord
    {
        if (   m_nRemainNum[(game->cur_playernum+1)%3] == 1  //last play farmer has 1 only
            || (m_nRemainNum[game->cur_playernum] == 1 ) // the down player of last play farmer has 1 only
            ||  m_nRemainNum[(game->cur_playernum+2)%3] == 1 // the Up player of last play farmer has 1 only  and current is a single.
            || suit->farmer_only_2)  
        {   //农民报单

            //如果手中有不含控制牌且不为单牌的套牌，则打出其中标签最小的一套
            t = find_smallest_in_combos(suit->combos,20,suit,1);

            if (t ==NULL)
            {   
                //手中只有两套或者三套单牌，且当前处于危险期，用非王的炸弹带两套标签最小的单牌打出
                if((s->singles_num ==2 ||s->singles_num == 3) && isDangous(suit))
                {
                    if(s->bomb_num>0)
                    {
                        c->type=411;
                        c->low = s->bomb[0]->low;
                        c->three_desc[0] = suit->summary->singles[0]->low;
                        c->three_desc[1] = suit->summary->singles[1]->low;
                        //remove_combo_poker(game->self_suit->h,suit->summary->singles[0],NULL);
                        //remove_combo_poker(game->self_suit->h,suit->summary->singles[1],NULL);
                        //remove_combo_poker(game->self_suit->h,suit->summary->bomb[0],NULL);

                        //if (s->singles[0]->control!=1)
                        //    s->combo_total_num --;
                        //else
                        //    s->biggest_num --;
                        //if (s->singles[1]->control!=1)
                        //    s->combo_total_num --;
                        //else
                        //    s->biggest_num --;
                        //s->real_total_num -=2;
                        //s->singles[0]->type=NOTHING_old;
                        //s->singles[1]->type=NOTHING_old;
                        //memcpy(s->singles,s->singles+2,(s->singles_num)*sizeof(void*));
                        //s->singles_num -=2 ;
                    }
                    else
                    {
                        t = s->singles[1];
                    }
                }
                //如果手中有两套以上单牌，打出第二小的
                else if ( s->singles_num >=2)  //   || s->singles_num == s->real_total_num )
                    t = s->singles[1];
                else
                {   
                    //zhp add 2011/5/3 和文档不一致
                    if(s->not_biggest_num>0)//打出含有控制牌的非冲锋套 del  new:打出其他含控制牌的非单牌
                    {
                        //t= s->not_biggest[0];
                        for(int k=0;k<s->not_biggest_num;k++)
                        {
                            if(s->not_biggest[k]->type!=SINGLE && s->not_biggest[k]->control == 1)
                            {
                                t=s->not_biggest[k];
                                break;
                            }
                        }
                    }

                    if (t ==NULL)
                    {
                        if (s->biggest_num>0 && s->biggest_num>s->bomb_num )//打出冲锋套                    
                        {
                            for(int k=0;k<s->biggest_num;k++)
                            {
                                if(s->biggest[k]->type!=BOMB_old&&s->biggest[k]->type!=ROCKET)
                                {
                                    t=s->biggest[k];
                                    break;
                                }
                            }//changed By fsy 2012-2-11 加入危险期
                        } else if(isDangous(suit)){
                            if(s->singles_num>0) //打出剩余的一张单牌                    
                                t=s->singles[0];
                            else if( s->bomb_num >0 )//打出炸弹
                                t= s->bomb[0];
                        }else{
                            if( s->bomb_num >0 )//打出炸弹
                                t= s->bomb[0];
                            else if(s->singles_num>0) //打出剩余的一张单牌                    
                                t=s->singles[0];
                        }
                    }
                }
            }
        }
        else 
        {
            if (   m_nRemainNum[(game->cur_playernum+1)%3] == 2  //last play farmer has 1 only
                || (m_nRemainNum[game->cur_playernum] == 2 ) // the down player of last play farmer has 1 only
                ||  m_nRemainNum[(game->cur_playernum+2)%3] == 2 )// the Up player of last play farmer has 1 only  and current is a single.          
            { //农民报双
                t = new COMBOHAND();
                pass = play_first_opp_has_2_poker_only(t,-1, suit, -1);
                if (!pass)
                {
                    if (t==NULL)
                    {
                        return;
                    }
                    memcpy(c,t,sizeof(COMBOHAND));
                    remove_combo_in_suit(s,t);
                    free(t);
                    t = NULL;
                    return;
                }
                free(t);
                t = NULL;
            }
            t = lord_select_combo_in_suit(s,suit);
        }
    }
    else
        t = lord_select_combo_in_suit(s,suit);

    if (t==NULL)
    {
        return;
    }
    memcpy(c,t,sizeof(COMBOHAND));
    remove_combo_in_suit(s,t);
    t->type=NOTHING_old;
}

void COGLordRbtAIClv::farmer_play_first_lord_has_1_poker_only(COMBOHAND* cur,int is_good_farmer, SUITS*suit, int isup)
{
    COMBOS_SUMMARY* s = suit->summary;
    search_combos_in_suits_for_min_single_farmer(suit->h,suit->combos,suit); //should done in other place, since you should know it..
    if (suit->summary->singles_num <=1
        || suit->summary->singles[1]->low >= suit->opps->end
        ) //todo refine... use function instead..
    {
        COMBOHAND * res;
        if ( (res=find_max_len_in_combos(suit->combos, 20))!=NULL) 
        {
            memcpy(cur,res,sizeof(COMBOHAND));
            res->type=NOTHING_old;
            remove_combo_in_suit(suit->summary,cur);
            return;
        }
    }
    else if (!isup && m_nRemainNum[(game->cur_playernum+1)%3]==1) //down player is farmer ,and has only one poker..
    {
        for (int i =suit->h->begin; i<suit->h->end ; i++)
        {
            if (suit->h->hands[i] > 0 && i < suit->opps->begin)
            {
                cur->len=1;
                cur->type =SINGLE;
                cur->low = i ;
                POKER t;
                suit->need_research = 1 ;
                memcpy(&t,suit->h,sizeof(POKER));
                remove_combo_poker(&t,cur,NULL);
                search_combos_in_suit(&t,suit->opps,suit);
                return;
            }
        }
    }
    else
    {
        if (!isup && suit->summary->singles_num ==2)
        { // 如果只有2套单牌，则打出最大的一套单牌，否则

            memcpy(cur,suit->summary->singles[1],sizeof(COMBOHAND));
            suit->summary->singles[1]->type=NOTHING_old;

            remove_combo_in_suit(suit->summary,cur);
            return;
        }
        if(suit->summary->pairs_num > 1)
        {
            int res = -1;
            for(int i = suit->opps->begin;i<suit->opps->end;i++)
            {
                if(suit->opps->hands[i]>1)
                {
                    res = i;
                    break;
                }
            }
            if(res != -1)
            {
                if(suit->summary->pairs[0]->low < res)
                {
                    memcpy(cur,suit->summary->pairs[0],sizeof(COMBOHAND));
                    suit->summary->pairs[0]->type=NOTHING_old;

                    remove_combo_in_suit(suit->summary,cur);
                    return;
                }

            }

        }

        {
            //手中只有两套或者三套单牌，且当前处于危险期，用非王的炸弹带两套标签最小的单牌打出
            //--save
            if((s->singles_num ==2 ||s->singles_num == 3) && isDangous(suit))
            {
                if(s->bomb_num>0)
                {
                    cur->type=411;
                    cur->low = s->bomb[0]->low;
                    cur->three_desc[0] = suit->summary->singles[0]->low;
                    cur->three_desc[1] = suit->summary->singles[1]->low;
                    //remove_combo_poker(game->self_suit->h,suit->summary->singles[0],NULL);
                    //remove_combo_poker(game->self_suit->h,suit->summary->singles[1],NULL);
                    //remove_combo_poker(game->self_suit->h,suit->summary->bomb[0],NULL);

                    //if (s->singles[0]->control!=1)
                    //    s->combo_total_num --;
                    //else
                    //    s->biggest_num --;
                    //if (s->singles[1]->control!=1)
                    //    s->combo_total_num --;
                    //else
                    //    s->biggest_num --;
                    //s->real_total_num -=2;
                    //s->singles[0]->type=NOTHING_old;
                    //s->singles[1]->type=NOTHING_old;
                    //memcpy(s->singles,s->singles+2,(s->singles_num)*sizeof(void*));
                    //s->singles_num -=2 ;
                    return;
                }
            }
            cur->len=1;
            cur->type =SINGLE;
            if(suit->h->begin == suit->h->end)
            {
                cur->low = suit->h->end;
            }
            else if(isup)
            {
                cur->low =  suit->h->end;
            }
            else
            {
                for(int i = suit->h->begin+1;i<suit->h->end;i++)
                {
                    if(suit->h->hands[i] > 0)
                    {
                        cur->low = i;
                        break;
                    }
                }
            }
            POKER t;
            memcpy(&t,suit->h,sizeof(POKER));
            suit->need_research = 1;
            remove_combo_poker(&t,cur,NULL);
            search_combos_in_suit(&t,suit->opps,suit);
            return;
            //goto combo_selected;
        }
    }
}
//select a combo from hands, save to cur
void COGLordRbtAIClv::upfarmer_play_first(COMBOHAND* cur,int is_good_farmer, SUITS*suit)
{
    int pass = 0;
    if ( suit->oppDown_num == 1)
    {
        farmer_play_first_lord_has_1_poker_only(cur,is_good_farmer,suit,1);
    }
    //else if ( !is_good_farmer) // for test
    else if ( is_good_farmer)
        up_good_farmer_play_first(game->self_suit,cur);
    else  // up bad first
    {
        if ( m_nRemainNum[(game->cur_playernum+2)%3] == 1 )
        {
            cur->len=1;
            cur->type =SINGLE;
            cur->low = suit->h->begin ;
            POKER t;
            memcpy(&t,suit->h,sizeof(POKER));
            remove_combo_poker(&t,cur,NULL);
            search_combos_in_suit(&t,suit->opps,suit);
            suit->need_research =1;
            return;
        }
        else if( suit->oppDown_num == 2) // 地主报双
        {
            pass = farmer_play_first_lord_has_2_poker_only(cur,is_good_farmer,suit,1);
            if(!pass) return ;
        }
        if ( m_nRemainNum[(game->cur_playernum+2)%3]== 2 )            //下家农民报双
        {
            for (int i = suit->h->begin; i<=suit->h->end; i++)
            {
                if (suit->h->hands[i]>=2)
                {
                    cur->len=1;
                    cur->type =PAIRS;
                    cur->low = i;
                    if (is_combo_biggest(suit->opps,cur,suit->oppUp_num,suit->oppDown_num,suit->lower_control_poker))
                        goto __play_single;
                    suit->need_research =1;
                    POKER t;
                    memcpy(&t,suit->h,sizeof(POKER));
                    remove_combo_poker(&t,cur,NULL);
                    search_combos_in_suit(&t,suit->opps,suit);
                    return;
                }
            }
            goto __play_single;

        } else {
            /*优先出对牌，从88开始往小找*/
            for (int i = char_to_poker('8'); i>0; i--)
            {
                if (suit->h->hands[i]==2)
                {
                    cur->len=1;
                    cur->type =PAIRS;
                    cur->low = i ;
                    suit->need_research =1;
                    POKER t;
                    memcpy(&t,suit->h,sizeof(POKER));
                    remove_combo_poker(&t,cur,NULL);
                    search_combos_in_suit(&t,suit->opps,suit);
                    return;
                }
            }
__play_single:

            COMBOHAND* t = unComfortPoker(suit,false,true);
            if(t == NULL)
            {
                /*没有对牌则出单牌，除去控制，从大往小出*/
                for (int i = suit->lower_control_poker-1; i>=suit->h->begin; i--)
                {
                    if (suit->h->hands[i]==1)
                    {
                        cur->len=1;
                        cur->type =SINGLE;
                        cur->low = i ;
                        POKER t;
                        memcpy(&t,suit->h,sizeof(POKER));
                        remove_combo_poker(&t,cur,NULL);
                        search_combos_in_suit(&t,suit->opps,suit);
                        suit->need_research =1;
                        return;
                    }
                }
                for( int i=0;i<suit->summary->pairs_num;i++)
                {
                    if(suit->summary->pairs[i]->control != 1)
                    {
                        cur->len=1;
                        cur->type =SINGLE;
                        cur->low = suit->summary->pairs[i]->low;
                        POKER t;
                        memcpy(&t,suit->h,sizeof(POKER));
                        remove_combo_poker(&t,cur,NULL);
                        search_combos_in_suit(&t,suit->opps,suit);
                        suit->need_research =1;
                        return;
                    }
                }
                t = &game->combos[0];
            }
            memcpy(cur,t,sizeof(COMBOHAND));
            remove_combo_in_suit(suit->summary,t);
            t->type =NOTHING_old;
        }
    }
    return;
}

void COGLordRbtAIClv::downfarmer_play_first(COMBOHAND* cur,int is_good_farmer, SUITS*suit)
{
    int pass = 0;
    //下家队友报单
    if ( suit->oppDown_num ==1 )
    {
        int lowest = suit->h->begin;
        for (int i=suit->h->begin; i<=suit->h->end; i++)
        {
            if (suit->h->hands[i]>0 && suit->h->hands[i]<4)
            {
                lowest = i;
                break;                
            }
        }
        if (lowest >= suit->opps->begin) //手中最小的牌不够小
        {
            if (suit->oppUp_num == 1) //地主也报单，走地主报单逻辑
            {
                farmer_play_first_lord_has_1_poker_only(cur,is_good_farmer,suit,0);
                return;
            }

            if(CONTROL_SUB_COMBO_NUM_IS_GOOD(suit)) //合手，走战斗农民出牌逻辑
            {
                down_good_farmer_play_first(suit,cur);
                return;
            }
        }
        else
        {
            if (lowest < suit->opps->begin)
            {
                if (suit->summary->bomb_num > 0) //有炸弹
                {                    
                    int biggestBomb = suit->summary->bomb[suit->summary->bomb_num - 1]->low;
                    if (isBiggestBomb(biggestBomb, suit->opps)) //有最大的炸弹
                    {
                        if (suit->summary->bomb_num > 1 && 
                            (suit->oppUp_num != 4 || maxBombCount(suit->opps) == 0))
                        {
                            cur->low = suit->summary->bomb[0]->low;
                        }
                        else
                        {
                            cur->low = biggestBomb;
                        }
                        cur->type = cur->low >= LIT_JOKER ? ROCKET : BOMB_old;
                        POKER t;
                        memcpy(&t,suit->h,sizeof(POKER));
                        remove_combo_poker(&t,cur,NULL);
                        search_combos_in_suit(&t,suit->opps,suit);
                        return;
                    }                    
                }
            }            
        }
        cur->type = SINGLE;
        cur->low = lowest;
        POKER t;
        memcpy(&t,suit->h,sizeof(POKER));
        remove_combo_poker(&t,cur,NULL);
        search_combos_in_suit(&t,suit->opps,suit);
        return;
    }
    else if ( suit->oppUp_num == 1)
    {
        farmer_play_first_lord_has_1_poker_only(cur,is_good_farmer,suit,0);
        return;
    }
    //add by fsy 地主报双逻辑
    else if( suit->oppUp_num == 2 )
    {
        pass = farmer_play_first_lord_has_2_poker_only(cur,is_good_farmer,suit,0);
        if(!pass) return;
    }
    if (is_good_farmer)
    {
        down_good_farmer_play_first(suit,cur);
    }
    else //bad farmer
    {
        COMBOHAND* t= unComfortPoker(suit,false,false);
        memcpy(cur,t,sizeof(COMBOHAND));
        t->type = NOTHING_old;
        remove_combo_in_suit(suit->summary,cur);
    }
}

//农民必管地主的牌,拍地主的2
int COGLordRbtAIClv::must_play_for_poker2(COMBOHAND * cur, SUITS* suit,COMBOHAND *pre, int check_opps_lit_joker)
{
    int    pass=1;
    if ( suit->h->hands[LIT_JOKER]==1 )
    {
        cur->type=SINGLE;
        cur->low = LIT_JOKER;
        pass =0;
        if ( suit->summary->bomb_num>0 && suit->summary->bomb[suit->summary->bomb_num-1]->low==LIT_JOKER)
        {
            suit->summary->bomb[suit->summary->bomb_num-1]->type=SINGLE;
            suit->summary->bomb[suit->summary->bomb_num-1]->low=BIG_JOKER;
            suit->summary->singles_num++;
            suit->summary->singles[suit->summary->singles_num-1]=suit->summary->bomb[suit->summary->bomb_num-1];
            suit->summary->bomb_num--;
        }
        else {
            //find joker in 31 3311.. 411..
            if ( !find_joker_in_combos(suit,LIT_JOKER))
            {
                rearrange_suit(suit,cur);
            }
        }
    }
    else if ( ( suit->h->hands[BIG_JOKER]==1 )
        &&(
        suit->opps->hands[LIT_JOKER] == 0
        || !check_opps_lit_joker
        )// Lit joker not seen, do not play for big_joker
        )
    {

        cur->type=SINGLE;
        cur->low = BIG_JOKER;
        pass =0;
        if ( !find_joker_in_combos(suit,BIG_JOKER))
        {
            rearrange_suit(suit,cur);
        }
    }
    else
        pass =1;
    return pass;
}

void COGLordRbtAIClv::up_good_farmer_play_first(SUITS * suit ,COMBOHAND* cur)//select cur from suit
{
    COMBOS_SUMMARY *s =suit->summary;
    COMBOHAND* t =NULL ;

    int pass = 0;
    //地主报双逻辑
    if(suit->oppDown_num == 2)
    {
        pass =     farmer_play_first_lord_has_2_poker_only(cur,1,suit,1);
        if(!pass) return;
    }
    //changed By fsy
    if ( !(CONTROL_SUB_COMBO_NUM_IS_GOOD(suit) && isStrongGood(suit))) //not heshou
    {
        if ( m_nRemainNum[(game->cur_playernum+2)%3] == 1 )  //地主下家报单
        {
            //非大牌控制的最小单牌
            if ( s->singles_num >=1 && s->singles[0]->low < game->lowest_bigPoker ) 
            {
                memcpy(cur,s->singles[0],sizeof(COMBOHAND));
                remove_combo_in_suit(s,s->singles[0]);
                return;
            }
        }

        if ( m_nRemainNum[(game->cur_playernum+2)%3] == 2 )  //地主下家报双
        {
            //<10的最小对子
            if ( s->pairs_num >=1 && s->pairs[0]->low < char_to_poker('T') ) 
            {
                //cur = s->pairs[0];
                memcpy(cur,s->pairs[0],sizeof(COMBOHAND));
                remove_combo_in_suit(s,s->pairs[0]);
                return;
            }
        }

        if (s->not_biggest_num == 2)
        {
            //select a biggest combo..
            if (s->biggest_num>0)
                t = s->biggest[0];
            else
            {
                if ( s->not_biggest[0]->low <= suit->h->begin && s->not_biggest[0]->low <= suit->opps->begin )
                    t= s->not_biggest[0];
                else if ( s->not_biggest[1]->low <= suit->h->begin && s->not_biggest[1]->low <= suit->opps->begin )
                    t= s->not_biggest[1];
                else if ( get_combo_number(s->not_biggest[0])>=get_combo_number(s->not_biggest[1]))
                {
                    t= s->not_biggest[0];
                }
                else
                    t= s->not_biggest[1];
            }
        }
        else
            t = unComfortPoker(suit,true,true);
    }
    else
        t = lord_select_combo_in_suit(s,suit);

    if (t==NULL)
    {
        return;
    }
    memcpy(cur,t,sizeof(COMBOHAND));
    remove_combo_in_suit(s,t);
    t->type=NOTHING_old;
}

void COGLordRbtAIClv::down_good_farmer_play_first(SUITS * suit ,COMBOHAND* cur)//select cur from suit
{
    /*如果下家报单，则优先出最小的单牌，否则
    如果只剩两套牌，且没冲锋套时，进入i），否则
    判断是否合手，如果否，进入d)；否则，进入e)。
    出所有套牌中标签最小的一套。（两套牌标签相同时，出张数多的一套）。
    如果当前套数=1，则先出冲锋套，再出其他套牌；否则
    寻找牌组中，套数最多的一类牌。如果是唯一一类，则进入h），否则进入g）
    存在多种牌套数相同时。选择张数最多的类型，进入h）
    打出这类牌中最小的一套。
    如果有3张以上套，则优先出，否则
    只有单/对，出标签小的那套，留标签大的套，标签一样时出对。
    */
    COMBOS_SUMMARY *s =suit->summary;
    COMBOHAND* t =NULL ;

    //   {
    if (s->not_biggest_num == 2 && s->real_total_num == 2)
    {
        /*如果2种牌各剩1套，且没冲锋套时。如果有3张以上套，则优先出；如果只有单/对，则出号码小的那套，留号码大的套
        */
        if ( suit->summary->singles_num==1 &&  suit->summary->pairs_num==1 )
        {
            if ( suit->summary->singles[0]->low > suit->summary->pairs[0]->low )
            {
                memcpy(cur, suit->summary->pairs[0],sizeof(COMBOHAND));
                suit->summary->pairs[0]->type=NOTHING_old;
            }
            else {
                memcpy(cur, suit->summary->singles[0],sizeof(COMBOHAND));
                suit->summary->singles[0]->type=NOTHING_old;
            }
            remove_combo_in_suit(suit->summary,cur);//to be optimized
        }
        else if ( suit->summary->singles_num==2 )
        {
            memcpy(cur, suit->summary->singles[0],sizeof(COMBOHAND));
            suit->summary->singles[0]->type=NOTHING_old;
            remove_combo_in_suit(suit->summary,cur);//to be optimized
        }
        else if ( suit->summary->pairs_num==2 )
        {
            memcpy(cur, suit->summary->pairs[0],sizeof(COMBOHAND));
            suit->summary->pairs[0]->type=NOTHING_old;
            remove_combo_in_suit(suit->summary,cur);//to be optimized
        }
        else
        {
            for (int k=20    ; k>=0; k--)
            {
                if (get_combo_number(&suit->combos[k])>=3)
                {
                    memcpy(cur, &suit->combos[k],sizeof(COMBOHAND));
                    suit->combos[k].type=NOTHING_old;
                    remove_combo_in_suit(suit->summary,cur);//to be optimized
                    return;
                }
            }
        }
        return;
    }

    if ( !(CONTROL_SUB_COMBO_NUM_IS_GOOD(suit)&&isStrongGood(suit))) //not heshou
    {
        t = unComfortPoker(suit,true,false);
    }
    else
        t = lord_select_combo_in_suit(s,suit);

    if (t==NULL)
    {
        return;
    }
    memcpy(cur,t,sizeof(COMBOHAND));
    remove_combo_in_suit(s,t);
    t->type=NOTHING_old;
}

// search in suit to get a bigger one than pre
// 1. search in summary first
// 2. search in hands, if find one, re-arrange hands and save the combo summary to suit->summary
//save result to cur
int COGLordRbtAIClv::must_play_bigger(COMBOHAND * cur, SUITS* suit,COMBOHAND *pre,int isup, int isgood,int isfarmer)
{
    COMBOHAND * c = find_combo(suit->summary,pre);
    int pass =1 ;

    //拆牌
#pragma region chaipai
    if ( c==NULL) {  
Label_Chaipai:
        COMBOHAND tmp1,tmp,*p=&tmp;
        c= &tmp1;
        memcpy(p,pre,sizeof(COMBOHAND));
        int first_time =1;
        SUITS suit1;
        POKER h,opps;
        COMBOHAND combo[20]={20*0};
        COMBOS_SUMMARY sum={0},*s;
        suit1.combos=&combo[0];
        suit1.h = &h;
        suit1.opps = &opps;
        suit1.summary =s = &sum;
        while ( find_a_bigger_combo_in_hands(suit,suit->h,c,p)) //could not handle corner case for  BOMB_old in two combos.
        {

            suit1.lower_control_poker = suit->lower_control_poker;
            suit1.oppDown_num = suit->oppDown_num;
            suit1.oppUp_num = suit->oppUp_num;
            memcpy(&h,suit->h,sizeof(POKER));

            memcpy(&opps,suit->opps,sizeof(POKER));
            remove_combo_poker(suit1.h,c,NULL);

            search_combos_in_suit(suit1.h,suit1.opps,&suit1);

            if (!search_1_2_for_a_three(p, suit1.h, &suit1, c)) //处理三带一
            {
                p->low = c->low;
                continue;
            }
            else
            {
                if (suit1.need_research)
                    search_combos_in_suit(suit1.h,suit1.opps,&suit1);
            }

            //update suit in hands;
            if ( is_combo_biggest(suit->opps,c,suit->oppDown_num,suit->oppUp_num,suit->lower_control_poker) )
            {
                suit1.summary->nOffsetBigger = 3;
                suit1.summary->nOffsetComboNum = 0;
            }
            else
            {
                suit1.summary->nOffsetBigger = 0;
                suit1.summary->nOffsetComboNum = 10;
            }

            //i.    控制 - 套的数目 最少
            //ii.    套的数量最少
            //iii.    套的种类
            //iv.    单牌数量最少

            if ( first_time
                ||(CONTROL_SUB_COMBO_NUM_EX(&suit1) > CONTROL_SUB_COMBO_NUM_EX(suit) )
                || ( (CONTROL_SUB_COMBO_NUM_EX(&suit1) == CONTROL_SUB_COMBO_NUM_EX(suit) )
                && suit1.summary->combo_total_num < suit->summary->combo_total_num)
                || ( (CONTROL_SUB_COMBO_NUM_EX(&suit1) == CONTROL_SUB_COMBO_NUM_EX(suit) )
                && suit1.summary->combo_total_num == suit->summary->combo_total_num
                && suit1.summary->combo_typenum < suit->summary->combo_typenum )
                ||  ( (CONTROL_SUB_COMBO_NUM_EX(&suit1) == CONTROL_SUB_COMBO_NUM_EX(suit) )
                && suit1.summary->combo_total_num == suit->summary->combo_total_num
                && suit1.summary->combo_typenum == suit->summary->combo_typenum
                && suit1.summary->singles_num < suit->summary->singles_num )
                )
            {
                memcpy(suit->combos , suit1.combos ,  20* sizeof(COMBOHAND)); //todo refine;
                memset(suit->summary, 0, sizeof(COMBOS_SUMMARY));
                sort_all_combos(suit->combos,20,suit->summary,suit->opps,suit->oppDown_num
                    ,suit->oppUp_num,suit->lower_control_poker); //to remove this..
                first_time = 0;
                memcpy(cur,c,sizeof(COMBOHAND));
            }
            p->low = c->low;
        }
        return first_time;
    }
#pragma endregion chaipai
    else if ( c->type!=BOMB_old /*|| 1*/)// !(isup && !isgood && isfarmer))  //up bad farmer do not play bomb here.
    {
        remove_combo_in_suit(suit->summary,c);
        memcpy(cur,c,sizeof(COMBOHAND));
        c->type=NOTHING_old;
        pass=0;
    }
    else if ( c->type==BOMB_old  )
    {
        SUITS suit1;
        POKER h,opps;
        COMBOHAND combo[20]={20*0};
        COMBOS_SUMMARY sum={0},*s;
        suit1.combos=&combo[0];
        suit1.h = &h;
        suit1.opps = &opps;
        suit1.summary =s = &sum;

        suit1.lower_control_poker = suit->lower_control_poker;
        suit1.oppDown_num = suit->oppDown_num;
        suit1.oppUp_num = suit->oppUp_num;
        memcpy(&h,suit->h,sizeof(POKER));

        memcpy(&opps,suit->opps,sizeof(POKER));
        remove_combo_poker(suit1.h,c,NULL);

        search_combos_in_suit(suit1.h,suit1.opps,&suit1);

        //changed By fsy 强合手，危险期
        if (  CONTROL_SUB_COMBO_NUM(&suit1)  > -20 && !isDangous(suit) && isStrongGood(suit))   //  出完控制数>出完套数-2 ,即合手， 可以出炸弹
        {
            remove_combo_in_suit(suit->summary,c);
            memcpy(cur,c,sizeof(COMBOHAND));
            c->type=NOTHING_old;
            pass=0;
        }
        else
        {
            goto Label_Chaipai;
        }
    }
    return pass;
}

//地主报单时地主下家农民被动出一张最小的牌
int COGLordRbtAIClv::find_a_bigger_combo_in_suit(SUITS* suit, COMBOHAND * cur, COMBOHAND* a)
{
    COMBOHAND * c = find_combo(suit->summary,a);
    int pass =1 ;

    if ( c ) 
    {
        remove_combo_in_suit(suit->summary,c);
        memcpy(cur,c,sizeof(COMBOHAND));
        c->type=NOTHING_old;
        pass=0;
    }

    return pass;
}

//地主报单农民必管逻辑
int COGLordRbtAIClv::must_play_bigger_for_single(COMBOHAND * cur, SUITS* suit,COMBOHAND *pre,int isup, int isgood,int isfarmer)
{
    COMBOHAND * c = find_combo(suit->summary,pre);
    int pass =1 ;
    if ( c==NULL) {
        //拆牌
        COMBOHAND tmp1,tmp,*p=&tmp;
        c= &tmp1;
        memcpy(p,pre,sizeof(COMBOHAND));
        int first_time =1;
        SUITS suit1;
        POKER h,opps;
        COMBOHAND combo[20]={20*0};
        COMBOS_SUMMARY sum={0},*s;
        suit1.combos=&combo[0];
        suit1.h = &h;
        suit1.opps = &opps;
        suit1.summary =s = &sum;
        while ( find_a_bigger_combo_in_hands(suit,suit->h,c,p)) 
        {
            suit1.lower_control_poker = suit->lower_control_poker;
            suit1.oppDown_num = suit->oppDown_num;
            suit1.oppUp_num = suit->oppUp_num;
            memcpy(&h,suit->h,sizeof(POKER));
            memcpy(&opps,suit->opps,sizeof(POKER));
            remove_combo_poker(suit1.h,c,NULL);
            search_combos_in_suits_for_min_single_farmer(suit1.h,suit1.combos,&suit1);
            if (!search_1_2_for_a_three(p, suit1.h, &suit1, c)) //处理三带一
            {
                break; //找不到带牌
            }
            else
            {
                if (suit1.need_research)
                    search_combos_in_suits_for_min_single_farmer(suit1.h,suit1.combos,&suit1);
            }
            //update suit in hands;
            if ( first_time
                ||( isup && get_2nd_min_singles(suit1.summary) > get_2nd_min_singles(suit->summary))//寻找第二小的单牌最大的组合
                ||( !isup && suit1.summary->singles_num < suit->summary->singles_num )//寻找单牌最少的组合
                )
            {
                memcpy(suit->combos , suit1.combos ,  20* sizeof(COMBOHAND)); 
                memset(suit->summary, 0, sizeof(COMBOS_SUMMARY));
                sort_all_combos(suit->combos,20,suit->summary,suit->opps,suit->oppDown_num
                    ,suit->oppUp_num,suit->lower_control_poker); 
                first_time = 0;
                memcpy(cur,c,sizeof(COMBOHAND));
            }
            p->low = c->low;
        }
        return first_time;
    }
    else if ( c->type!=BOMB_old || 1)// !(isup && !isgood && isfarmer)) //检查炸弹？
    {
        remove_combo_in_suit(suit->summary,c);
        memcpy(cur,c,sizeof(COMBOHAND));
        c->type=NOTHING_old;
        pass=0;
    }
    return pass;
}

int COGLordRbtAIClv::farmer_shunpai(SUITS * suit ,COMBOHAND* cur, COMBOHAND* pre)
{
    COMBOS_SUMMARY *s =suit->summary;
    COMBOHAND *c;
    int mustPlay=0;
    int current_control = calc_controls(suit->h, suit->opps, cacl_control_poker_num(game)); //move calculate to other place
    suit->max_control.single = current_control;
    current_control += suit->summary->extra_bomb_ctrl*10;
    int current_combo_num = suit->summary->combo_total_num;
    //顺牌
    COMBOHAND tmp,*p =&tmp;
    memcpy(p,pre,sizeof(COMBOHAND)); //todo: optimize this
    while ( (c =find_combo(s,p))!=NULL)
    {
        int max_poker_in_c;
        if ( (c->type==SINGLE_SERIES || c->type==PAIRS_SERIES || c->type >=3311 ||
            ((max_poker_in_c=has_control_poker(c,game->lowest_bigPoker))<0))
            && c->control != 1)
        {        //可以出牌
            remove_combo_in_suit(s,c);
            memcpy(cur,c,sizeof(COMBOHAND));
            c->type = NOTHING_old;
            return 0;
        }
        else
        {
            // calculate control..
            POKER  t,opps;
            memcpy(&t,suit->h,sizeof(POKER));
            memcpy(&opps,suit->opps,sizeof(POKER));
            remove_combo_poker(&t,c,NULL);
            //remove one/two poker bigger than max poker in c in opps.
            assume_remove_controls_in_opps(&opps, c,max_poker_in_c);
            int new_control = calc_controls(&t, &opps, cacl_control_poker_num(game));
            add_poker(&t,&opps,&t);
            int new_lower = get_lowest_controls(&t, cacl_control_poker_num(game));
            int baktype= c->type;
            c->type=NOTHING_old;
            update_summary(suit->summary,suit->h,suit->opps,suit->combos,
                20,suit->oppDown_num,suit->oppUp_num,new_lower);
            int combo_num_after = suit->summary->combo_total_num;//  - (c->control ==0);
            new_control+=s->extra_bomb_ctrl*10;

            c->type= baktype;
            update_summary(suit->summary,suit->h,suit->opps,suit->combos,
                20,suit->oppDown_num,suit->oppUp_num,suit->lower_control_poker);

            if ( (new_control >= current_control)
                || ( new_control > (combo_num_after-2)*10 )
                ||(new_control>=30)
                || mustPlay
                )
            {
                remove_combo_in_suit(s,c);
                memcpy(cur,c,sizeof(COMBOHAND));
                c->type = NOTHING_old;
                return 0;
            }
            else
            {
                memcpy(p,c,sizeof(COMBOHAND));
                continue;
            }
        }
    }
    return 1;
}

int COGLordRbtAIClv::farmer_play_when_lord_has_1_poker_only(COMBOHAND* cur, COMBOHAND* pre, SUITS*suit,  int isup,int isgood)
{
    if ( game->pre_playernum == m_nLord && pre->type != SINGLE)
    {
        //地主报单农民必管逻辑
        search_combos_in_suits_for_min_single_farmer(suit->h,suit->combos,suit);
        return must_play_bigger_for_single(cur,suit,pre,isup,isgood,1);
    }
    else
    {
        if(game->pre_playernum == m_nLord)
        {
            {
                //如果手中有绝对大的单牌，则出
_IS_BIG_ENOUGH:
                vector<int> biggestSingles;
				for (int k = max_ddz(pre->low + 1, suit->opps->end); k <= suit->h->end; k++)
                {                    
                    if (suit->h->hands[k]>0)
                    {
                        biggestSingles.push_back(k);                        
                    }
                }
                if (biggestSingles.size() > 0)
                {
                    SUITS suit1;
                    POKER h,opps;
                    COMBOHAND combo[20]={20*0};
                    COMBOS_SUMMARY sum={0},*s;
                    suit1.combos=&combo[0];
                    suit1.h = &h;
                    suit1.opps = &opps;
                    suit1.summary =s = &sum;
                    for (int i=0; i<biggestSingles.size(); i++)
                    {
                        COMBOHAND c;
                        c.type = SINGLE;
                        c.low = biggestSingles[i];
                        c.len = 1;

                        suit1.lower_control_poker = suit->lower_control_poker;
                        suit1.oppDown_num = suit->oppDown_num;
                        suit1.oppUp_num = suit->oppUp_num;
                        memcpy(&h,suit->h,sizeof(POKER));
                        memcpy(&opps,suit->opps,sizeof(POKER));
                        remove_combo_poker(suit1.h,&c,NULL);
                        search_combos_in_suits_for_min_single_farmer(suit1.h,suit1.combos,&suit1);
                        if (suit1.need_research)
                        {
                            search_combos_in_suits_for_min_single_farmer(suit1.h,suit1.combos,&suit1);
                        }
                        if ( i==0 || get_2nd_min_singles(suit1.summary) > get_2nd_min_singles(suit->summary)
                            || suit1.summary->singles_num < suit->summary->singles_num )
                        {
                            memcpy(suit->combos , suit1.combos ,  20 * sizeof(COMBOHAND)); 
                            memset(suit->summary, 0, sizeof(COMBOS_SUMMARY));
                            sort_all_combos(suit->combos,20,suit->summary,suit->opps,suit->oppDown_num
                                ,suit->oppUp_num,suit->lower_control_poker);
                            memcpy(cur,&c,sizeof(COMBOHAND));
                        }
                    }
                    return 0;
                }                
                if (suit->summary->bomb_num!=0 )//有炸弹 
                {
                    if ( isup)
                    {
                        if(suit->summary->singles_num <2)
                        {
                            memcpy(cur,suit->summary->bomb[0],sizeof(COMBOHAND));
                            suit->summary->bomb[0]->type=NOTHING_old;
                            remove_combo_in_suit(suit->summary,cur);
                            return 0;
                        }
                    }
                    else
                    {
                        POKER t;
                        memcpy(&t,suit->h,sizeof(POKER));
                        remove_combo_poker(&t,suit->summary->bomb[0],NULL); //todo: for mulit bomb
                        memcpy(cur,suit->summary->bomb[0],sizeof(COMBOHAND));
                        suit->summary->bomb[0]->type=NOTHING_old;
                        remove_combo_in_suit(suit->summary,cur);
                        search_combos_in_suits_for_min_single_farmer(&t,suit->combos,suit);
                        if ( get_2nd_min_singles(suit->summary) >= suit->opps->end )
                        { //heshou
                            return 0;
                        }
                        else
                        {
                            search_combos_in_suits_for_min_single_farmer(&t,suit->combos,suit);                        
                        }
                        return find_a_bigger_combo_in_suit( suit ,cur ,pre );
                        //return 1;
                    }
                }
                if ( isup)
                {
                    if (suit->h->end>pre->low)
                    {
                        cur->type=SINGLE;
                        cur->low = suit->h->end;
                        cur->len = 1;
                        suit->need_research = 1;
                        POKER t;
                        memcpy(&t,suit->h,sizeof(POKER));
                        remove_combo_poker(&t,cur,NULL);
                        search_combos_in_suit(&t,suit->opps,suit);
                        //suit->h->hands[i]--;
                        return 0;
                    }
                }
                else
                {
                    return find_a_bigger_combo_in_suit( suit ,cur ,pre );
                }
            }
        }
        else 
        {
            //_CHECK_GOOD_ENOUGH:
            if(pre->type == SINGLE)
            {
                if(isup) //地主上家
                {
                    //如果该牌为外面最大.. 则过牌？
                    if (pre->low >= suit->opps->end && game->pre_playernum != m_nLord )
                    {
                        return 1;
                    }
                    goto _IS_BIG_ENOUGH;
                }
            }
            search_combos_in_suits_for_min_single_farmer(suit->h,suit->combos,suit);
            if ( get_2nd_min_singles(suit->summary) >= suit->opps->end ) //合手
            {
                COMBOHAND* c= find_combo(suit->summary,pre);//
                if ( c==NULL)
                {
                    COMBOHAND tmp1,tmp,*p=&tmp;
                    c= &tmp1;
                    memcpy(p,pre,sizeof(COMBOHAND));
                    int first_time =1;
                    SUITS suit1;
                    POKER h,opps;
                    COMBOHAND combo[20]={20*0};
                    COMBOS_SUMMARY sum={0},*s;
                    suit1.combos=&combo[0];
                    suit1.h = &h;
                    suit1.opps = &opps;
                    suit1.summary =s = &sum;
                    while ( find_a_bigger_combo_in_hands(suit,suit->h,c,p)) //could not handle corner case for  BOMB_old in two combos.
                    {
                        suit1.lower_control_poker = suit->lower_control_poker;
                        suit1.oppDown_num = suit->oppDown_num;
                        suit1.oppUp_num = suit->oppUp_num;
                        memcpy(&h,suit->h,sizeof(POKER));

                        memcpy(&opps,suit->opps,sizeof(POKER));
                        remove_combo_poker(suit1.h,c,NULL);

                        search_combos_in_suits_for_min_single_farmer(suit1.h,suit1.combos,&suit1);

                        if (!search_1_2_for_a_three(p, suit1.h, &suit1, c)) //处理三带一
                        {
                            p->low = c->low;
                            continue;
                        }
                        else        {
                            if (suit1.need_research)
                                search_combos_in_suits_for_min_single_farmer(suit1.h,suit1.combos,&suit1);
                        }


                        if ( get_2nd_min_singles(suit->summary) >= suit->opps->end )
                        {//update suit in hands;
                            memcpy(suit->combos , suit1.combos ,  20* sizeof(COMBOHAND)); //todo refine;
                            memset(suit->summary, 0, sizeof(COMBOS_SUMMARY));
                            sort_all_combos(suit->combos,20,suit->summary,suit->opps,suit->oppDown_num
                                ,suit->oppUp_num,suit->lower_control_poker); //to remove this..
                            first_time = 0;
                            memcpy(cur,c,sizeof(COMBOHAND));
                            break;
                        }
                        p->low = c->low;
                    }
                    return first_time;
                }
                else if ( c->type!=BOMB_old || 1)// !(isup && !isgood && isfarmer))  //up bad farmer do not play bomb here.
                {
                    remove_combo_in_suit(suit->summary,c);
                    memcpy(cur,c,sizeof(COMBOHAND));
                    c->type=NOTHING_old;

                    return 0;
                }
            }
            else if (pre->type==SINGLE )//goto...
            {
                return 1;
            }
        }
    }
    return 1;
}

void COGLordRbtAIClv::prase_poker(  vector <int> &   ParaVec )
{
    m_handCard = ParaVec;
    POKER * h = game->self_suit->h;
    memset(h ,0 ,sizeof(POKER));

    int nWhat = -1;
    for( int i = 0 ; i < ParaVec.size() ; i++ )
    {
        if ( ParaVec[i] >= 52)
        {
            nWhat = ParaVec[i] - 52 + 13;
        }
        else
        {
            nWhat = ParaVec[i] % 13;
        }
        h->hands[nWhat]++;
    }
    sort_poker(h);
}

void COGLordRbtAIClv::VectorToPoker( vector <int> &   curCard, POKER * h )
{
    memset(h ,0 ,sizeof(POKER));

    int nWhat = -1;
    for( int i = 0 ; i < curCard.size() ; i++ )
    {
        if ( curCard[i] >= 52)
        {
            nWhat = curCard[i] - 52 + 13;
        }
        else
        {
            nWhat = curCard[i] % 13;
        }
        h->hands[nWhat]++;
    }
}

//ParaVec里找h,输出pOut
void COGLordRbtAIClv::search_poker( vector <int> &   ParaVec ,COMBOHAND* h, vector <int> & pOut )
{
    std::vector<int> curCard;    

    //找到牌的值
#pragma region search
    switch (h->type)
    {
    case ROCKET :
        curCard.push_back(LIT_JOKER);
        curCard.push_back(BIG_JOKER);
        break;
    case SINGLE_SERIES:
        for (int i=h->low; i<h->low+h->len; i++) curCard.push_back(i);
        break;
    case PAIRS_SERIES:
        for (int i=h->low; i<h->low+h->len; i++) {
            curCard.push_back(i);
            curCard.push_back(i);
        }
        break;
    case THREE_SERIES:
        for (int i=h->low; i<h->low+h->len; i++) {
            curCard.push_back(i);
            curCard.push_back(i);
            curCard.push_back(i);
        }
        break;
    case BOMB_old:
        curCard.push_back(h->low);
    case THREE:
        curCard.push_back(h->low);
    case PAIRS:
        curCard.push_back(h->low);
    case SINGLE:
        curCard.push_back(h->low);
        break;
    case 31: //31
        for (int i=h->low; i<h->low+1; i++)
        {
            curCard.push_back(i);
            curCard.push_back(i);
            curCard.push_back(i);
        }
        curCard.push_back(h->three_desc[0]);
        break;
    case 32: //32
        for (int i=h->low; i<h->low+1; i++) 
        {
            curCard.push_back(i);
            curCard.push_back(i);
            curCard.push_back(i);
        }
        curCard.push_back(h->three_desc[0]);
        curCard.push_back(h->three_desc[0]);
        break;
    case 3311: //3311
        for (int i=h->low; i<h->low+2; i++)
        {
            curCard.push_back(i);
            curCard.push_back(i);
            curCard.push_back(i);
        }
        curCard.push_back(h->three_desc[0]);
        curCard.push_back(h->three_desc[1]);
        break;
    case 3322: //3322
        for (int i=h->low; i<h->low+2; i++) 
        {
            curCard.push_back(i);
            curCard.push_back(i);
            curCard.push_back(i);
        }
        curCard.push_back(h->three_desc[0]);
        curCard.push_back(h->three_desc[1]);
        curCard.push_back(h->three_desc[0]);
        curCard.push_back(h->three_desc[1]);
        break;
    case 333111: //333111
        for (int i=h->low; i<h->low+3; i++) 
        {
            curCard.push_back(i);
            curCard.push_back(i);
            curCard.push_back(i);
        }
        curCard.push_back(h->three_desc[0]);
        curCard.push_back(h->three_desc[1]);
        curCard.push_back(h->three_desc[2]);
        break;
    case 333222: //333222
        for (int i=h->low; i<h->low+3; i++) 
        {
            curCard.push_back(i);
            curCard.push_back(i);
            curCard.push_back(i);
        }
        curCard.push_back(h->three_desc[0]);
        curCard.push_back(h->three_desc[1]);
        curCard.push_back(h->three_desc[2]);
        curCard.push_back(h->three_desc[0]);
        curCard.push_back(h->three_desc[1]);
        curCard.push_back(h->three_desc[2]);
        break;
    case 33332222: //33332222
        for (int i=h->low; i<h->low+4; i++)
        {
            curCard.push_back(i);
            curCard.push_back(i);
            curCard.push_back(i);
        }
        curCard.push_back(h->three_desc[0]);
        curCard.push_back(h->three_desc[1]);
        curCard.push_back(h->three_desc[2]);
        curCard.push_back(h->three_desc[3]);
        curCard.push_back(h->three_desc[0]);
        curCard.push_back(h->three_desc[1]);
        curCard.push_back(h->three_desc[2]);
        curCard.push_back(h->three_desc[3]);
        break;
    case 33331111: //33331111
        for (int i=h->low; i<h->low+4; i++)
        {
            curCard.push_back(i);
            curCard.push_back(i);
            curCard.push_back(i);
        }
        curCard.push_back(h->three_desc[0]);
        curCard.push_back(h->three_desc[1]);
        curCard.push_back(h->three_desc[2]);
        curCard.push_back(h->three_desc[3]);
        break;
    case 3333311111: //3333311111
        for (int i=h->low; i<h->low+5; i++) 
        {
            curCard.push_back(i);
            curCard.push_back(i);
            curCard.push_back(i);
        }
        curCard.push_back(h->three_desc[0]);
        curCard.push_back(h->three_desc[1]);
        curCard.push_back(h->three_desc[2]);
        curCard.push_back(h->three_desc[3]);
        curCard.push_back(h->three_desc[4]);
        break;
    case 411: //411
        for (int i=h->low; i<h->low+1; i++) 
        {
            curCard.push_back(i);
            curCard.push_back(i);
            curCard.push_back(i);
            curCard.push_back(i);
        }
        curCard.push_back(h->three_desc[0]);
        curCard.push_back(h->three_desc[1]);
        break;
    case 422: //422
        for (int i=h->low; i<h->low+1; i++) 
        {
            curCard.push_back(i);
            curCard.push_back(i);
            curCard.push_back(i);
            curCard.push_back(i);
        }
        curCard.push_back(h->three_desc[0]);
        curCard.push_back(h->three_desc[1]);
        curCard.push_back(h->three_desc[0]);
        curCard.push_back(h->three_desc[1]);
        break;
    case NOTHING_old:
        break;
    default:
        break;
    };
#pragma endregion search

    std::vector<int> newVector;   
    std::copy(m_handCard.begin(), m_handCard.end(), std::back_inserter(newVector));      

   /* TCHAR szBuf[1024] = {0},
        szTmp[10]= {0};*/

    int nWhat = 0;
    for (int i = 0; i < curCard.size(); i++)//更新手中牌
    {
        bool bFind = false;

        for (int j = 0; j < newVector.size(); j++)
        {
            if ( newVector[j] >= 52)
            {
                nWhat = newVector[j] - 52 + 13;
            }
            else
            {
                nWhat = newVector[j] % 13;

            }
            if ( nWhat == curCard[i])
            {
                pOut.push_back(newVector[j]);
                newVector.erase(newVector.begin()+j);
                bFind = true;
                break;
            }
        }
        /*_stprintf(szTmp ,_T("%d,") ,curCard[i]);
        _tcscat( szBuf , szTmp );*/
    }

    for( int i = 0 ; i < m_handCard.size() ; i++ )
    {
        int nWhat = m_handCard[i] ;
       /* _stprintf(szTmp ,_T("%d,") ,nWhat);
        _tcscat( szBuf , szTmp );*/
    }

    for( int i = 0 ; i < newVector.size() ; i++ )
    {
        int nWhat = newVector[i] ;
    }
}

void COGLordRbtAIClv::OnMesTakeOutCard( vector <int> &   curCard  , int nPlayerNum  )
{
    if ( curCard.size() == 0 )
    {
        game->playerCounter++;    
        return;
    }

    if ( nPlayerNum == m_nSeat )
        RemoveCard( curCard );

    POKER curPoker;
    VectorToPoker( curCard ,&curPoker );

    sort_poker(&curPoker );

    COMBOHAND *combo,hand;
    combo=&hand;

    if ( is_combo(&curPoker,combo))
    {
        OnTakeOut( nPlayerNum,combo);
    }
}

//选择一套牌出
int COGLordRbtAIClv::takeout_poker(   vector <int> &   pOut )
{
    pOut.clear();
    if (m_handCard.size() == 0) 
    {
        return 0;
    }

    COMBOHAND hand;

    int pass1 = select_a_combo(&hand);

    if (hand.type == THREE_ONE || hand.type == THREE_TWO)
    {
        checkKicker(&hand, &(game->combos_summary));
    }
    if (!pass1)
    {
        search_poker( m_handCard , &hand ,pOut );
    }
    return pass1;
}

void COGLordRbtAIClv::send_poker(  vector <int> &   curCard )
{
    vector <int>::iterator Iter;
    for( Iter = m_handCard.begin(); Iter != m_handCard.end(); )
    {
        Iter = m_handCard.erase( Iter);
    }

    std::copy(curCard.begin(), curCard.end(), std::back_inserter(m_handCard));

    POKER * h = game->self_suit->h;
    VectorToPoker( curCard ,h );

    game->self_suit->lower_control_poker=
        game->lowest_bigPoker = get_lowest_controls(&game->all, cacl_control_poker_num(game));

    sort_poker(game->self_suit->h);
    sub_poker(&game->all,game->self_suit->h,game->self_suit->opps);

    //将牌分套
    search_combos_in_suit(game->self_suit->h,game->self_suit->opps,game->self_suit);
}

void COGLordRbtAIClv::send_pot(  vector <int> &   curPot)
{
    POKER curPoker;
    VectorToPoker( curPot ,&curPoker );

    add_poker(game->self_suit->h,&curPoker,game->self_suit->h);
    sub_poker(game->self_suit->opps,&curPoker,game->self_suit->opps);

    sort_poker(game->self_suit->h);

    search_combos_in_suit(game->self_suit->h,game->self_suit->opps,game->self_suit);

    std::copy(curPot.begin(), curPot.end(), std::back_inserter(m_handCard));  
}

// select a combo form suit
COMBOHAND* COGLordRbtAIClv::lord_select_combo_in_suit(COMBOS_SUMMARY* s,SUITS* suit)
{
    if ( !(CONTROL_SUB_COMBO_NUM_IS_GOOD(suit) && isStrongGood(suit))) //不合手
    {
        return unComfortPoker(suit,-1,false); //地主
    }

    COMBOHAND * c= NULL;
    int num =s->combo_total_num;
    bool hasBiggestBomb = true;
    if (s->bomb_num > 0)
    {
        hasBiggestBomb = isBiggestBomb(s->bomb[s->bomb_num - 1]->low, &game->opp);
    }
    else
    {
        hasBiggestBomb = maxBombCount(&game->opp) == 0 ;
    }
    if (num<=s->bomb_num + (hasBiggestBomb ? 1 : 0))//for bomb

    {

        if( s->biggest_num>0){
            //非炸弹，先出小冲锋套
            int key=s->biggest[0]->low;
            int res=0;
            for(int i=0;i < s->biggest_num;i++) 
            {
                if( key >  s->biggest[i]->low
                    && s->biggest[i]->type!=BOMB_old 
                    && s->biggest[i]->type!=ROCKET)
                {
                    res=i;
                    key =  s->biggest[i]->low;
                }
            } 
            if(s->biggest[res]->type!=BOMB_old && s->biggest[res]->type!=ROCKET) 
            {
                return s->biggest[res];
            }
        }
        if ( s->not_biggest_num>0 )
        {
            return s->not_biggest[0];
        }

        if ( s->bomb_num>0 )
        {
            return s->bomb[0];
        }
        return NULL;
    }

    //找出冲锋套中套数最多的套
    int max=0,type=0,temp=0;

    for(int i=0;i<s->biggest_num;i++)
    {
        if(s->biggest[i]->type == SINGLE )
        {
            temp++;
        }
    }
    if(temp>max) {
        type = SINGLE;
    }
    temp = 0;
    for(int i=0;i<s->biggest_num;i++)
    {
        if(s->biggest[i]->type == PAIRS ){
            temp++;
        }
    }
    if(temp>max) {
        type = PAIRS;
    }
    temp = 0;
    for(int i=0;i<s->biggest_num;i++)
    {
        if(s->biggest[i]->type == THREE ){
            temp++;
        }
    }
    if(temp>max) {
        type = THREE;
    }
    temp = 0;
    for(int i=0;i<s->biggest_num;i++)
    {
        if(s->biggest[i]->type == SINGLE_SERIES ){
            temp++;
        }
    }
    if(temp>max) {
        type = SINGLE_SERIES;
    }
    temp = 0;
    for(int i=0;i<s->biggest_num;i++)
    {
        if(s->biggest[i]->type == PAIRS_SERIES){
            temp++;
        }
    }
    if(temp>max) {
        type = PAIRS_SERIES;
    }
    temp = 0;
    for(int i=0;i<s->biggest_num;i++)
    {
        if(s->biggest[i]->type == THREE_SERIES ){
            temp++;
        }
    }
    if(temp>max) {
        type = THREE_SERIES;
    }
    temp = 0;
    for(int i=0;i<s->biggest_num;i++)
    {
        if(s->biggest[i]->type == THREE_ONE )
        {
            temp++;
        }
    }
    if(temp>max) {
        type = THREE_ONE;
    }
    temp = 0;
    for(int i=0;i<s->biggest_num;i++)
    {
        if(s->biggest[i]->type == THREE_TWO ){
            temp++;
        }
    }
    if(temp>max) {
        type = THREE_TWO;
    }
    temp = 15;
    int res=-1;
    for(int i=0;i< s->biggest_num;i++)
    {
        if(s->biggest[i]->type == type)
        {
            if(temp >s->biggest[i]->low) 
            {
                res = i;
                temp = s->biggest[i]->low;
            }
        }
    }
    c=s->biggest[res];

    // 判断如果出完该套牌后，出完控制数>出完套数-1，则打出。否则出所有套牌中标签最小的一套。
    {
        SUITS suit1;
        POKER h,opps;
        COMBOHAND combo[20]={20*0};
        COMBOS_SUMMARY sum={0},*s;
        suit1.combos=&combo[0];
        suit1.h = &h;
        suit1.opps = &opps;
        suit1.summary =s = &sum;
        suit1.lower_control_poker = suit->lower_control_poker;
        suit1.oppDown_num = suit->oppDown_num;
        suit1.oppUp_num = suit->oppUp_num;
        memcpy(&h,suit->h,sizeof(POKER));

        memcpy(&opps,suit->opps,sizeof(POKER));
        remove_combo_poker(suit1.h,c,NULL);

        search_combos_in_suit(suit1.h,suit1.opps,&suit1);

        int max_poker_in_c = has_control_poker(c, game->lowest_bigPoker);

        if (  CONTROL_SUB_COMBO_NUM(&suit1)  < -10 )
        {
            //出所有套牌中标签最小的一套。（两套牌标签相同时，出张数多的一套）
            return  find_smallest_in_combos(suit->combos,20,suit,0);
        }
    }
    return c;
}

//返回1表示pass , 农民管队友
int COGLordRbtAIClv::farmer_select_after_farmer(SUITS * suit ,COMBOHAND* cur, COMBOHAND* pre)//select a combo from suit and save to cur.
{
    int mustPlay = 0;
	int isup = game->self_suit->id == UPFARMER_old;
    int nOther = 0, nRemain = 0;
    nOther = isup ? 2 : 1;
    nRemain = m_nRemainNum[(game->cur_playernum + nOther)%3];
    COMBOS_SUMMARY *s =suit->summary;
    COMBOHAND *c;
    int current_control = calc_controls(suit->h, suit->opps, cacl_control_poker_num(game)); //move calculate to other place
    suit->max_control.single = current_control;
    current_control += suit->summary->extra_bomb_ctrl*10;
    int current_combo_num = suit->summary->combo_total_num;

    //     必管或必不管,    如果打出A后，出完套数<=炸弹数，则出A，称为“强合手必管”
    {
        COMBOHAND tmp,*p =&tmp;
        memcpy(p,pre,sizeof(COMBOHAND)); //todo: optimize this
        while ( (c =find_combo(s,p))!=NULL)
        {
            int max_poker_in_c = has_control_poker(c,game->lowest_bigPoker);
            /*******************************
            d)    如果打出A后，出完控制数量>出完套数-2，则出牌A，称为“管牌”。否则，
            g)    令X=A，跳回到顺牌逻辑3) -b)
            *********************************/
            // calculate control..
            POKER  t,opps;
            memcpy(&t,suit->h,sizeof(POKER));
            memcpy(&opps,suit->opps,sizeof(POKER));
            remove_combo_poker(&t,c,NULL);
            //remove one/two poker bigger than max poker in c in opps.
            assume_remove_controls_in_opps(&opps, c,max_poker_in_c);
            int new_control = calc_controls(&t, &opps, cacl_control_poker_num(game));
            add_poker(&t,&opps,&t);
            int new_lower = get_lowest_controls(&t, cacl_control_poker_num(game));
            int baktype= c->type;
            c->type=NOTHING_old;
            update_summary(suit->summary,suit->h,suit->opps,suit->combos,
                20,suit->oppDown_num,suit->oppUp_num,new_lower);
            int combo_num_after = suit->summary->combo_total_num;//  - (c->control ==0);
            new_control+=s->extra_bomb_ctrl*10;

            c->type= baktype;
            update_summary(suit->summary,suit->h,suit->opps,suit->combos,
                20,suit->oppDown_num,suit->oppUp_num,suit->lower_control_poker);

            if ( ( suit->summary->extra_bomb_ctrl * 10 ) >= ( (combo_num_after)*10 ) )  
            {
                remove_combo_in_suit(s,c);
                memcpy(cur,c,sizeof(COMBOHAND));
                c->type = NOTHING_old;
                return 0;
            }
            else
            {
                memcpy(p,c,sizeof(COMBOHAND));
                continue;
            }
        }
    }
    int nLabel = char_to_poker('K');

    if ( nRemain <=2 || pre->type==SINGLE_SERIES || pre->type==PAIRS_SERIES || pre->type >=3311 ||
        pre->low >= nLabel  )
    {//c)    如果另一农民报单或者报双，或该套牌的标签>=K，或该套牌为飞机、双顺，则“必不管”
        return 1;
    }

    //顺牌
#pragma region shunpai
    COMBOHAND tmp,*p =&tmp;
    memcpy(p,pre,sizeof(COMBOHAND)); //todo: optimize this
    while ( (c =find_combo(s,p))!=NULL)
    {
        int max_poker_in_c;
        if ( ((max_poker_in_c=has_control_poker(c,game->lowest_bigPoker))<0) )
        {//如果该牌型不包含当前大牌控制，则出牌
            remove_combo_in_suit(s,c);
            memcpy(cur,c,sizeof(COMBOHAND));
            c->type = NOTHING_old;
            return 0;
        }
        else
        {
            /*******************************
            d)    如果打出A后，出完控制数量>出完套数-2，则出牌A，称为“管牌”。否则，
            g)    令X=A，跳回到顺牌逻辑3) -b)
            *********************************/
            // calculate control..
            POKER  t,opps;
            memcpy(&t,suit->h,sizeof(POKER));
            memcpy(&opps,suit->opps,sizeof(POKER));
            remove_combo_poker(&t,c,NULL);
            //remove one/two poker bigger than max poker in c in opps.
            assume_remove_controls_in_opps(&opps, c,max_poker_in_c);
            int new_control = calc_controls(&t, &opps, cacl_control_poker_num(game));
            add_poker(&t,&opps,&t);
            int new_lower = get_lowest_controls(&t, cacl_control_poker_num(game));
            int baktype= c->type;
            c->type=NOTHING_old;
            update_summary(suit->summary,suit->h,suit->opps,suit->combos,
                20,suit->oppDown_num,suit->oppUp_num,new_lower);
            int combo_num_after = suit->summary->combo_total_num;//  - (c->control ==0);
            new_control+=s->extra_bomb_ctrl*10;

            c->type= baktype;
            update_summary(suit->summary,suit->h,suit->opps,suit->combos,
                20,suit->oppDown_num,suit->oppUp_num,suit->lower_control_poker);

            if (( new_control > (combo_num_after-2)*10 )  
                )
            {
                remove_combo_in_suit(s,c);
                memcpy(cur,c,sizeof(COMBOHAND));
                c->type = NOTHING_old;
                return 0;
            }
            else
            {
                memcpy(p,c,sizeof(COMBOHAND));
                continue;
            }
        }
    }
#pragma endregion shunpai
    //Chai Pai
#pragma region chaipai
    {
        COMBOHAND tmp1;
        c= &tmp1;
        memcpy(p,pre,sizeof(COMBOHAND));
        int first_time =1;
        int nBigger = 0;
        int nExtraCombo = 0;
        while ( find_a_bigger_combo_in_hands(suit,suit->h,c,p)) //could not handle corner case for  BOMB_old in two combos.
        {
            SUITS suit1;
            POKER h,opps;
            COMBOHAND combo[20]={20*0};
            COMBOS_SUMMARY sum={0},*s;
            suit1.combos=&combo[0];
            suit1.h = &h;
            suit1.opps = &opps;
            suit1.summary =s = &sum;
            suit1.lower_control_poker = suit->lower_control_poker;
            suit1.oppDown_num = suit->oppDown_num;
            suit1.oppUp_num = suit->oppUp_num;
            memcpy(&h,suit->h,sizeof(POKER));

            memcpy(&opps,suit->opps,sizeof(POKER));
            remove_combo_poker(suit1.h,c,NULL);

            search_combos_in_suit(suit1.h,suit1.opps,&suit1);

            bool  bBigger = FALSE;

            if (!search_1_2_for_a_three(p, suit1.h, &suit1, c)) //处理三带一
            {
                p->low = c->low;
                continue;
            }
            else
            {
                if (suit1.need_research)
                    search_combos_in_suit(suit1.h,suit1.opps,&suit1);
            }

            int max_poker_in_c = has_control_poker(c, game->lowest_bigPoker);
            if ( max_poker_in_c < 0 )  //不包含大牌控制
            {
                /*
                1)    如果打出(A)，系统判断剩余的牌最优组合后的套数，如果套数比之前的套数-1，则直接打出，
                2)    如果套数和以前一样但打出的一套是冲锋套，则直接打出
                */
                if ( suit1.summary->combo_total_num <  suit->summary->combo_total_num
                    )
                {
                    {
                        memcpy(suit->combos , suit1.combos ,  20* sizeof(COMBOHAND)); //todo refine;
                        memset(suit->summary, 0, sizeof(COMBOS_SUMMARY));
                        sort_all_combos(suit->combos,20,suit->summary,suit->opps,suit->oppDown_num
                            ,suit->oppUp_num,suit->lower_control_poker); //to remove this..
                        memcpy(cur,c,sizeof(COMBOHAND));
                    }

                    return 0;
                }
                if (  CONTROL_SUB_COMBO_NUM(&suit1)  > -20 )
                {
                    goto SAVE_CUR_RESULT;
                }
                else
                {
                    p->low=c->low;
                    continue;
                }
            }
            else
            {   //包含大牌控制

                p->low=c->low;
                continue;

SAVE_CUR_RESULT:
                //update suit in hands;
                if ( first_time
                    ||(CONTROL_SUB_COMBO_NUM(&suit1) > CONTROL_SUB_COMBO_NUM(suit) )
                    || ( (CONTROL_SUB_COMBO_NUM(&suit1) == CONTROL_SUB_COMBO_NUM(suit) )
                    && suit1.summary->combo_total_num < suit->summary->combo_total_num)
                    || ( (CONTROL_SUB_COMBO_NUM(&suit1) == CONTROL_SUB_COMBO_NUM(suit) )
                    && suit1.summary->combo_total_num == suit->summary->combo_total_num
                    && suit1.summary->combo_typenum < suit->summary->combo_typenum )
                    ||  ( (CONTROL_SUB_COMBO_NUM(&suit1) == CONTROL_SUB_COMBO_NUM(suit) )
                    && suit1.summary->combo_total_num == suit->summary->combo_total_num
                    && suit1.summary->combo_typenum == suit->summary->combo_typenum
                    && suit1.summary->singles_num < suit->summary->singles_num )
                    )
                {
                    memcpy(suit->combos , suit1.combos ,  20* sizeof(COMBOHAND)); //todo refine;
                    memset(suit->summary, 0, sizeof(COMBOS_SUMMARY));
                    sort_all_combos(suit->combos,20,suit->summary,suit->opps,suit->oppDown_num
                        ,suit->oppUp_num,suit->lower_control_poker); //to remove this..
                    first_time = 0;
                    memcpy(cur,c,sizeof(COMBOHAND));
                }
                p->low = c->low;
            }
        }
        return first_time;
    }
#pragma endregion chaipai
    return 0;
}

//断线续玩时传进来已经出过的牌
void COGLordRbtAIClv::send_takeout_card(  vector <int> &   takeCard)
{
    POKER curPoker;
    VectorToPoker( takeCard ,&curPoker );
    sub_poker(game->self_suit->opps,&curPoker,game->self_suit->opps);
    search_combos_in_suit(game->self_suit->h,game->self_suit->opps,game->self_suit);
}

//断线续玩时传进来用户牌张数
void COGLordRbtAIClv::init_playercard_num(  vector <int> &   cardNum)
{
    if ( cardNum.size()!=3 )
        return;

    for(int i=0;i<3;i++)
    {
        m_nRemainNum[i] = cardNum[i];
    }
}

//add By fsy 2012_2_14
//返回0表示分牌成功
int COGLordRbtAIClv::farmer_play_first_lord_has_2_poker_only(COMBOHAND* cur,int is_good_farmer, SUITS*suit, int isup)
{
    COMBOS_SUMMARY* s = suit->summary;
    if(suit->oppSingleKing){
        farmer_play_first_lord_has_1_poker_only(cur,is_good_farmer,suit,isup);
        return 0;
    }
    return play_first_opp_has_2_poker_only(cur,is_good_farmer,suit,isup);

}

//返回0表示分牌成功
int COGLordRbtAIClv::farmer_play_when_lord_has_2_poker_only(COMBOHAND* cur, COMBOHAND* pre, SUITS*suit,  int isup,int isgood)
{
    if(pre->type == PAIRS ){
        int largest= 0;
        for(int i=MAX_POKER_KIND-1;i>0;i--)
        {
            if(suit->opps->hands[i]>1)
            {
                largest = i;
                break;
            }
        }
        for(int i=0;i<suit->summary->pairs_num;i++)
        {
            if(suit->summary->pairs[i]->low > largest && suit->summary->pairs[i]->low > game->pre.low)
            {
                memcpy( cur,suit->summary->pairs[i],sizeof(COMBOHAND));
                suit->summary->pairs[i]->type=NOTHING_old;
                remove_combo_in_suit(suit->summary,cur);
                return 0;
            }
        }
        // Label_Chaipai:
        COMBOHAND * c = find_combo(suit->summary,pre);
        COMBOHAND tmp1,tmp,*p=&tmp;
        c= &tmp1;
        memcpy(p,pre,sizeof(COMBOHAND));
        int first_time =1;
        SUITS suit1;
        POKER h,opps;
        COMBOHAND combo[20]={20*0};
        COMBOS_SUMMARY sum={0},*s;
        suit1.combos=&combo[0];
        suit1.h = &h;
        suit1.opps = &opps;
        suit1.summary =s = &sum;
        while ( find_a_bigger_combo_in_hands(suit,suit->h,c,p)) //could not handle corner case for  BOMB_old in two combos.
        {

            suit1.lower_control_poker = suit->lower_control_poker;
            suit1.oppDown_num = suit->oppDown_num;
            suit1.oppUp_num = suit->oppUp_num;
            memcpy(&h,suit->h,sizeof(POKER));

            memcpy(&opps,suit->opps,sizeof(POKER));
            remove_combo_poker(suit1.h,c,NULL);

            search_combos_in_suit(suit1.h,suit1.opps,&suit1);

            if (!search_1_2_for_a_three(p, suit1.h, &suit1, c)) //处理三带一
            {
                p->low = c->low;
                continue;
            }
            else
            {
                if (suit1.need_research)
                    search_combos_in_suit(suit1.h,suit1.opps,&suit1);
            }

            if ( is_combo_biggest(suit->opps,c,suit->oppDown_num,suit->oppUp_num,suit->lower_control_poker) )
            {
                suit1.summary->nOffsetBigger = 3;
                suit1.summary->nOffsetComboNum = 0;
            }
            else
            {
                suit1.summary->nOffsetBigger = 0;
                suit1.summary->nOffsetComboNum = 10;
            }

            //i.    控制 - 套的数目 最少
            //ii.    套的数量最少
            //iii.    套的种类
            //iv.    单牌数量最少

            if ( first_time
                ||(CONTROL_SUB_COMBO_NUM_EX(&suit1) > CONTROL_SUB_COMBO_NUM_EX(suit) )
                || ( (CONTROL_SUB_COMBO_NUM_EX(&suit1) == CONTROL_SUB_COMBO_NUM_EX(suit) )
                && suit1.summary->combo_total_num < suit->summary->combo_total_num)
                || ( (CONTROL_SUB_COMBO_NUM_EX(&suit1) == CONTROL_SUB_COMBO_NUM_EX(suit) )
                && suit1.summary->combo_total_num == suit->summary->combo_total_num
                && suit1.summary->combo_typenum < suit->summary->combo_typenum )
                ||  ( (CONTROL_SUB_COMBO_NUM_EX(&suit1) == CONTROL_SUB_COMBO_NUM_EX(suit) )
                && suit1.summary->combo_total_num == suit->summary->combo_total_num
                && suit1.summary->combo_typenum == suit->summary->combo_typenum
                && suit1.summary->singles_num < suit->summary->singles_num )
                )
            {
                memcpy(suit->combos , suit1.combos ,  20* sizeof(COMBOHAND)); //todo refine;
                memset(suit->summary, 0, sizeof(COMBOS_SUMMARY));
                sort_all_combos(suit->combos,20,suit->summary,suit->opps,suit->oppDown_num
                    ,suit->oppUp_num,suit->lower_control_poker); //to remove this..
                first_time = 0;
                memcpy(cur,c,sizeof(COMBOHAND));
            }
            p->low = c->low;
        }
        return first_time;
    }else return 1;
}

//返回0成功拆牌
int COGLordRbtAIClv::play_first_opp_has_2_poker_only(COMBOHAND* cur,int is_good_farmer, SUITS*suit, int isup)
{
    COMBOS_SUMMARY* s = suit->summary;
    if(s->singles_num>0 &&s->pairs_num>0)
    {
        int res=-1,key = -1;
        bool isMax=true;
        bool unControl = false;
        int unControlIndex = -1;
        for(int i=0;i<s->singles_num;i++)
        {
            if(s->singles[i]->control == 0){
                unControl = true;
                unControlIndex = i;
            }
            if(key < s->singles[i]->low){
                res = i;
                key=s->singles[i]->low;
            }
        }

        for(int i=key;i<MAX_POKER_KIND;i++)
        {
            if(suit->opps->hands[i]>0)
            {
                isMax = false;
            }
        }

        if(s->bomb_num > 0 
            || ((s->singles[res]->control == 1 || s->singles[res]->low == LIT_JOKER || s->singles[res]->low == BIG_JOKER) 
            && isMax))
        {
            if(s->not_biggest_num>0 && unControl){
                //打出最大非控制单牌
                memcpy( cur,s->singles[unControlIndex],sizeof(COMBOHAND));
                suit->summary->singles[unControlIndex]->type=NOTHING_old;
                remove_combo_in_suit(suit->summary,cur);
                return 0;
            }
            else{
                //从最小的对牌拆出
                cur->low = s->pairs[0]->low;
                cur->len = 1;
                cur->type = SINGLE;
                POKER t;
                memcpy(&t,suit->h,sizeof(POKER));
                remove_combo_poker(&t,cur,NULL);
                search_combos_in_suit(&t,suit->opps,suit);
                return 0;    
            }
        }else {
            if(is_good_farmer == -1)
            {
                //农民报单
                suit->farmer_only_2 = true;
                lord_select_a_combo(suit,cur);
                suit->farmer_only_2 = false;
                return 0;
            }
            else
            {
                //地主报单逻辑
                farmer_play_first_lord_has_1_poker_only(cur,is_good_farmer,suit,isup);
                return 0;
            }
        }
    }else{
        //非报双逻辑
        return 1;
    }
}

int COGLordRbtAIClv::lord_play_when_famer_has_2_poker_only(COMBOHAND* cur, COMBOHAND* pre, SUITS*suit)
{
    if(pre->type == PAIRS ){
        int largest= 0;
        for(int i=MAX_POKER_KIND-1;i>0;i--)
        {
            if(suit->opps->hands[i]>1)
            {
                largest = i;
                break;
            }
        }
        for(int i=0;i<suit->summary->pairs_num;i++)
        {
            if(suit->summary->pairs[i]->low > largest && suit->summary->pairs[i]->low > game->pre.low)
            {
                memcpy( cur,suit->summary->pairs[i],sizeof(COMBOHAND));
                suit->summary->pairs[i]->type=NOTHING_old;
                remove_combo_in_suit(suit->summary,cur);
                return 0;
            }
        }
        //转回地主顺牌逻辑
        return 1;
    }
    else return 1;    
}

//地主上家垃圾农民返回函数有所不同
COMBOHAND* COGLordRbtAIClv::unComfortPoker(SUITS * suit,int is_good_famer,int isUp)
{
    //不顺牌
    COMBOHAND* cur=NULL;
    int res = -1,low = 15,i=0;
    if(is_game_first_half(game->cur_playernum))
    {
        while(game->combos[i].type != 0)
        {
            //所有的出牌都是主动出牌
            if(game->combos[i].type == game->pre.type 
                && game->combos[i].control != 1)
            {
                if( low > game->combos[i].low && game->combos[i].control != 1)
                {
                    low = game->combos[i].low;
                    res = i;
                }
            }
            i++;
        }
        if(res != -1)
        {
            cur = &game->combos[res];
        }
        else
        {
            //喜好牌
            if( isUp > 0  && game->self_suit->likeType[m_nLord] != game->self_suit->likeType[(m_nSeat+2)%3]
            && game->self_suit->likeType[(m_nSeat+2)%3] != 0)
            {
                while(game->combos[i].type != 0)
                {
                    if(game->combos[i].type == game->self_suit->likeType[(m_nSeat+2)%3] 
                    && game->combos[i].control != 1)
                    {
                        if( low > game->combos[i].low)
                        {
                            low = game->combos[i].low;
                            res = i;
                        }
                    }
                    i++;
                }
                if(res != -1)
                {
                    cur = &game->combos[res];
                } 
            }
            else if(isUp == 0 && game->self_suit->likeType[m_nLord] != game->self_suit->likeType[(m_nSeat+1)%3]
            && game->self_suit->likeType[(m_nSeat+1)%3] != 0)
            {
                while(game->combos[i].type != 0)
                {
                    if(game->combos[i].type == game->self_suit->likeType[(m_nSeat+1)%3] 
                    && game->combos[i].control != 1)
                    {
                        if( low > game->combos[i].low)
                        {
                            low = game->combos[i].low;
                            res = i;
                        }
                    }
                    i++;
                }
                if(res != -1)
                {
                    cur = &game->combos[res];
                } 
            }

            if(!(!is_good_famer && isUp))
            {
                //出所有套牌中标签最小的一套。（两套牌标签相同时，出张数多的一套）
                cur = find_smallest_in_combos(suit->combos,20,suit,0);
            }
        }
    }
    else
    {
        if(!(!is_good_famer && isUp))
        {
            //出所有套牌中标签最小的一套。（两套牌标签相同时，出张数多的一套）
            cur = find_smallest_in_combos(suit->combos,20,suit,0);
        }
    }
    return cur;
}

// 旧版机器人客户端AI逻辑代码 end
// ========================================================
// 旧版机器人BaseOperate.cpp的中C函数代码

char COGLordRbtAIClv::poker_to_char(int i)
{
    switch ( i) {
    case 0:
        return '3';
    case 1:
        return '4';
    case 2:
        return '5';
    case 3:
        return '6';
    case 4:
        return '7';
    case 5:
        return '8';
    case 6:
        return '9';
    case 7:
        return 'T';
    case 8:
        return 'J';
    case 9:
        return 'Q';
    case 10:
        return 'K';
    case 11:
        return 'A';
    case 12:
        return '2';
    case 13:
        return 'X';
    case 14:
        return 'D';
    default:
        return '?';
    }
}

int COGLordRbtAIClv::char_to_poker(char c)
{
    switch ( c) {
    default:
        return -1;
    case '3':
        return 0;

    case '4':
        return 1;

    case '5':
        return 2;

    case '6':
        return 3;

    case '7':
        return 4;

    case '8':
        return 5;

    case '9':
        return 6;

    case 't':
    case 'T':
        return 7;

    case 'j':
    case 'J':
        return 8;

    case 'q':
    case 'Q':
        return 9;

    case 'k':
    case 'K':
        return 10;

    case 'a':
    case 'A':
        return 11;

    case '2':
        return 12;

    case 'x':
    case 'X':
        return 13;

    case 'D':
    case 'd':
        return 14;
    }
}

int COGLordRbtAIClv::sort_poker(POKER * h )
{
    int res=true;
    h->begin=-1;
    h->end=h->total=0;
    for (int i=0; i<MAX_POKER_KIND; i++)
    {
        if (h->hands[i]>0)
        {
            if (h->begin==-1) h->begin = i;
            h->end =i;
        }
        else if (h->hands[i]<0)
        {
            res=false;
        }
        h->total+=h->hands[i];
    }
    if (h->begin==-1) h->begin = 0;
    return res;
}


void COGLordRbtAIClv::read_poker(char * buf, POKER * h  )
{
    do {
        switch ( *buf) {
        case '3':
            h->hands[0]++;
            break;
        case '4':
            h->hands[1]++;
            break;
        case '5':
            h->hands[2]++;
            break;
        case '6':
            h->hands[3]++;
            break;
        case '7':
            h->hands[4]++;
            break;
        case '8':
            h->hands[5]++;
            break;
        case '9':
            h->hands[6]++;
            break;
        case 't':
        case 'T':
            h->hands[7]++;
            break;
        case 'j':
        case 'J':
            h->hands[8]++;
            break;
        case 'q':
        case 'Q':
            h->hands[9]++;
            break;
        case 'k':
        case 'K':
            h->hands[10]++;
            break;
        case 'a':
        case 'A':
            h->hands[11]++;
            break;
        case '2':
            h->hands[12]++;
            break;
        case 'x':
        case 'X':
            h->hands[13]++;
            break;
        case 'D':
        case 'd':
            h->hands[14]++;
            break;
        default:
            break;
        }
    }    while ( *buf++!='\0');


    //check more than four
    {
        int i;
        for (i=0; i<15; i++)
        {
            if (h->hands[i]>4) h->hands[i]=4;
        }
    }
    sort_poker(h);
}


int COGLordRbtAIClv::remove_combo_poker(POKER* hand, COMBOHAND * h, COMBOHAND *h1  ) // h must be contained by hand.
{
    switch (h->type)
    {
    case ROCKET :
        hand->hands[LIT_JOKER]=0;
        hand->hands[BIG_JOKER]=0;
        break;
    case SINGLE_SERIES:
        for (int i=h->low; i<h->low+h->len; i++) hand->hands[i]--;
        break;
    case PAIRS_SERIES:
        for (int i=h->low; i<h->low+h->len; i++) {
            hand->hands[i]-=2;
        }
        break;
    case THREE_SERIES:
        for (int i=h->low; i<h->low+h->len; i++) {
            hand->hands[i]-=3;
        }
        break;
        //减一个就可以了?
    case BOMB_old:
        hand->hands[h->low]--;
    case THREE:
        hand->hands[h->low]--;
    case PAIRS:
        hand->hands[h->low]--;
    case SINGLE:
        hand->hands[h->low]--;
        break;
    case 31: //31
        for (int i=h->low; i<h->low+1; i++) hand->hands[i]-=3;
        hand->hands[h->three_desc[0]]--;
        break;
    case 32: //32
        for (int i=h->low; i<h->low+1; i++) hand->hands[i]-=3;
        hand->hands[h->three_desc[0]]--;
        hand->hands[h->three_desc[0]]--;
        break;
    case 3311: //3311
        for (int i=h->low; i<h->low+2; i++) hand->hands[i]-=3;
        hand->hands[h->three_desc[0]]--;
        hand->hands[h->three_desc[1]]--;
        break;
    case 3322: //3322
        for (int i=h->low; i<h->low+2; i++) hand->hands[i]-=3;
        hand->hands[h->three_desc[0]]--;
        hand->hands[h->three_desc[1]]--;
        hand->hands[h->three_desc[0]]--;
        hand->hands[h->three_desc[1]]--;
        break;
    case 333111: //333111
        for (int i=h->low; i<h->low+3; i++) hand->hands[i]-=3;
        hand->hands[h->three_desc[0]]--;
        hand->hands[h->three_desc[1]]--;
        hand->hands[h->three_desc[2]]--;
        break;
    case 333222: //333222
        for (int i=h->low; i<h->low+3; i++) hand->hands[i]-=3;
        hand->hands[h->three_desc[0]]--;
        hand->hands[h->three_desc[1]]--;
        hand->hands[h->three_desc[2]]--;
        hand->hands[h->three_desc[0]]--;
        hand->hands[h->three_desc[1]]--;
        hand->hands[h->three_desc[2]]--;
        break;
    case 33332222: //33332222
        for (int i=h->low; i<h->low+4; i++) hand->hands[i]-=3;
        hand->hands[h->three_desc[0]]--;
        hand->hands[h->three_desc[1]]--;
        hand->hands[h->three_desc[2]]--;
        hand->hands[h->three_desc[0]]--;
        hand->hands[h->three_desc[1]]--;
        hand->hands[h->three_desc[2]]--;
        hand->hands[h->three_desc[3]]-=3;
        break;
    case 33331111: //33331111
        for (int i=h->low; i<h->low+4; i++) hand->hands[i]-=3;
        hand->hands[h->three_desc[0]]--;
        hand->hands[h->three_desc[1]]--;
        hand->hands[h->three_desc[2]]--;
        hand->hands[h->three_desc[3]]--;
        break;
    case 3333311111: //3333311111
        for (int i=h->low; i<h->low+5; i++) hand->hands[i]-=3;
        hand->hands[h->three_desc[0]]--;
        hand->hands[h->three_desc[1]]--;
        hand->hands[h->three_desc[2]]--;
        hand->hands[h->three_desc[3]]--;
        hand->hands[h->three_desc[4]]--;
        break;
    case 411: //411
        for (int i=h->low; i<h->low+1; i++) hand->hands[i]-=4;
        hand->hands[h->three_desc[0]]--;
        hand->hands[h->three_desc[1]]--;
        break;
    case 422: //422
        for (int i=h->low; i<h->low+1; i++) hand->hands[i]-=4;
        hand->hands[h->three_desc[0]]--;
        hand->hands[h->three_desc[1]]--;
        hand->hands[h->three_desc[0]]--;
        hand->hands[h->three_desc[1]]--;
        break;
    case NOTHING_old:
        break;
    default:
        break;
    };

    return sort_poker(hand);

}

void COGLordRbtAIClv::add_poker(POKER* a, POKER * b, POKER * c ) // c=a+b
{
    for (int i=0; i<MAX_POKER_KIND; i++)
    {
        c->hands[i]  = a->hands[i] + b->hands[i];
    }
    c->total=a->total+b->total;
	c->end = max_ddz(a->end, b->end);
	c->begin = min_ddz(a->begin, b->begin);
}

void COGLordRbtAIClv::sub_poker(POKER* a, POKER * b, POKER * c ) // c=a-b
{
    for (int i=0; i<MAX_POKER_KIND; i++)
    {
        c->hands[i]  = a->hands[i] - b->hands[i];
    }
    sort_poker(c);
}


int COGLordRbtAIClv::is_sub_poker(POKER* a, POKER * b ) // a in b ? 1 : 0
{
    for (int i=0; i<MAX_POKER_KIND; i++)
    {
        if ( a->hands[i] > b->hands[i]) return false;
    }
    return true;
}


bool COGLordRbtAIClv::getBomb(POKER* h, COMBOHAND * p) //
{
    int total=h->total;
    for (int i=h->begin; i<=h->end; i++)
    {
        if (h->hands[i] == 4) //bomb got
        {
            p->type = BOMB_old;
            p->low = i;
            return true;
        }
    }
    return false;
}


bool COGLordRbtAIClv::getThree(POKER* h, COMBOHAND * p) //
{
    int total=h->total;
    for (int i=h->begin; i<=h->end; i++)
    {
        if (h->hands[i] == 3) //Got Three
        {
            p->type = THREE;
            p->low  = i;
            return true;
        }
    }
    return false;
}

bool COGLordRbtAIClv::getThreeSeries(POKER* h, COMBOHAND * p)
{
    for (int i = h->begin; i<= (CEIL_A(h->end)- 1);)
    {
        int j;
        for (j=i; j<=CEIL_A(h->end); j++) {
            if ( h->hands[j]<3) {
                break;
            }
        }
        if ( j > i+1 )
        {
            p->type = THREE_SERIES;
            p->low = i;
            p->len = j-i;
            return true;
        }
        i=j+1;
    }
    return false;
}

bool COGLordRbtAIClv::getDoubleSeries(POKER* h, COMBOHAND * p)
{
    for (int i = h->begin; i<= (CEIL_A(h->end) - 2);)
    {
        int j;
        for (j=i; j<=CEIL_A(h->end); j++) {
            if ( h->hands[j]<2) {
                break;
            }
        }
        if ( j >i+2  )
        {
            p->type = PAIRS_SERIES;
            p->low = i;
            p->len = j-i;
            return true;
        }
        i=j+1;
    }
    return false;
}

//双顺调优
bool COGLordRbtAIClv::updateDoubleSeries(POKER* h, COMBOHAND * p)
{
    //from head to tail.
    while ( p->len >3)
    {
        int end=p->len+p->low-1;
        if ( h->hands[end] > h->hands[p->low] )
        { //remove tail
            h->hands[end]+=2;
            p->len--;
        }
        else if (h->hands[p->low] >0)
        {
            h->hands[p->low]+=2;
            p->low++;
            p->len--;
        }
        else
            break;
    }
    return false;
}

bool COGLordRbtAIClv::updateDoubleSeries1(POKER* h, COMBOHAND * p)
{
    if (p->len ==3)
    {
        if ( h->hands[p->low]==1 && h->hands[p->low+2]==1) // cl
            return true;
    }
    return false;
}

bool COGLordRbtAIClv::getSeries(POKER* h, COMBOHAND * p)
{
    for (int i = h->begin; i<= (h->end - 4);)
    {
        int j;
        for (j=i; j<=CEIL_A(h->end); j++) {
            if ( h->hands[j]<1) {
                break;
            }
        }
        if ( j > i+4  )
        {
            p->type = SINGLE_SERIES;
            p->low = i;
            p->len = j-i;
            return true;
        }

        i=j+1;
    }
    return false;
}

bool COGLordRbtAIClv::getBigBomb(POKER* h, COMBOHAND * p, COMBOHAND * a) //
{
    int total=h->total;
    for (int i=a->low+1; i<=h->end; i++)
    {
        if (h->hands[i] == 4) //bomb got
        {
            p->type = BOMB_old;
            p->low = i;
            return true;
        }
    }
    if (a->low<LIT_JOKER && h->hands[LIT_JOKER]==1 &&h->hands[BIG_JOKER]==1)
    {
        p->type = ROCKET;
        p->low = LIT_JOKER;
        return true;
    }
    return false;
}

bool COGLordRbtAIClv::getBigThree(POKER* h, COMBOHAND * p,COMBOHAND* a) //
{
    for (int i=a->low+1; i<=h->end; i++)
    {
        if (h->hands[i] >= 3) //Got Three
        {
            p->type = THREE;
            p->low  = i;
            p->len = 1;
            return true;
        }
    }
    return false;
}

bool COGLordRbtAIClv::getBigSingle(POKER* h, COMBOHAND * p,int start, int end, int number )
{
    for (int i = start ; i<= end; i++)
    {
        if (h->hands[i] >= number) //Got Three
        {
            p->type = number==3 ? THREE: (number==2 ? PAIRS: SINGLE) ;
            p->low  = i;
            p->len = 1;
            return true;
        }
    }
    return false;
}

bool COGLordRbtAIClv::getBigSeries(POKER* h, COMBOHAND * p,int start, int end, int number, int len ) //
{
    end =CEIL_A(end);
    for (int i = start ; i<= (end -len + 1);)
    {
        int j;
        for (j=i; j<= end; j++) {
            if ( h->hands[j]<number) break;
        }
        if ( j >= i+ len )
        {
            p->type = number==3 ? THREE_SERIES: (number==2 ? PAIRS_SERIES: SINGLE_SERIES) ;
            p->low = i;
            p->len = len;
            return true;
        }
        i=j+1;
    }
    return false;
}

bool COGLordRbtAIClv::getSingleSeries(POKER* h, COMBOHAND * p,int start, int end, int number ) //
{
    end =CEIL_A(end);
    for (int i = start ; i<= (end -4);)
    {
        int j;
        for (j=i; j<= end; j++) {
            if ( h->hands[j]<1) break;
            else if ( j-i >=4 && h->hands[j]>1)
            {
                j++;
                break;
            }
        }
        if ( j >= i+ 5 )
        {
            p->type =  SINGLE_SERIES;
            p->low = i;
            p->len = j-i;
            return true;
        }
        i=j+1;
    }
    return false;
}

int COGLordRbtAIClv::is_series(POKER *h,int number)
{

    for (int i = h->begin; i<= CEIL_A(h->end); i++)
    {
        if ( h->hands[i]!=1) return false;
    }
    return true;
}

int COGLordRbtAIClv::is_doubleseries(POKER *h)
{

    for (int i = h->begin; i<= CEIL_A(h->end); i++)
    {
        if ( h->hands[i]!=2) return false;
    }
    return true;
}

int COGLordRbtAIClv::is_threeseries(POKER *h)
{

    for (int i = h->begin; i<= CEIL_A(h->end); i++)
    {
        if ( h->hands[i]!=3) return false;
    }
    return true;
}

int COGLordRbtAIClv::is_411(POKER *h, COMBOHAND *c )
{
    int j=0;
    c->type = -1;
    // 新四带二规则 wangjian add 2013-05-20
    // 新规则:所带牌必须不同
    int oneCount = 0;
    for (int i = h->begin; i <= CEIL_A(h->end) ; i++){
        if (h->hands[i] != 4 && h->hands[i] != 1 && h->hands[i] != 0)
        {
            return false;
        }
        if(h->hands[i] == 1 ){
            oneCount++ ;
        }
    }
    if (oneCount != 2){
        return false;
    }
    // add end

    for (int i = h->begin; i<= h->end; i++)
    {
        if ( h->hands[i] ==4 )
        {
            c->type = 411;
            c->low= i;
            c->len =1;
            break;
        }
    }
    for (int i = h->begin; i<= (h->end); i++)
    {
        if ( h->hands[i] ==1  )
            c->three_desc[j++] = i;
        else if ( h->hands[i] ==2  )
        {
            c->three_desc[0] = i;
            c->three_desc[1] = i;
        }
    }
    if (c->type == 411)
        return true;
    else
        return false;
}


int COGLordRbtAIClv::is_422(POKER *h, COMBOHAND *c )
{
    int j=0;
    c->type = -1;
    // 新四带二规则 wangjian add 2013-05-20
    // 新规则:所带牌必须不同
    int twoCount = 0;
    for (int i = h->begin; i <= CEIL_A(h->end) ; i++){
        if (h->hands[i] != 4 && h->hands[i] != 2 && h->hands[i] != 0)
        {
            return false;
        }
        if(h->hands[i] == 2 ){
            twoCount++ ;
        }
    }
    if (twoCount != 2){
        return false;
    }
    // add end
    for (int i = h->begin; i<= h->end; i++)
    {
        if ( h->hands[i] ==4 )
        {
            c->type = 422;
            c->low = i;
            break;
        }
    }
    for (int i = h->begin; i<= (h->end); i++)
    {
        if ( h->hands[i] ==2 )
            c->three_desc[j++] = i;
    }
    if ( j != 2)
        c->type = -1;
    if (c->type == 422)
        return true;
    else
        return false;
}


int COGLordRbtAIClv::is_31(POKER *h, COMBOHAND *c )
{
    int j=0;
    c->type = -1;
    for (int i = h->begin; i<= h->end; i++)
    {
        if ( h->hands[i] ==3  )
        {
            c->type = 31;
            c->low = i;
            break;
        }
    }
    for (int i = h->begin; i<= (h->end); i++)
    {
        if ( h->hands[i] ==1 )
            c->three_desc[j++] = i;
    }
    if (c->type == 31)
        return true;
    else
        return false;
}

int COGLordRbtAIClv::is_32(POKER *h, COMBOHAND *c )
{
    int j=0;
    c->type = -1;
    for (int i = h->begin; i<= h->end; i++)
    {
        if ( h->hands[i] ==3 )
        {
            c->type = 32;
            c->low = i;
            break;
        }
    }
    for (int i = h->begin; i<= (h->end); i++)
    {
        if ( h->hands[i] ==2 )
            c->three_desc[j++] = i;
    }
    if (j!=1)
        c->type = -1;
    if (c->type == 32)
        return true;
    else
        return false;
}

int COGLordRbtAIClv::is_3311(POKER *h, COMBOHAND *c )
{
    int j=0;
    c->type = -1;
    // 新飞机规则 wangjian add 2013-05-20
    // 新规则:所带牌必须不同
    for (int i = h->begin; i <= CEIL_A(h->end) ; i++){
        if (h->hands[i] != 3 && h->hands[i] != 1 && h->hands[i] != 0)
        {
            return false;
        }
    }
    // add end
    for (int i = h->begin; i<= CEIL_A(h->end)-1; i++)
    {
        if ( h->hands[i] ==3 && h->hands[i+1] ==3 )
        {
            c->type = 3311;
            c->low = i;
            c->len =2;
            break;
        }
    }

    for (int i = h->begin; i<= (h->end); i++)
    {
        if ( h->hands[i] == 1 )
            c->three_desc[j++] = i;

    }
    if (c->type == 3311)
        return true;
    else
        return false;
}

int COGLordRbtAIClv::is_3322(POKER *h, COMBOHAND *c )
{
    int j=0;
    c->type = -1;
    // 新飞机规则 wangjian add 2013-05-20
    // 新规则:所带牌必须不同
    for (int i = h->begin; i <= CEIL_A(h->end) ; i++){
        if (h->hands[i] != 3 && h->hands[i] != 2 && h->hands[i] != 0)
        {
            return false;
        }
    }
    // add end

    for (int i = h->begin; i<= CEIL_A(h->end)-1; i++)
    {
        if ( h->hands[i] ==3 && h->hands[i+1] ==3 )
        {
            c->type = 3322;
            c->low = i;
            c->len =2;
            break;
        }
    }
    for (int i = h->begin; i<= (h->end); i++)
    {
        if ( h->hands[i] ==2 )
            c->three_desc[j++] = i;
    }
    if (j!=2)
        c->type = -1;
    if (c->type == 3322)
        return true;
    else
        return false;
}

int COGLordRbtAIClv::is_333111(POKER *h, COMBOHAND *c )
{
    int j=0;
    c->type = -1;
    
    // 新飞机规则 wangjian add 2013-05-20
    // 新规则:所带牌必须不同
    int ontCount = 0;
    for (int i = h->begin; i <= CEIL_A(h->end) ; i++){
        if (h->hands[i] != 3 && h->hands[i] != 1 && h->hands[i] != 0)
        {
            return false;
        }
        if(h->hands[i] == 1 ){
            ontCount++ ;
        }
    }
    if (ontCount != 3){
        return false;
    }
    // add end

    for (int i = h->begin; i<= CEIL_A(h->end)-2; i++)
    {
        if ( h->hands[i] ==3 && h->hands[i+1] ==3&& h->hands[i+2] ==3 )
        {
            c->type = 333111;
            c->low = i;
            c->len =3;
            break;
        }
    }


    for (int i = h->begin; i<= (h->end); i++)
    {

        if ( (i<c->low||i>c->low+2) && h->hands[i]>=1 )
        {
            for (int k=0; k<h->hands[i]; k++)
                c->three_desc[j++] = i;
        }
    }
    if (c->type == 333111)
        return true;
    else
        return false;
}

int COGLordRbtAIClv::is_333222(POKER *h, COMBOHAND *c )
{
    int j=0;
    c->type = -1;
    // 新飞机规则 wangjian add 2013-05-20
    // 新规则:所带牌必须不同
    int twoCount = 0;
    for (int i = h->begin; i <= CEIL_A(h->end) ; i++){
        if (h->hands[i] != 3 && h->hands[i] != 2 && h->hands[i] != 0)
        {
            return false;
        }
        if (h->hands[i] == 2) {
            twoCount++;
        }
    }
    if (twoCount != 3) {
        return false;
    }
    // add end
    for (int i = h->begin; i<= CEIL_A(h->end)-2; i++)
    {
        if ( h->hands[i] ==3 && h->hands[i+1] ==3 && h->hands[i+2] ==3 )
        {
            c->type = 333222;
            c->low =i;
            c->len =3;
            break;
        }
    }
    for (int i = h->begin; i<= (h->end); i++)
    {
        if ( h->hands[i] ==2 )
            c->three_desc[j++] = i;
    }
    if ( j!=3)
        c->type = -1;
    if (c->type == 333222)
        return true;
    else
        return false;
}

int COGLordRbtAIClv::is_33331111(POKER *h, COMBOHAND *c )
{
    int j=0;
    c->type = -1;
    // 新飞机规则 wangjian add 2013-05-20
    // 新规则:所带牌必须不同
    int oneCount = 0;
    for (int i = h->begin; i <= CEIL_A(h->end) ; i++){
        if (h->hands[i] != 3 && h->hands[i] != 1 && h->hands[i] != 0)
        {
            return false;
        }
        if(h->hands[i] == 1){
            oneCount++;
        }
    }
    if (oneCount != 4){
        return false;
    }
    // add end
    for (int i = h->begin; i<= CEIL_A(h->end)-3; i++)
    {
        if ( h->hands[i] ==3 && h->hands[i+1] ==3&& h->hands[i+2] ==3 && h->hands[i+3] ==3 )
        {
            c->type = 33331111;
            c->len =4;
            c->low =i;
            break;
        }
    }

    for (int i = h->begin; i<= (h->end); i++)
    {
        if ( (i<c->low||i>c->low+3) && h->hands[i]>=1 )
        {
            for (int k=0; k<h->hands[i]; k++)
                c->three_desc[j++] = i;
        }
    }
    if (c->type == 33331111)
        return true;
    else
        return false;
}

int COGLordRbtAIClv::is_3333311111(POKER *h, COMBOHAND *c )
{
    int j=0;
    c->type = -1;
    
    // 新飞机规则 wangjian add 2013-05-20
    // 新规则:所带牌必须不同
    int oneCount = 0;
    for (int i = h->begin; i <= CEIL_A(h->end) ; i++){
        if (h->hands[i] != 3 && h->hands[i] != 1 && h->hands[i] != 0)
        {
            return false;
        }
        if(h->hands[i] == 1){
            oneCount++;
        }
    }
    if (oneCount != 5){
        return false;
    }
    // add end

    for (int i = h->begin; i<= CEIL_A(h->end)-3; i++)
    {
        if ( h->hands[i] ==3 && h->hands[i+1] ==3&& h->hands[i+2] ==3 && h->hands[i+3] ==3 )
        {
            c->type = 3333311111;
            c->len =5;
            c->low =i;
            break;
        }
    }
    for (int i = h->begin; i<= (h->end); i++)
    {
        if ( (i<c->low||i>c->low+4) && h->hands[i]>=1 )
        {
            for (int k=0; k<h->hands[i]; k++)
            {
                c->three_desc[j++] = i;
            }
        }
    }
    if (c->type == 3333311111)
        return true;
    else
        return false;
}

int COGLordRbtAIClv::is_33332222(POKER *h, COMBOHAND *c )
{
    int j=0;
    c->type = -1;
    // 新飞机规则 wangjian add 2013-05-20
    // 新规则:所带牌必须不同
    int twoCount = 0;
    for (int i = h->begin; i <= CEIL_A(h->end) ; i++){
        if (h->hands[i] != 3 && h->hands[i] != 2 && h->hands[i] != 0)
        {
            return false;
        }
        if(h->hands[i] == 2){
            twoCount++;
        }
    }
    if (twoCount != 4){
        return false;
    }
    // add end

    for (int i = h->begin; i<= CEIL_A(h->end)-3; i++)
    {
        if ( h->hands[i] ==3 && h->hands[i+1] ==3 && h->hands[i+2] ==3  && h->hands[i+3] ==3 )
        {
            c->type = 33332222;
            c->low = i;
            c->len = 4;
            break;
        }
    }
    for (int i = h->begin; i<= (h->end); i++)
    {
        if ( h->hands[i] ==2 )
            c->three_desc[j++] = i;
    }
    if ( j!=4)
        c->type = -1;
    if (c->type == 33332222)
        return true;
    else
        return false;
}

//todo: add a parma number in COMBOHAND
int COGLordRbtAIClv::get_combo_number(COMBOHAND *h)
{
    {
        switch (h->type)
        {
        case ROCKET :
            return 2;
        case SINGLE_SERIES:
            return h->len;
        case PAIRS_SERIES:
            return h->len*2;
        case THREE_SERIES:
            return h->len*3;
        case 31: //31
        case BOMB_old:
            return 4;
        case THREE:
            return 3;
        case PAIRS:
            return 2;
        case SINGLE:
            return 1;

        case 32: //32
            return 5;
        case 411:
            return 6;
        case 422:
        case 3311: //3311
            return 8;
        case 3322: //3322
            return 10;
        case 333111: //333111
            return 12;
        case 333222: //333222
            return 15;
        case 33331111: //333111
            return 16;
            //    case 333222: //333222
        case 3333311111: //333111
        case 33332222: //333222
            return 20;

        default:
            return 0;
        }
    }
}

int COGLordRbtAIClv::is_combo(POKER *h, COMBOHAND * c)
{
#if ALLOW_SAME_KICKER
    return ::is_combo((POKERS*)h, (COMBO_OF_POKERS*)c); // 注意请保持POKER和POKERS，COMBOHAND和COMBO_OF_POKERS的内存结构一样
#endif
    c->type = -1;
    if ( h->total> 0 )
    {
        int        number =h->total;
        c->len = h->total;
        c->low = h->begin ;
        if (number>4)
        {
            if (is_series(h,number))
            {
                c->type=SINGLE_SERIES;
                return true;
            }
            else if (number%2==0 && is_doubleseries(h) )
            {
                c->type=PAIRS_SERIES;
                c->len/=2;
                return true;
            }
            else if (number%3==0 && is_threeseries(h) )
            {
                c->type=THREE_SERIES;
                c->len/=3;
                return true;
            }
        }
        switch (h->total)
        {
        case 1 :
            c->type =SINGLE;
            break;
        case 2 :
            if (h->hands[h->begin]==2) {
                c->type =PAIRS;
                c->len>>=1;
            }
            else if (h->begin == LIT_JOKER) c->type= ROCKET;
            break;
        case 3 :
            if (h->hands[h->begin]==3) c->type =THREE;
            break;
        case 4 :
            if (h->hands[h->begin]==4) c->type =BOMB_old;
            else if (h->hands[h->begin]==3 || h->hands[h->end]==3)
            {
                c->type = 31;
                c->len = 1;
                if (h->hands[h->begin]==3)
                {
                    c->three_desc[0] =   h->end;
                    c->low = h->begin;
                }
                else
                {
                    c->three_desc[0] =   h->begin;
                    c->low = h->end;
                }
            }
            break;
        case 5 :
            is_32(h,c);
            break;
        case 6 :
            is_411(h,c);
            break;
        case 7 :
            break;
        case 8 :
            if ( !is_3311(h,c))
                is_422(h,c);
            break; //3311
        case 9 :
            break;
        case 10 :
            is_3322(h,c);
            break;//3322
        case 11 :
            break;
        case 12 :
            is_333111(h,c);
            break; //333111
        case 13 :
            break;
        case 15 :
            is_333222(h,c);
            break;
        case 16 :
            is_33331111(h,c);
            break; //33331111
        case 20 :
            if (!is_33332222(h,c))
                is_3333311111(h,c);
            break; //33332222
        default:
            break;

        }
    }
    if (c->type!=-1)
        return true;
    else
        return false;
}

//检查本手牌是否最大,即判断是否冲锋套
/************************************************************
1)    套牌张数大于敌方手牌数的最大值时
2)    绝对大的单、对. 
3)    飞机、飞机带翅膀、连对
4)    三带一、三带二，如果主干的三张牌是最大的，则为冲锋套
5)    满足以下条件的顺子是冲锋套：
a)    剩余牌中不存在管住该顺子的牌型时
b)    手牌多的敌人的手牌数与所冲顺子长度的对比小于下表值时
**************************************************************/
bool COGLordRbtAIClv::is_combo_biggest(POKER *opp, COMBOHAND * c , int opp1_num, int opp2_num,int lower)
{
    bool res = true;
    static   char BigSNum[8]={7,9,11,13,15,17,18,18};
    int num = get_combo_number(c);
    //check number
    if (opp1_num <num && opp2_num < num)
        return res;

    switch ( c->type )
    {
    case ROCKET:
    case BOMB_old:
    case PAIRS_SERIES:
    case THREE_SERIES:
    default:
        break;

    case SINGLE_SERIES:
        if ( opp1_num+opp2_num > BigSNum[c->len -5 ])
        {
            int start = c->low+1;
            int len=c->len;
            int end=min_ddz(Pa,opp->end);
            for (int i = start ; i<= (end -len + 1);)
            {
                int j,sum=0;
                for (j=i; j<= end; j++) {
                    if ( opp->hands[j]<1) break;
                }
                if ( j >= i+ len )
                {
                    int loop = j-i-len;
                    for ( int k=0; k<=loop ; k++)
                    {
                        sum = 0;
                        for (int m=i+k; m<j ; m++)
                            if (opp->hands[m] ==1)
                                sum++;
                        if (sum<4)
                            res = false;
                    }
                }
                i=j+1;
            }
        }
        break;

    case THREE_TWO:
    case THREE_ONE:
    case THREE:
		for (int k = c->low + 1; k <= min_ddz(P2, opp->end); k++)
        {
            if ( opp->hands[k]>=3 ) {
                res=false;
                break;
            }
        }
        break;
    case PAIRS :
		for (int k = c->low + 1; k <= min_ddz(P2, opp->end); k++)
        {
            if ( opp->hands[k]>=2 ) {
                res=false;
                break;
            }
        }
        break;
    case SINGLE :
        for (int k=c->low+1; k<=opp->end; k++)
        {
            if ( opp->hands[k]>=1 ) {
                res=false;
                break;
            }
        }
        break;
    }
    return res;
}

int COGLordRbtAIClv::check_combo_a_Big_than_b(COMBOHAND * a, COMBOHAND *b)
{
    if ( b->type == a->type )
    {
        if (b->low< a->low)
            return true;
    }
    else if ( (a->type == BOMB_old && b->type!=ROCKET)
        || a->type == ROCKET)
        return true;
    return false;
}

int COGLordRbtAIClv::get_lowest_controls( POKER *h,int number )
{
    int high = h->end;
    if ( h->total <= number)
        return h->begin;
    int sum=0,j=0,j1=0;
    int i;
    for ( i= high; i>0 && number>0; i--)
    {
        number -=h->hands[i];
        if (number<=0)
            return i;
    }
    return 0;
}

int COGLordRbtAIClv::calc_controls( POKER *h, POKER *opp ,int number )
{
    int nRet = 0;
    int high = opp->end > h->end ? opp->end : h->end;
    int sum=0,j=0,j1=0,HighPokers1[9], HighPoker[9];
    for ( int i= high; i>0 && (j+j1)< number ; i--)
    {
        for (int k=0; k<h->hands[i]; k++)
        {
            HighPoker[j++] = i;
        }
        for (int k=0; k<opp->hands[i]; k++)
        {
            HighPokers1[j1++] = i;
        }
    }
    j--;
    j1--;

    int M=0,N=0;
    int last;
    for ( ; j >= 0 && j1>=0;)
    {
        // simulate round
        last = HighPoker[j];
        j--;
search_0:
        for (int k=j1; k>=0; k--)
            if ( HighPokers1[k]> last)
            {
                last =HighPokers1[k];
                for (int p=k; p<j1 ; p++) //remove poker
                    HighPokers1[p]= HighPokers1[p+1];
                j1--;
                goto search_1;
            }
            N++;
            continue;
search_1:
            for (int k=j; k>=0; k--)
                if ( last < HighPoker[k])
                {
                    last=HighPoker[k];
                    for (int p=k; p<j ; p++) //remove poker
                        HighPoker[p]= HighPoker[p+1];
                    j--;
                    goto search_0;
                }
                M++;
    }
    j=j1>=0?j:j+1;
    if (j<0) j++;

    nRet = ((N+j)*10-M);

    if ( nRet < 0 )
        nRet = 0;

    return nRet;
}

int COGLordRbtAIClv::browse_pokers(POKER *h, COMBOHAND * pCombos)
{
    int num=0;
    for (int i=h->begin; i<=min_ddz(P2,h->end); i++)
    {
        if ( h->hands[i] == 0)
            continue;
        else if ( h->hands[i] == 1)
            pCombos->type = SINGLE;
        else if ( h->hands[i] == 2)
            pCombos->type = PAIRS;
        else if ( h->hands[i] == 3)
            pCombos->type = THREE;
        else if ( h->hands[i] == 4)
            pCombos->type = BOMB_old;

        pCombos->low = i;
        pCombos->len = 1;
        //        pCombos->number = h->hands[i];
        pCombos++;
        num++;
    }
    if ( h->hands[LIT_JOKER]==1 && h->hands[BIG_JOKER]==1 ) {
        pCombos->type = ROCKET;
        pCombos->low= LIT_JOKER;
        pCombos++;
        num++;
    }
    else if (h->hands[LIT_JOKER]==1) {
        pCombos->type = SINGLE;
        pCombos->low = LIT_JOKER;
        pCombos->len = 1;
        pCombos++;
        num++;
    } else if (h->hands[BIG_JOKER]==1) {
        pCombos->type = SINGLE;
        pCombos->len = 1;
        pCombos->low = BIG_JOKER;
        pCombos++;
        num++;
    }
    return num;
}

int COGLordRbtAIClv::search222inSingleSeries(POKER* h, COMBOHAND * p, COMBOHAND * s)
{
    int len = p->len -5;
    int res = 0;
    if (len>=3 )
    {
        if ( h->hands[p->low+1]>=1 && h->hands[p->low+2]>=1 && h->hands[p->low]>=1)
        {
            s->type = PAIRS_SERIES;
            s->len =3;
            s->low = p->low;
            p->len -=3;
            h->hands[p->low]--;
            h->hands[p->low+1]--;
            h->hands[p->low+2]--;
            p->low+=3;

            //found a double series
            if (len >4 && h->hands[p->low]>=1 )
            {
                s->len++;
                p->len--;

                h->hands[p->low]--;
                p->low++;
                if (h->hands[p->low-1]>0) {
                    h->hands[p->low-1]--;
                    p->low--;
                    p->len++;
                    return 1;
                }
            }
            else             if (h->hands[p->low-1]>0) {
                h->hands[p->low-1]--;
                p->low--;
                p->len++;
                return 1;
            }

            return 1;
        }
        else        if ( h->hands[p->low+p->len-1-1]>=1 && h->hands[p->low+p->len-1-2]>=1 && h->hands[p->low+p->len-1]>=1)
        {
            s->type = PAIRS_SERIES;
            s->len =3;
            s->low = p->low+p->len-3;

            h->hands[p->low+p->len-1]--;
            h->hands[p->low+p->len-1-1]--;
            h->hands[p->low+p->len-1-2]--;
            p->len -=3;
            //found a double series
            if (len >4 && h->hands[p->low+p->len-1]>=1 )
            {
                s->len++;
                s->low--;
                h->hands[p->low+p->len-1]--;
                p->len--;
                if (h->hands[p->low+p->len]>0) {
                    h->hands[p->low+p->len]--;
                    p->len++;
                    return 1;
                }
            }
            else                    if (h->hands[p->low+p->len]>0) {
                h->hands[p->low+p->len]--;
                p->len++;
                return 1;
            }
            return 1;
        }
    }
    return res;
}

int  COGLordRbtAIClv::search234inSingleSeries(POKER* h, COMBOHAND * p, COMBOHAND * s) //tobe optimized
{
    //from head to tail.
    while ( p->len >5)
    {
        int end=p->len+p->low-1;
        if ( h->hands[end] > h->hands[p->low] )
        { //remove tail
            h->hands[end]++;
            p->len--;
        }
        else if (h->hands[p->low] >0)
        {
            h->hands[p->low]++;
            p->low++;
            p->len--;
        }
        else
            break;
    }
    //todo:
    return 0;
}


int  COGLordRbtAIClv::searchMultiSingleSeries(POKER* h, COMBOHAND * p) //tobe optimized
{
    COMBOHAND * first = p;
    int num=0;

    // got all 5-series
    for (int i = h->begin; i<= (CEIL_A(h->end) - 4); i++)
    {
        if (getSingleSeries(h, p, i ,  CEIL_A(h->end), 1)) //could be optimized
        {
            remove_combo_poker(h, p, NULL);
            p++;
            num++;
        }
    }
    p=first;
    for (int i=0; i<num; i++) {
        {
            while ( p->len+p->low<=Pa && h->hands[p->len+p->low])
            {
                h->hands[p->len+p->low]--;
                p->len++;
            }
            p++;
        }
    }

    p=first;

    for (int i=0; i<num; i++) {

        if (  i<num-1 && p->low+p->len == (p+1)->low ) //connect
        {
            num--;
            p->len+= (p+1)->len;
            i++;
            if ( num > i )
                memcpy(p+1 ,p+2,sizeof(COMBOHAND)*(num-i));
        }
        else  if (  i<num-2 && p->low+p->len == (p+2)->low ) //connect
        {

            p->len+= (p+2)->len;
            if ( num ==4 )
                memcpy(p+2 ,p+3,sizeof(COMBOHAND)*(1));
            num--;
        }
        else if (  i<num-3 && p->low+p->len == (p+3)->low ) //connect
        {

            p->len+= (p+3)->len;
            num--;
        }
        p++;
    }
    return num;
}

int COGLordRbtAIClv::search_general_1(POKER* h , COMBOHAND * pCombos,
                     bool skip_bomb,bool skip_double, bool skip_three, bool skip_series)
{
    int combo_nums=0;
    int num =0;
    if (!skip_series) {

        num= searchMultiSingleSeries(h,pCombos);
        combo_nums+=num;
        pCombos+=num;
        COMBOHAND * tmp=pCombos-num;
        for (int k=0; k<num; k++)
        {
            int res =  search222inSingleSeries(h,tmp+k,pCombos);
            if (res==1) {
                pCombos++;
                combo_nums++;
            }
            else
                search234inSingleSeries(h,tmp+k,pCombos);
            //     remove_combo_poker(h,pCombos,NULL);
        }
    }
    if (!skip_bomb)
        while ( getBomb(h,pCombos))
        {
            remove_combo_poker(h,pCombos,NULL);
            combo_nums++;
            pCombos++;
        }

        if (!skip_three)
            while ( getThreeSeries(h,pCombos))
            {
                remove_combo_poker(h,pCombos,NULL);
                combo_nums++;
                pCombos++;
            }

            if (!skip_double)
                while ( getDoubleSeries(h,pCombos)) //todo: check three in doubles.
                {
                    remove_combo_poker(h,pCombos,NULL);
                    updateDoubleSeries(h,pCombos);
                    combo_nums++;
                    pCombos++;
                }

                //todo search for three's in series head and tail.
                num = browse_pokers(h,pCombos);
                pCombos+=num;
                combo_nums+=num;
                return combo_nums;
}

int COGLordRbtAIClv::search_general_2(POKER* h , COMBOHAND * pCombos,
                     bool skip_bomb,bool skip_double, bool skip_three, bool skip_series)
{
    int combo_nums=0;
    int num ;
    if (!skip_bomb)
        while ( getBomb(h,pCombos))
        {
            remove_combo_poker(h,pCombos,NULL);
            combo_nums++;
            pCombos++;
        }

        if (!skip_three)
            while ( getThreeSeries(h,pCombos))
            {
                remove_combo_poker(h,pCombos,NULL);
                combo_nums++;
                pCombos++;
            }
            if (!skip_series)    {

                num = searchMultiSingleSeries(h,pCombos);
                combo_nums+=num;
                pCombos+=num;
                COMBOHAND * tmp=pCombos-num;
                for (int k=0; k<num; k++)
                {
                    int res =  search222inSingleSeries(h,tmp+k,pCombos);
                    if (res==1) {
                        pCombos++;
                        combo_nums++;
                    }
                    else
                        search234inSingleSeries(h,tmp+k,pCombos);
                    //     remove_combo_poker(h,pCombos,NULL);
                }
            }

            if (!skip_double)
                //may be removed.
                while ( getDoubleSeries(h,pCombos)) //todo: check three in doubles.
                {
                    remove_combo_poker(h,pCombos,NULL);
                    updateDoubleSeries(h,pCombos);
                    combo_nums++;
                    pCombos++;
                }

                //todo search for three's in series head and tail.
                num = browse_pokers(h,pCombos);
                pCombos+=num;
                combo_nums+=num;

                return combo_nums;
}

int COGLordRbtAIClv::search_general_3(POKER* h , COMBOHAND * pCombos,
                     bool skip_bomb,bool skip_double, bool skip_three, bool skip_series)
{
    int combo_nums=0;
    int num;
    if (!skip_bomb)
        while ( getBomb(h,pCombos))
        {
            remove_combo_poker(h,pCombos,NULL);
            combo_nums++;
            pCombos++;
        }

        if (!skip_three)
            while ( getThreeSeries(h,pCombos))
            {
                remove_combo_poker(h,pCombos,NULL);
                combo_nums++;
                pCombos++;
            }

            if (!skip_double)
                while ( getDoubleSeries(h,pCombos))
                {
                    remove_combo_poker(h,pCombos,NULL);
                    updateDoubleSeries(h,pCombos);
                    combo_nums++;
                    pCombos++;
                }

                if (!skip_series)
                {
                    num = searchMultiSingleSeries(h,pCombos);
                    combo_nums+=num;
                    pCombos+=num;
                    COMBOHAND * tmp=pCombos-num;
                    for (int k=0; k<num; k++)
                    {
                        int res =  search222inSingleSeries(h,tmp+k,pCombos);
                        if (res==1) {
                            pCombos++;
                            combo_nums++;
                        }
                        else
                            search234inSingleSeries(h,tmp+k,pCombos);
                    }

                }
                //todo search for three's in series head and tail.
                num = browse_pokers(h,pCombos);
                pCombos+=num;
                combo_nums+=num;

                return combo_nums;
}

//1. bomb 2. three 3. straight 4. else
int COGLordRbtAIClv::search_general_4(POKER* h , COMBOHAND * pCombos,
                     bool skip_bomb,bool skip_double, bool skip_three, bool skip_series)
{
    int combo_nums=0;
    int num =0;

    if (!skip_bomb)
        while ( getBomb(h,pCombos))
        {
            remove_combo_poker(h,pCombos,NULL);
            combo_nums++;
            pCombos++;
        }
        while ( getThree(h,pCombos))
        {
            remove_combo_poker(h,pCombos,NULL);
            combo_nums++;
            pCombos++;
        }

        if (!skip_series) {

            num= searchMultiSingleSeries(h,pCombos);
            combo_nums+=num;
            pCombos+=num;
            COMBOHAND * tmp=pCombos-num;
            for (int k=0; k<num; k++)
            {
                int res =  search222inSingleSeries(h,tmp+k,pCombos);
                if (res==1) {
                    pCombos++;
                    combo_nums++;
                }
                else
                    search234inSingleSeries(h,tmp+k,pCombos);
            }
        }
        //todo search for three's in series head and tail.
        num = browse_pokers(h,pCombos);
        pCombos+=num;
        combo_nums+=num;

        return combo_nums;
}

COMBOHAND*  COGLordRbtAIClv::find_max_len_in_combos(COMBOHAND* combos, int total)
{
    COMBOHAND* cur=NULL;
    int    max= 0;
    for (int k=0; k<total; k++)
    {
        if (combos[k].type!=NOTHING_old)
        {
            int tmp;
            (tmp=get_combo_number(&combos[k]));
            if (tmp>max) {
                max=tmp;
                cur= combos+k;
            }
            else if (tmp == max && max>1 && combos[k].low < cur->low)
            {
                max=tmp;
                cur= combos+k;
            }
            else if (tmp == max && max==1 && combos[k].low > cur->low)
            {
                max=tmp;
                cur= combos+k;
            }
        }
    }
    return cur;
}

COMBOHAND*  COGLordRbtAIClv::find_biggest_in_combos(COMBOHAND* combos, int total)
{
    COMBOHAND* cur=NULL;
    int    max= 0;
    for (int k=0; k<total; k++)
    {
        if (combos[k].type!=NOTHING_old)
        {
            if (combos[k].low>=max) {
                max=combos[k].low;
                cur= combos+k;
            }
        }
    }
    return cur;
}

//todo: you could sort not_biggest first...

/***********************
出所有套牌中标签最小的一套
如果两套牌标签相同时，出张数多的一套
************************/
COMBOHAND*  COGLordRbtAIClv::find_smallest_in_combos(COMBOHAND* com, int total,SUITS* suit ,bool not_search_single)
{
    COMBOHAND* cur=NULL;

    COMBOS_SUMMARY * s= suit->summary;
    //    COMBOHAND* combos=&s->not_biggest[0];
    int    max=0;
    if ( !not_search_single) {
        if ( s->not_biggest_num >0) {
            cur= s->not_biggest[0];
            max= s->not_biggest[0]->low;
        }
        for (int k=1; k<s->not_biggest_num; k++)
        {
            if (s->not_biggest[k]->type!=NOTHING_old )
            {
                if (s->not_biggest[k]->low>max) {}
                else if (s->not_biggest[k]->low<max) {
                    max=s->not_biggest[k]->low;
                    cur= s->not_biggest[k];
                }
                else if ( get_combo_number(s->not_biggest[k])> get_combo_number(cur)) {
                    max=s->not_biggest[k]->low;
                    cur=s->not_biggest[k];
                }
            }
        }
        if ( cur == NULL) // search biggest
        { //max len..
            cur= s->biggest[0];
            max= s->biggest[0]->low;
            if (s->biggest[0]->type==BOMB_old)
                max=LIT_JOKER;
            if(s->biggest[0]->type==NOTHING_old)
                max=BIG_JOKER;
            for (int k=0; k<s->biggest_num; k++)
            {
                if (s->biggest[k]->type!=NOTHING_old )
                {
                    if (s->biggest[k]->low>max) {
                    }
                    else if (s->biggest[k]->low<max) {
                        max=s->biggest[k]->low;
                        cur= s->biggest[k];
                    }
                    else if ( get_combo_number(s->biggest[k])> get_combo_number(cur)) {
                        max=s->biggest[k]->low;
                        cur=s->biggest[k];
                    }
                }
            }
        }
    }
    else
    {
        int k;
        for (k=0; k<s->not_biggest_num; k++)
        {
            if (s->not_biggest[k]->type!=NOTHING_old &&  s->not_biggest[k]->type!=SINGLE)
            {
                max=s->not_biggest[k]->low;
                cur= s->not_biggest[k];
                k++;
                break;
            }
        }
        if (k == s->not_biggest_num) k = 0;
        for (; k<s->not_biggest_num; k++)
        {
            if (s->not_biggest[k]->type!=NOTHING_old &&  s->not_biggest[k]->type!=SINGLE)
            {
                if (s->not_biggest[k]->low>max) {}
                else if (s->not_biggest[k]->low<max) {
                    max=s->not_biggest[k]->low;
                    cur= s->not_biggest[k];
                }
                else if ( get_combo_number(s->not_biggest[k])< get_combo_number(cur)) {
                    max=s->not_biggest[k]->low;
                    cur=s->not_biggest[k];
                }
            }
        }
#if 0
        if ( cur == NULL) // search biggest
        { //max len..
            cur= s->biggest[0];
            max= s->biggest[0]->low;
            if (s->biggest[0]->type==BOMB_old)
                max=LIT_JOKER;
            for (int k=0; k<s->biggest_num; k++)
            {
                if (s->biggest[k]->type!=NOTHING_old )
                {
                    if (s->biggest[k]->low>max) {
                    }
                    else if (s->biggest[k]->low<max) {
                        max=s->biggest[k]->low;
                        cur= s->biggest[k];
                    }
                    else if ( get_combo_number(s->biggest[k])> get_combo_number(cur)) {
                        max=s->biggest[k]->low;
                        cur=s->biggest[k];
                    }
                }
            }
        }
#endif 
    }
    return cur;
}


int COGLordRbtAIClv::find_combo_with_3_controls_in_combos(COMBOHAND* com, int total,SUITS* suit)
{
    COMBOHAND* cur=NULL;

    COMBOS_SUMMARY * s= suit->summary;

    int    max=0;
    cur= s->not_biggest[0];
    max= s->not_biggest[0]->low;
    for (int k=0; k<s->not_biggest_num; k++)
    {
        if (s->not_biggest[k]->type!=NOTHING_old )
        {
            if (s->not_biggest[k]->low>max) {}
            else if (s->not_biggest[k]->low<max) {
                max=s->not_biggest[k]->low;
                cur= s->not_biggest[k];
            }
            else if ( get_combo_number(s->not_biggest[k])< get_combo_number(cur)) {
                max=s->not_biggest[k]->low;
                cur=s->not_biggest[k];
            }
        }
    }
    return 0;
}

int COGLordRbtAIClv::get_control_poker_num_in_combo(COMBOHAND* c, int lower)
{
    switch ( c->type )
    {
    case ROCKET:
        return 0;
    case BOMB_old:
        return 0;
    case SINGLE_SERIES:
        if ( c->low + c->len - 1  >=lower)
            return c->low+c->len-lower;
    case PAIRS_SERIES:
        if ( c->low + c->len - 1  >=lower)
            return 2*(c->low+c->len-lower);
    case THREE_SERIES:
        if ( c->low + c->len - 1  >=lower)
            return 3*(c->low+c->len-lower);
    case 31:
        return 3*( c->low >=lower) + c->three_desc[0]>=lower ;
    case 32:
        return ( c->low >=lower)*3 + 2*(c->three_desc[0]>=lower);
#if 0
    case THREE:
        if ( c->low >=lower)
            return 3;
    case PAIRS :

        if ( c->low >=lower)
            return 2;
    case SINGLE :
        if ( c->low >=lower)
            return 1;
#else
    case THREE:  //donot check three,pairs,single
    case PAIRS :
    case SINGLE :
        return 0;
#endif
    case 3311:
    case 333111:
    case 33331111:
    case 3333311111: {
        int     num =0;
        if ( c->low + c->len - 1  >=lower)  //todo fixme...
            num = 3*(c->low+c->len-lower);
        for (int i=0; i<c->len; i++)
        {
            num += c->three_desc[i]>=lower;
        }
        return num;
                     }
    case 3322:
    case 333222:
    case 33332222:
        {
            int     num =0;
            if ( c->low + c->len - 1  >=lower)  //todo fixme...
                num = 3*(c->low+c->len-lower);
            for (int i=0; i<c->len; i++)
            {
                num += 2*(c->three_desc[i]>=lower);
            }
            return num;
        }
    case 411:
        {
            int     num =0;
            if ( c->low >=lower)  //todo fixme...
                num = 4;
            for (int i=0; i<2; i++)
            {
                num += (c->three_desc[0]>=lower);
            }
            return num;
        }
    case 422:
        {
            int     num =0;
            if ( c->low >=lower)  //todo fixme...
                num = 4;
            for (int i=0; i<2; i++)
            {
                num += 2*(c->three_desc[0]>=lower);
            }
            return num;
        }
    }

    return 0;
}

//to be optimized
//current for
void COGLordRbtAIClv::sort_all_combos(COMBOHAND * c , int total, COMBOS_SUMMARY * s,
                     POKER *opp, int opp1, int opp2, /* for check biggest*/
                     int lower  /*for check control,not used now*/
                     )
{
    memset(s,0,sizeof(COMBOS_SUMMARY));//todo: optimized

    //统计
#pragma region compute
    for (int k=0; k<total; k++)
    {
        switch (c[k].type)
        {
        case ROCKET:
        case BOMB_old:
            s->bomb[s->bomb_num]=&c[k];
            s->bomb_num++;
            break;
        case SINGLE_SERIES:
            s->series[s->series_num]=&c[k];
            s->series_num++;
            s->series_detail[c[k].len -5]++;
            break;
        case PAIRS_SERIES:
            s->pairs_series[s->pairs_series_num]=&c[k];
            s->pairs_series_num++;
            break;
        case THREE_SERIES:
            s->threeseries[s->threeseries_num]=&c[k];
            s->threeseries_num++;
            break;
        case THREE:
            s->three[s->three_num]=&c[k];
            s->three_num++;
            break;
        case THREE_ONE:
            s->three_one[s->three_one_num]=&c[k];
            s->three_one_num++;
            break;
        case THREE_TWO:
            s->three_two[s->three_two_num]=&c[k];
            s->three_two_num++;
            break;
        case PAIRS :
            s->pairs[s->pairs_num]=&c[k];
            s->pairs_num++;
            break;
        case SINGLE :
            s->singles[s->singles_num]=&c[k];
            s->singles_num++;
            break;
        case 3311:
        case 333111:
        case 33331111:
        case 3333311111:
            s->threeseries_one[s->threeseries_one_num++]=&c[k];
            break;
        case 3322:
        case 333222:
        case 33332222:
            s->threeseries_two[s->threeseries_two_num++]=&c[k];
            break;
        case 411:
            s->four_one[s->four_one_num++]=&c[k];
            break;
        case 422:
            s->four_two[s->four_two_num++]=&c[k];
            break;
        default:
            break;
        case NOTHING_old:
            break;
        }
    }
#pragma endregion compute

    //之前只查找了3同张，在这里是为其加上后面带的牌
    // refine for three;
    int j,num=s->threeseries_num;
    for (j=0; j<num; j++) //todo: spilt a pair..
    {
       long int type1[6]={0,31,3311,333111,33331111,3333311111};
        int type2[5]={0,32,3322,333222,33332222};
        if ( s->threeseries[j]->len <= s->singles_num)
        {
            for (int k=0; k< s->threeseries[j]->len ; k++ )
            {

                s->threeseries[j]->three_desc[k] = s->singles[k]->low;


                s->singles[k]->type =NOTHING_old;
            }
            s->singles_num-=s->threeseries[j]->len ;
            memmove(s->singles, s->singles + s->threeseries[j]->len, (s->singles_num)*sizeof(void*));
            s->threeseries_one[j]= s->threeseries[j];
            s->threeseries[j]->type = type1[s->threeseries[j]->len];
            s->threeseries_one_num ++;
            s->threeseries_num --;
        }
        else if (s->threeseries[j]->len <= s->pairs_num)
        {
            for (int k=0; k< s->threeseries[j]->len ; k++ )
            {
                s->threeseries[j]->three_desc[k] = s->pairs[k]->low;
                s->pairs[k]->type =NOTHING_old;
            }
            s->pairs_num-=s->threeseries[j]->len ;
            memmove(s->pairs, s->pairs + s->threeseries[j]->len, (s->pairs_num)*sizeof(void*));
            s->threeseries_two[j]= s->threeseries[j];
            s->threeseries[j]->type = type2[s->threeseries[j]->len];
            s->threeseries_two_num ++;
            s->threeseries_num --;
        }
        else if(s->threeseries[j]->len <= s->pairs_num*2 + s->singles_num )
        {
            for (int k=0; k< s->threeseries[j]->len ;  )
            {
                if(s->singles_num>0 && s->pairs_num>0 )                    
                {
                    if ( k== (s->threeseries[j]->len -1) ||s->singles[0]->low>s->pairs[0]->low )
                    {
__pad_single:
                        s->threeseries[j]->three_desc[k++] = s->singles[0]->low;
                        s->singles[0]->type =NOTHING_old;
                        s->singles_num-- ;
                        memmove(s->singles, s->singles + 1, (s->singles_num)*sizeof(void*));
                    }
                    else
                    {
__pad_single_in_pair:
                        s->threeseries[j]->three_desc[k++] = s->pairs[0]->low;
                        if(k==s->threeseries[j]->len)
                        {
                            s->pairs[0]->type= SINGLE;
                            s->pairs_num --; 
                            memmove(s->pairs, s->pairs + 1, (s->pairs_num)*sizeof(void*));
                            s->singles_num ++;
                            for(int single=s->singles_num ; single>0; single--)
                                s->singles[single]=s->singles[single-1];
                            s->singles[0]=s->pairs[0];
                            break;
                        }
                        s->threeseries[j]->three_desc[k++] = s->pairs[0]->low;
                        s->pairs[0]->type =NOTHING_old;
                        s->pairs_num-- ;
                        memmove(s->pairs, s->pairs + 1, (s->pairs_num)*sizeof(void*));
                    }
                }
                else if (s->singles_num>0)
                {
                    goto __pad_single;
                }
                else if(s->pairs_num>0)
                {
                    goto __pad_single_in_pair;
                }

            } 
            s->threeseries_one[j]= s->threeseries[j];
            s->threeseries[j]->type = type1[s->threeseries[j]->len];
            s->threeseries_one_num ++;
            s->threeseries_num --;            
        }
    }

    if (s->threeseries_num >0 ) //strange
    {
        memmove(s->threeseries, s->threeseries + num - s->threeseries_num, (s->threeseries_num)*sizeof(void*));
    }

    for (j=0; j<s->three_num; j++) //todo: spilt a pair..
    {
        if ( j<s->singles_num)
        {
            s->three_one[j]= s->three[j];
            s->three[j]->type = 31;
            s->three[j]->three_desc[0] = s->singles[j]->low;
            s->three_one_num ++;
            s->singles[j]->type =NOTHING_old;
        }
        else if (j-s->singles_num < s->pairs_num)
        {
            //search single first
            s->three_two[j-s->singles_num]= s->three[j];
            s->three[j]->type = 32;
            s->three[j]->three_desc[0] = s->pairs[j-s->singles_num]->low;
            s->three_two_num = j-s->singles_num;
            s->pairs[j-s->singles_num]->type =NOTHING_old;
        }
        else //todo refine
            break;
    }

    if (j<s->three_num)
    {
        memmove(s->three, s->three + j, (s->three_num - j)*sizeof(void*));
        s->three_num =(s->three_num-j);
        s->singles_num = 0;
        s->pairs_num = 0;
    }
    else if (s->three_num > 0 )
    {
        if ( s->three_num <= s->singles_num)
        {
            memmove(s->singles, s->singles + s->three_num, (s->singles_num - s->three_num)*sizeof(void*));
            s->singles_num =(s->singles_num-s->three_num);
        }
        else
        {

            int number = s->three_num - s->singles_num ;
            s->singles_num = 0;
            memmove(s->pairs, s->pairs + number, (s->pairs_num - number)*sizeof(void*));
            s->pairs_num -= number;
        }
        s->three_num = 0;
    }

    s->real_total_num = 0;
    s->extra_bomb_ctrl     = 0;
    s->combo_with_2_controls = 0;
    s->real_total_num = 0;
    s->biggest_num = 0;
    s->extra_bomb_ctrl =0;
    s->not_biggest_num =0;
    s->combo_with_2_controls = 0;
    for (int k=0; k<total; k++)
    {
        if (c[k].type!=NOTHING_old)
        {
            s->real_total_num++;
            if ( is_combo_biggest(opp,c+k,opp1,opp2,lower) )
            {
                c[k].control = 1;
                s->biggest[s->biggest_num++] = &c[k];
                if (c[k].type==BOMB_old && c[k].low<lower)
                    s->extra_bomb_ctrl ++;
            }
            else
            {
                s->not_biggest[s->not_biggest_num++] = &c[k];
            }
            if ( get_control_poker_num_in_combo    (c+k,lower)>=2 && c[k].type!=SINGLE_SERIES )
                s->combo_with_2_controls ++;
        }
    }

    int series_type=0;
    if (s->series_num>0)
    {
        for (int k=0; k<10; k++)
            series_type+= s->series_detail[k]!=0;
    }
    s->combo_typenum =     (s->pairs_num!=0)  +  series_type
        + (s->singles_num!=0) + (s->three_num!=0) + (s->three_two_num!=0)
        + (s->three_one_num!=0) + (s->pairs_series_num!=0) + (s->threeseries_num!=0)
        + (s->threeseries_two_num!=0)+ (s->threeseries_one_num!=0)+(s->four_one_num!=0)
        +(s->four_two_num!=0)
        ;
    s->combo_smallest_single = s->singles_num>0?s->singles[0]->low:0;

    //add by fsy 2012_2_23
    int three_flag = 0,series_flag = 0,three1_flag = 0,three2_flag = 0;
    int three_low = 15,series_low = 15,three1_low = 15,three2_low = 15;
    int three_res = -1,series_res = -1,three1_res = -1,three2_res = -1,series_len = 0;
    for(int i = 0;i<s->biggest_num;i++)
    {
        if(s->biggest[i]->type == SINGLE_SERIES)
        {
            for(int j = 0;j< s->biggest[i]->len;j++)
            {
                if(opp->hands[s->biggest[i]->low + j] == 0)
                {
                    series_flag = 1;
                    series_len = s->biggest[i]->len;
                }
            }            
        }
        else if(s->biggest[i]->type == THREE)
        {
            three_flag = 1;
        }
        else if(s->biggest[i]->type == THREE_ONE)
        {
            three1_flag = 1;
        }
        else if(s->biggest[i]->type == THREE_TWO)
        {
            three2_flag = 1;
        }
    }
    for(int i = 0;i<s->not_biggest_num;i++)
    {
        if(three_flag == 1 && s->not_biggest[i]->type == THREE )
        {
            if(three_low > s->not_biggest[i]->low )
            {
                three_low = s->not_biggest[i]->low;
                three_res = i;
            }
        }
        else if(three1_flag == 1 && s->not_biggest[i]->type == THREE_ONE )
        {
            if(three1_low > s->not_biggest[i]->low )
            {
                three1_low = s->not_biggest[i]->low;
                three1_res = i;
            }
        }
        else if(three2_flag == 1 && s->not_biggest[i]->type == THREE_TWO )
        {
            if(three2_low > s->not_biggest[i]->low )
            {
                three2_low = s->not_biggest[i]->low;
                three2_res = i;
            }
        }
        else if(series_flag == 1 && s->not_biggest[i]->type == SINGLE_SERIES && s->not_biggest[i]->len == series_len) 
        {
            if(series_low > s->not_biggest[i]->low )
            {
                series_low = s->not_biggest[i]->low;
                series_res = i;
            }
        }
    }
    if(three_res != -1)
    {
        s->biggest[s->biggest_num++] = s->not_biggest[three_res];
        memmove(&s->not_biggest[three_res], &s->not_biggest[three_res + 1], (s->not_biggest_num - three_res - 1)*sizeof(COMBOHAND*));
        s->not_biggest[s->not_biggest_num-1] = NULL;
        s->not_biggest_num--;
        if(series_res != -1) series_res--;
        if(three1_res != -1) three1_res--;
        if(three2_res != -1) three2_res--;
    }
    if(series_res != -1)
    {
        s->biggest[s->biggest_num++] = s->not_biggest[series_res];
        memmove(&s->not_biggest[series_res], &s->not_biggest[series_res + 1], (s->not_biggest_num - series_res - 1)*sizeof(COMBOHAND*));
        s->not_biggest[s->not_biggest_num-1] = NULL;
        s->not_biggest_num--;
        if(three1_res != -1) three1_res--;
        if(three2_res != -1) three2_res--;
    }
    if(three1_res != -1)
    {
        s->biggest[s->biggest_num++] = s->not_biggest[three1_res];
        memmove(&s->not_biggest[three1_res], &s->not_biggest[three1_res + 1], (s->not_biggest_num - three1_res - 1)*sizeof(COMBOHAND*));
        s->not_biggest[s->not_biggest_num-1] = NULL;
        s->not_biggest_num--;
        if(three2_res != -1) three2_res--;
    }
    if(three2_res != -1)
    {
        s->biggest[s->biggest_num++] = s->not_biggest[three2_res];
        memmove(&s->not_biggest[three2_res], &s->not_biggest[three2_res + 1], (s->not_biggest_num - three2_res - 1)*sizeof(COMBOHAND*));
        s->not_biggest[s->not_biggest_num-1] = NULL;
        s->not_biggest_num--;
    }
    s->combo_total_num = s->real_total_num - s->biggest_num + s->combo_with_2_controls;

    // 场外有n种牌没见过，手中有m个冲锋套，若n>m,则手中套数+m；若n<m，则手中套数+n 
    int out_bomb = 0;
    for(int i = opp->begin;i<opp->end;i++)
    {
        if(opp->hands[i] == 4)
        {
            out_bomb++;
        }
    }
    s->combo_total_num = s->real_total_num - s->biggest_num + s->combo_with_2_controls + min_ddz(out_bomb,s->biggest_num);
}

//update summary of combos
void COGLordRbtAIClv::update_summary(COMBOS_SUMMARY *s, POKER*h, POKER * opp, COMBOHAND * c,int total, int opp1,int opp2,int lower)
{
    s->real_total_num = 0;
    s->biggest_num = 0;
    s->extra_bomb_ctrl =0;
    s->not_biggest_num =0;
    s->combo_with_2_controls = 0;
    for (int k=0; k<total; k++)
    {
        if (c[k].type!=NOTHING_old)
        {
            s->real_total_num++;
            if ( is_combo_biggest(opp,c+k,opp1,opp2,lower) )
            {
                c[k].control = 1;
                s->biggest[s->biggest_num++] = &c[k];
                if (c[k].type==BOMB_old && c[k].low<lower)
                    s->extra_bomb_ctrl ++;
            }
            else
            {
                c[k].control = 0;
                s->not_biggest[s->not_biggest_num++] = &c[k];

            }
            if ( get_control_poker_num_in_combo    (c+k,lower)>=2  && c[k].type!=SINGLE_SERIES)
                s->combo_with_2_controls ++;
        }
    }
    s->combo_total_num = s->real_total_num - s->biggest_num + s->combo_with_2_controls;
    int series_type=0;
    if (s->series_num>0)
    {
        for (int k=0; k<10; k++)
            series_type+= s->series_detail[k]!=0;
    }
    s->combo_typenum =     (s->pairs_num!=0)  +  series_type
        + (s->singles_num!=0) + (s->three_num!=0) + (s->three_two_num!=0)
        + (s->three_one_num!=0) + (s->pairs_series_num!=0) + (s->threeseries_num!=0)
        + (s->threeseries_two_num!=0)+ (s->threeseries_one_num!=0)+(s->four_one_num!=0)
        +(s->four_two_num!=0)
        ;
    s->combo_smallest_single = s->singles_num>0?s->singles[0]->low:0;
}

int COGLordRbtAIClv::cmp_summary_for_min_single_farmer( COMBOS_SUMMARY *a,COMBOS_SUMMARY *b) //return 1 if a is better than b
{
    if ( a->singles_num <=1  )
        return true;
    else if ( b->singles_num>=2 && a->singles[1] >= b->singles[1] )
        return true;
    else
        return false;
}

/*
i.    控制 - 套的数目 最少。
ii.    套的种类最少
iii.    单牌数量最少
iv.    最小的单牌最大
*/
int COGLordRbtAIClv::cmp_summary( COMBOS_SUMMARY *a,COMBOS_SUMMARY *b) //return a>b
{
    if (    (a->combo_total_num-a->extra_bomb_ctrl< b->combo_total_num-b->extra_bomb_ctrl )
        )
        return true;
    else    if (    a->combo_total_num-a->extra_bomb_ctrl > b->combo_total_num-b->extra_bomb_ctrl
        )
        return false;
    else  if (a->combo_total_num-a->extra_bomb_ctrl == b->combo_total_num-b->extra_bomb_ctrl
        )
    {
        if (a->combo_typenum < b->combo_typenum)
            return true;
        else if (a->combo_typenum > b->combo_typenum)
            return false;
        else {
            if (a->singles_num > b->singles_num )
                return false;
            else if (a->singles_num < b->singles_num )
                return true;
            else
                return a->combo_smallest_single > b->combo_smallest_single;
        }
    }
    return 0;
}

int COGLordRbtAIClv::search_combos_in_hands(POKER* h , COMBOHAND * pCombos, COMBOS_SUMMARY *pSummary,SUITS * suit)
{
    int combo_nums=0;
    //serach four
    if ( (h->end-h->begin < 2 || h->end-h->begin < 4 )&& h->total <4  )
    {
        combo_nums =  browse_pokers(h,pCombos);;
        return combo_nums;
    }
    else if ( h->end-h->begin < 4 )
    {
        POKER t;
        memcpy(&t,h,sizeof(POKER));
        h= &t;
        while ( getBomb(h,pCombos))
        {
            remove_combo_poker(h,pCombos,NULL);
            combo_nums++;
            pCombos++;
        }

        if ( getThreeSeries(h,pCombos))
        {
            remove_combo_poker(h,pCombos,NULL);
            combo_nums++;
            pCombos++;
        }
        if ( getDoubleSeries(h,pCombos)) //todo: check three in doubles.
        {
            remove_combo_poker(h,pCombos,NULL);
            updateDoubleSeries(h,pCombos);
            combo_nums++;
            pCombos++;
        }

        int num=browse_pokers(h,pCombos);
        combo_nums+=num;
        pCombos+=num;

        sort_all_combos(pCombos-combo_nums,combo_nums,pSummary,
            suit->opps,suit->oppDown_num,suit->oppUp_num, suit->lower_control_poker);
        return combo_nums;
    }
    else
    {
        COMBOHAND *presult,*pNow,*pTmp; //todo: use alloce
        COMBOS_SUMMARY *cur,*result,*tmp;
        presult=(COMBOHAND*) malloc(sizeof(COMBOHAND)*20); //comtmp[0][0];
        pNow=(COMBOHAND*) malloc(sizeof(COMBOHAND)*20);
        memset(presult,0, sizeof(COMBOHAND)*20); //comtmp[0][0];
        memset(pNow,0 ,sizeof(COMBOHAND)*20);


        result= (COMBOS_SUMMARY *)malloc(sizeof(COMBOS_SUMMARY));
        cur=(COMBOS_SUMMARY *)malloc(sizeof(COMBOS_SUMMARY));
        memset(cur,0,sizeof(COMBOS_SUMMARY));
        memset(result,0,sizeof(COMBOS_SUMMARY));

        int num=0,numRes=0;
        POKER t;

        memcpy(&t,h,sizeof(POKER));
        numRes= result->combo_total_num  = search_general_1( &t,presult,0,0,0,0);
        sort_all_combos(presult,numRes, result,suit->opps,suit->oppDown_num,suit->oppUp_num, suit->lower_control_poker);

        memcpy(&t,h,sizeof(POKER));
        num= cur->combo_total_num = search_general_2( &t,pNow,0,0,0,0);
        sort_all_combos(pNow,num, cur,suit->opps,suit->oppDown_num,suit->oppUp_num, suit->lower_control_poker);

        if ( cmp_summary(cur,result))
        {
            numRes = num;
            pTmp = presult;
            presult=pNow;
            pNow=pTmp;
            tmp=result;
            result= cur;
            cur=tmp;
        }

        memcpy(&t,h,sizeof(POKER));
        num = cur->combo_total_num = search_general_3( &t,pNow,0,0,0,0);
        sort_all_combos(pNow,num,cur,suit->opps,suit->oppDown_num,suit->oppUp_num, suit->lower_control_poker);

        if ( cmp_summary(cur,result))
        {
            numRes = num;
            pTmp = presult;
            presult=pNow;
            pNow=pTmp;

            tmp=result;
            result= cur;
            cur=tmp;
        }
        // for (int k=0; k<numRes; k++)
        memcpy(pCombos,presult,numRes*sizeof(COMBOHAND));

        free(presult);
        free(pNow);
        free(cur);
        free(result);
        return numRes;
    }
}

int COGLordRbtAIClv::get_2nd_min_singles(COMBOS_SUMMARY *s )
{
    if (s->singles_num <=1 )
        return BIG_JOKER;
    else {
        return s->singles[1]->low;
    }
}

//地主报单农民组牌方式
int COGLordRbtAIClv::search_combos_in_suits_for_min_single_farmer(POKER* h , COMBOHAND * pCombos,SUITS * suit)
{
    memset(pCombos,0, sizeof(COMBOHAND)*20);

    COMBOHAND *pNow;
    COMBOS_SUMMARY *cur,*result;
    pNow=(COMBOHAND*) malloc(sizeof(COMBOHAND)*20);
    memset(pNow,0, sizeof(COMBOHAND)*20);

    result=suit->summary;
    cur=(COMBOS_SUMMARY *)malloc(sizeof(COMBOS_SUMMARY));
    memset(cur,0,sizeof(COMBOS_SUMMARY));
    //        memset(result,0,sizeof(COMBOS_SUMMARY));

    POKER t;
    memcpy(&t,h,sizeof(POKER));
    int num  = search_general_1( &t,pCombos,0,1,0,0);
    sort_all_combos(pCombos,num, result,suit->opps,suit->oppDown_num,suit->oppUp_num, suit->lower_control_poker);

    memcpy(&t,h,sizeof(POKER));
    int num1 = search_general_4( &t,pNow,0,1,0,0);
    sort_all_combos(pNow,num1, cur,suit->opps,suit->oppDown_num,suit->oppUp_num, suit->lower_control_poker);

    if ( cur->singles_num < result->singles_num
        || (cur->singles_num == result->singles_num
        &&  get_2nd_min_singles(cur) > get_2nd_min_singles(result) )
        )
    {
		memcpy(pCombos, pNow, max_ddz(num1, num)*sizeof(COMBOHAND));
        num =num1;        
        sort_all_combos(pCombos,num, result,suit->opps,suit->oppDown_num,suit->oppUp_num, suit->lower_control_poker);
    }

    free(pNow);
    free(cur);
    return num;
}

/*
Get the
*/
int COGLordRbtAIClv::search_combos_in_suit( POKER *h , POKER *opp, SUITS* suit )
{
    suit->max_control.single = calc_controls(h, opp, cacl_control_poker_num(game));

    COMBOHAND * combos=suit->combos;
    COMBOS_SUMMARY *pcombo_summary, combo_summary;
    pcombo_summary = & combo_summary;
    for (int k=0; k<20; k++)
    {
        combos[k].type=NOTHING_old;;
    }
    int number = search_combos_in_hands( h , combos , pcombo_summary,suit);
    sort_all_combos(suit->combos,number,suit->summary,suit->opps,suit->oppDown_num
        ,suit->oppUp_num,suit->lower_control_poker);
    //check_biggest_in_all_combos(suit->combos,suit->summary,opp,suit->oppUp_num,suit->oppDown_num);
    //get_pairs(hands_combo);
    return true;
}

//之前只查找了3同张，在这里是为其加上后面带的牌
//get a single or pair for  three
//and update the summary
int COGLordRbtAIClv::search_1_2_for_a_three(COMBOHAND * p ,POKER*h, SUITS* suit,COMBOHAND * c)
{
    suit->need_research = 0;
    COMBOS_SUMMARY *s =suit->summary;
    if (p->type == THREE_ONE )
    {
        if (suit->summary->singles_num>0) {
            c->type=THREE_ONE;
            c->three_desc[0] = suit->summary->singles[0]->low;
            remove_combo_poker(h,suit->summary->singles[0],NULL);
            s->singles_num -- ;
            if (s->singles[0]->control!=1)
                s->combo_total_num --;
            else
                s->biggest_num --;
            s->real_total_num --;
            s->singles[0]->type=NOTHING_old;
            memmove(s->singles,s->singles+1,(s->singles_num)*sizeof(void*));
        }
        else if (suit->summary->series_num>0 && s->series[0]->len>5)
        {
            c->type=THREE_ONE;
            c->three_desc[0]= s->series[0]->low;
            s->series[0]->low++;
            s->series[0]->len--;
        }
        else if (suit->summary->pairs_num>0)
        {
            c->type=THREE_ONE;
            c->three_desc[0]= s->pairs[0]->low;
            s->pairs[0]->type=SINGLE;
            h->hands[ s->pairs[0]->low]-=1;
            suit->need_research = 1;
        }
        else {
            for ( int i=h->begin; i<=h->end; i++)
                if ( h->hands[i]>0&& i!=c->low)
                {
                    c->type=THREE_ONE;
                    c->three_desc[0]= i;
                    h->hands[i]--;
                    suit->need_research = 1;
                    return 1;
                }
        }
    }

    else if (p->type == THREE_TWO )
    {
        if (suit->summary->pairs_num>0)
        {
            c->type=THREE_TWO;
            c->three_desc[0]= s->pairs[0]->low;
            s->pairs[0]->type=NOTHING_old;
            memmove(s->pairs,s->pairs+1,(s->pairs_num-1)*sizeof(void*));
        }
        else {
			for (int i = h->begin; i <= max_ddz(P2, h->end); i++)
                if ( h->hands[i]>=2 && i!=c->low)
                {
                    c->type=THREE_TWO;
                    c->three_desc[0]= i;
                    h->hands[i]-=2;
                    suit->need_research = 1;
                    return 1;
                }
                return 0;
        }
    }

    else  if (p->type == 3322 )
    {
        if (suit->summary->pairs_num>1)
        {
            c->type=3322;
            c->three_desc[0]= s->pairs[0]->low;
            c->three_desc[1]= s->pairs[1]->low;
            s->pairs[0]->type=NOTHING_old;
            s->pairs[1]->type=NOTHING_old;
            memmove(s->pairs,s->pairs+2,(s->pairs_num-2)*sizeof(void*));
        }
        else {
            int j=0;
			for (int i = h->begin; i <= max_ddz(P2, h->end); i++) {
                if ( h->hands[i]>=2 && i!=c->low)
                {
                    c->three_desc[j++]= i;
                    if (j>=2)
                    {
                        c->type = 3322;
                        suit->need_research = 1;
                        h->hands[c->three_desc[0]]  -=2;
                        h->hands[c->three_desc[1]]  -=2;
                        return 1;
                    }
                }
            }
            return 0;
        }
    }

    if (p->type == 3311 )
    {
        if (suit->summary->singles_num>1) {
            c->type=3311;
            c->three_desc[0]= suit->summary->singles[0]->low;
            c->three_desc[1]= suit->summary->singles[1]->low;

            remove_combo_poker(h,suit->summary->singles[0],NULL);
            remove_combo_poker(h,suit->summary->singles[1],NULL);
            s->singles_num -=2 ;
            if (s->singles[0]->control!=1)
                s->combo_total_num --;
            else
                s->biggest_num --;
            if (s->singles[1]->control!=1)

                s->combo_total_num --;
            else
                s->biggest_num --;

            s->real_total_num -=2;
            s->singles[0]->type=NOTHING_old;
            s->singles[1]->type=NOTHING_old;
            memmove(s->singles,s->singles+2,(s->singles_num)*sizeof(void*));
        }
        else if (suit->summary->pairs_num>0)
        {
            c->type=3311;
            c->three_desc[0]= s->pairs[0]->low;
            c->three_desc[1]= s->pairs[0]->low;
            s->pairs[0]->type=NOTHING_old;
            memmove(s->pairs,s->pairs+1,(s->pairs_num-1)*sizeof(void*));
        }
        else {
            int j=0;
            for ( int i=h->begin; i<=h->end; i++)
                if (h->hands[i]>0)
                {
                    c->type=3311;
                    c->three_desc[j++]= i;
                    h->hands[i]--;
                    if (j==2)
                        break;
                }
                suit->need_research = 1;
                //            search_combos_in_suit(h,suit->opps,suit);
                return 0;
        }
    }
    return 1;
}

//search in poker h to find a combo c bigger than a
// do not search bombs for non-bomb combo hand
int COGLordRbtAIClv::find_a_bigger_combo_in_hands(SUITS* suit,POKER* h, COMBOHAND * c, COMBOHAND* a)
{
    //= suit->h;
    if (h->total<get_combo_number(a))
        return false;
    switch ( a->type )
    {
    case ROCKET:
        ;
        return false;
    case BOMB_old:
        return false;
        if (getBigBomb(h,c,a))
        {
            return true;
        }
        return false;
    case SINGLE_SERIES:
        if (getBigSeries(h,c,a->low+1,CEIL_A(h->end),1,a->len)) //todo: fix me
        {
            return true;
        }
        break;

    case PAIRS_SERIES:
        if (getBigSeries(h,c,a->low+1,CEIL_A(h->end),2,a->len)) //todo: fix me
        {
            return true;
        }
        break;
    case THREE_SERIES:
        if (getBigSeries(h,c,a->low+1,CEIL_A(h->end),3,a->len))//todo: fix me
        {
            return true;
        }
        break;
    case 3311:
    case 333111:
    case 33331111:
    case 3333311111:
        if (getBigSeries(h,c,a->low+1,CEIL_A(h->end),3,a->len))//todo: fix me
        {
            return true;
        }
        break;
    case 3322:
    case 333222:
    case 33332222:
        if (getBigSeries(h,c,a->low+1,CEIL_A(h->end),3,a->len))//todo: fix me
        {
            return true;
        }
        break;
    case 31:
    case 32:
    case THREE:
        if (getBigThree(h,c,a)) //todo: fix me
        {
            return true;
        }
        break;
    case PAIRS :
        if (getBigSingle(h,c,a->low+1,h->end,2)) //todo: fix me
        {
            return true;
        }
        break;
    case SINGLE :
        if (getBigSingle(h,c,a->low+1,h->end,1)) //todo: fix me
        {
            return true;
        }
    default:
        break;
    }
    return false;
}

//a stupid fucntion...
//select  a combo c from suit->h, but not remove
//arrange the other poker in suit->h to combos and save summary to suit->summary
int    COGLordRbtAIClv::rearrange_suit(SUITS *suit,COMBOHAND* c)
{
    POKER t;
    memcpy(&t,suit->h,sizeof(POKER));
    int res=remove_combo_poker(&t,c,NULL);
    search_combos_in_suit(&t,suit->opps,suit);
    return res;
}

// Remove combo a from the suit s
// return true if success
int COGLordRbtAIClv::remove_combo_in_suit( COMBOS_SUMMARY *s , COMBOHAND* a)
{
    //   COMBOS_SUMMARY *s =suit->summary;
    switch ( a->type )
    {
    case ROCKET:
    case BOMB_old:
        if (s->bomb_num>0)
        {
            for (int k=0; k<s->bomb_num; k++)
            {
                if ( s->bomb[k]->low == a->low) {
                    //memcpy(c,s->bomb[k],sizeof(COMBOHAND));
                    if (k!=s->bomb_num-1)
                        memmove(&s->bomb[k], &s->bomb[k + 1], (s->bomb_num - k)*sizeof(void*));
                    s->bomb_num --;
                    return true;
                }
            }
        }
        break;
    case SINGLE_SERIES:
        if (s->series_num>0)
        {
            for (int k=0; k<s->series_num; k++)
            {
                if ( s->series[k]->low == a->low && s->series[k]->len == a->len) {
                    // memcpy(c,s->series[k],sizeof(COMBOHAND));
                    if (k!=s->series_num-1)
                        memmove(&s->series[k], &s->series[k + 1], (s->series_num - k)*sizeof(void*));
                    s->series_num --;
                    s->series_detail[a->len-5]--;
                    return true;
                }
            }
        }
        break;
    case PAIRS_SERIES:
        if (s->pairs_series_num>0)
        {
            for (int k=0; k<s->pairs_series_num; k++)
            {
                if ( s->pairs_series[k]->low == a->low && s->pairs_series[k]->len == a->len) {
                    // memcpy(c,s->series[k],sizeof(COMBOHAND));
                    if (k!=s->pairs_series_num-1)
                        memmove(&s->pairs_series[k], &s->pairs_series[k + 1], (s->pairs_series_num - k)*sizeof(void*));
                    s->pairs_series_num --;
                    //s->series_detail[a->len-5]--;
                    return true;
                }
            }
        }
        break;

    case THREE_SERIES:
        if (s->threeseries_num>0)
        {
            for (int k=0; k<s->threeseries_num; k++)
            {
                if ( s->threeseries[k]->low == a->low && s->threeseries[k]->len == a->len) {
                    // memcpy(c,s->series[k],sizeof(COMBOHAND));
                    if (k!=s->threeseries_num-1)
                        memmove(&s->threeseries[k], &s->threeseries[k + 1], (s->threeseries_num - k)*sizeof(void*));
                    s->threeseries_num --;
                    //s->series_detail[a->len-5]--;
                    return true;

                }
            }
        }
        break;
    case THREE:
        for (int k=0; k<s->three_num; k++)
        {
            if ( s->three[k]->low == a->low ) {
                // memcpy(c,s->series[k],sizeof(COMBOHAND));
                if (k!=s->three_num-1)
                    memmove(&s->three[k], &s->three[k + 1], (s->three_num - k)*sizeof(void*));
                s->three_num --;
                return true;

            }
        }
        break;

        break;
    case 31:
        if (s->three_one_num>0)
        {
            for (int k=0; k<s->three_one_num; k++)
            {
                if ( s->three_one[k]->low == a->low  ) {
                    // memcpy(c,s->three_one[k],sizeof(COMBOHAND));
                    if (k!=s->three_one_num-1)
                        memmove(&s->three_one[k], &s->three_one[k + 1], (s->three_one_num - k)*sizeof(void*));
                    s->three_one_num --;
                    return true;

                }
            }
        }
        break;
    case 32:
        if (s->three_two_num>0)
        {
            for (int k=0; k<s->three_two_num; k++)
            {
                if ( s->three_two[k]->low == a->low  ) {
                    //  memcpy(c,s->three_two[k],sizeof(COMBOHAND));
                    if (k!=s->three_two_num-1)
                        memmove(&s->three_two[k], &s->three_two[k + 1], (s->three_two_num - k)*sizeof(void*));
                    s->three_two_num --;
                    return true;

                }
            }
        }
        break;
    case PAIRS :
        if (s->pairs_num>0)
        {
            for (int k=0; k<s->pairs_num; k++)
            {
                if ( s->pairs[k]->low == a->low  ) {
                    //  memcpy(c,s->pairs[k],sizeof(COMBOHAND));
                    if (k!=s->pairs_num-1)
                        memmove(&s->pairs[k], &s->pairs[k + 1], (s->pairs_num - k)*sizeof(void*));
                    s->pairs_num --;
                    return true;
                }
            }
        }
        break;
    case SINGLE :
        if (s->singles_num>0)
        {
            for (int k=0; k<s->singles_num; k++)
            {
                if ( s->singles[k]->low == a->low  ) {
                    //memcpy(c,s->singles[k],sizeof(COMBOHAND));
                    if (k!=s->singles_num-1)
                        memmove(&s->singles[k], &s->singles[k + 1], (s->singles_num - k)*sizeof(void*));
                    s->singles_num --;
                    return true;
                }
            }
        }
        break;
    case 3311:
    case 333111:
    case 33331111:
    case 3333311111:
        if (s->threeseries_one_num>0)
        {
            for (int k=0; k<s->threeseries_one_num; k++)
            {
                if ( s->threeseries_one[k]->low == a->low  ) {
                    //memcpy(c,s->singles[k],sizeof(COMBOHAND));
                    if (k!=s->threeseries_one_num-1)
                        memmove(&s->threeseries_one[k], &s->threeseries_one[k + 1], (s->threeseries_one_num - k)*sizeof(void*));
                    s->threeseries_one_num --;
                    return true;
                }
            }
        }
        break;
    case 3322:
    case 333222:
    case 33332222:
        if (s->threeseries_two_num>0)
        {
            for (int k=0; k<s->threeseries_two_num; k++)
            {
                if ( s->threeseries_two[k]->low == a->low  ) {
                    if (k!=s->threeseries_two_num-1)
                        memmove(&s->threeseries_two[k], &s->threeseries_two[k + 1], (s->threeseries_two_num - k)*sizeof(void*));
                    s->threeseries_two_num --;
                    return true;
                }
            }
        }
        break;
    case 411:
        {
            for (int k=0; k<s->four_one_num; k++)
            {
                if ( s->four_one[k]->low == a->low  ) {
                    if (k!=s->four_one_num-1)
                        memmove(&s->four_one[k], &s->four_one[k + 1], (s->four_one_num - k)*sizeof(void*));
                    s->four_one_num --;
                    return true;
                }
            }
        }
        break;

    case 422:
        {
            for (int k=0; k<s->four_two_num; k++)
            {
                if ( s->four_two[k]->low == a->low  ) {
                    if (k!=s->four_one_num-1)
                        memmove(&s->four_two[k], &s->four_two[k + 1], (s->four_two_num - k)*sizeof(void*));
                    s->four_two_num --;
                    return true;
                }
            }
        }
        break;

    default:
        break;
    }
    return false;
}

COMBOHAND* COGLordRbtAIClv::find_combo( COMBOS_SUMMARY *s , COMBOHAND* a)//find c > a in s and return c.
{
    switch ( a->type )
    {
    case ROCKET:
        return NULL;
    case BOMB_old:
        if (s->bomb_num>0)
        {
            for (int k=0; k<s->bomb_num; k++)
            {
                if ( s->bomb[k]->low > a->low) {
                    return s->bomb[k];
                }
            }
        }
        break;
    case SINGLE_SERIES:
        if (s->series_num>0)
        {
            for (int k=0; k<s->series_num; k++)
            {
                if ( s->series[k]->low > a->low && s->series[k]->len == a->len) {
                    return s->series[k];
                }
            }
        }
        break;
    case PAIRS_SERIES:
        if (s->pairs_series_num>0)
        {
            for (int k=0; k<s->pairs_series_num; k++)
            {
                if ( s->pairs_series[k]->low > a->low && s->pairs_series[k]->len == a->len) {
                    return s->pairs_series[k];
                }
            }
        }
        break;
    case THREE_SERIES:
        if (s->threeseries_num>0)
        {
            for (int k=0; k<s->threeseries_num; k++)
            {
                if ( s->threeseries[k]->low > a->low && s->threeseries[k]->len == a->len) {
                    return s->threeseries[k];
                }
            }
        }
        break;
        break;
    case THREE:
        if (s->three_num>0)
        {
            for (int k=0; k<s->three_num; k++)
            {
                if ( s->three[k]->low > a->low && s->three[k]->len == a->len) {
                    return s->three[k];
                }
            }
        }
        break;
    case 31:
        if (s->three_one_num>0)
        {
            for (int k=0; k<s->three_one_num; k++)
            {
                if ( s->three_one[k]->low > a->low  ) {
                    return s->three_one[k];
                }
            }
        }
        break;
    case 32:
        if (s->three_two_num>0)
        {
            for (int k=0; k<s->three_two_num; k++)
            {
                if ( s->three_two[k]->low > a->low  ) {
                    return s->three_two[k];
                }
            }
        }
        break;
    case PAIRS :
        if (s->pairs_num>0)
        {
            for (int k=0; k<s->pairs_num; k++)
            {
                if ( s->pairs[k]->low > a->low  ) {
                    return s->pairs[k];
                }
            }
        }
        break;
    case SINGLE :
        if (s->singles_num>0)
        {
            for (int k=0; k<s->singles_num; k++)
            {
                if ( s->singles[k]->low > a->low  ) {
                    return s->singles[k];
                }
            }
        }
        break;
    case 3311:
    case 333111:
    case 33331111:
    case 3333311111:
        if (s->threeseries_one_num>0)
        {
            for (int k=0; k<s->threeseries_one_num; k++)
            {
                if ( s->threeseries_one[k]->low > a->low && s->threeseries_one[k]->len == a->len) {
                    return s->threeseries_one[k];
                }
            }
        }
        break;
    case 3322:
    case 333222:
    case 33332222:
        if (s->threeseries_one_num>0)
        {
            for (int k=0; k<s->threeseries_two_num; k++)
            {
                if ( s->threeseries_two[k]->low > a->low && s->threeseries_two[k]->len == a->len) {
                    return s->threeseries_two[k];
                }
            }
        }
        break;
    }
    if (a->type!=BOMB_old && s->bomb_num>0)
    {
        for (int k=0; k<s->bomb_num; k++)
        {
            return s->bomb[k];
        }
    }
    return NULL;
}

int COGLordRbtAIClv::has_control_poker(COMBOHAND* c, int lower)
{
    if (c->type == BOMB_old|| c->type ==ROCKET)
        return c->low;
    if (c->len ==0)
    {
        c->len=1;
    }
    if ( c->low + c->len -1 >= lower ) //for theries , pairs, single
        return c->low + c->len -1;
    if ( c->type == THREE_ONE || c->type == THREE_TWO )
    {
        if ( c->three_desc[0] >=lower )
            return c->three_desc[0];
    }
    if ( c->type >=3311 ) //plane , trick..
    {
        if ( c->three_desc[0] >=lower ||  c->three_desc[1] >=lower )
			return max_ddz(c->three_desc[0], c->three_desc[1]);
    }
    return -1;
}

//remove 1 or 2 pokers from opps hand
void COGLordRbtAIClv::assume_remove_controls_in_opps(POKER * opps, COMBOHAND * c, int begin)
{
    if (c->type==SINGLE)       {
        for (int k=begin+1; k<=opps->end; k++)
            if (  opps->hands[k]>0)
                opps->hands[k]--;
    }
    else if ( c->type==PAIRS)
    {
        for (int k=begin+1; k<=opps->end; k++)
            if (  opps->hands[k]>=2)
                opps->hands[k]-=2;

        if (  opps->hands[LIT_JOKER]==1 &&opps->hands[BIG_JOKER]==1 )
            opps->hands[LIT_JOKER]=opps->hands[BIG_JOKER]=0;

    }
}

int COGLordRbtAIClv::check_poker_suit(POKER*hand ,SUITS * suit)
{
    POKER t,*h=&t;
    memcpy(h,hand,sizeof(POKER));
    for (int k=0; k<20; k++)
    {
        if (suit->combos[k].type!=NOTHING_old) {
            if ( remove_combo_poker(h,&suit->combos[k],NULL )==false)
                return 0;
        }
    }
    if (h->total ==0)
        return 1;
    else
        return 0;
}

int COGLordRbtAIClv::find_max(COMBOS_SUMMARY* s)
{
    int max=s->singles_num;
    if (s->pairs_num>max) max=s->pairs_num;
    if (s->three_num>max) max=s->three_num;
    if (s->series_num>max) max=s->series_num;
    if (s->three_one_num+s->three_two_num>max) max=s->three_one_num+s->three_two_num;
    if (s->series_num>max) max=s->series_num;

    return max;
}

int COGLordRbtAIClv::rand_a_poker(POKER *all)
{
    int k;
    while (1)
    {
        k=rand()%MAX_POKER_KIND;
        if (all->hands[k]>0)
        {
            all->hands[k]--;
            return k;
        }
    }
}

void COGLordRbtAIClv::full_poker(POKER *h)
{
    for (int i=0; i<MAX_POKER_KIND; i++)
    {
        h->hands[i]  = i>=LIT_JOKER?1:4;
    }
    h->end=BIG_JOKER;
    h->begin=P2;
    h->total=54;
}

//find and remove a joker in combos, 31 3311... 411
int COGLordRbtAIClv::find_joker_in_combos(SUITS* suit, int joker)
{
    if (suit->summary->singles_num>0 &&  suit->summary->singles[suit->summary->singles_num-1]->type==joker )
    {
        suit->summary->singles_num--;
        suit->summary->singles[suit->summary->singles_num]->type=NOTHING_old;
    }
    else if (suit->summary->three_one_num>1 && suit->summary->three_one[suit->summary->three_one_num-1]->three_desc[0] == joker)
    {
        suit->summary->three_one_num--;
        suit->summary->three[suit->summary->three_num++]=suit->summary->three_one[suit->summary->three_one_num];
        suit->summary->three_one[suit->summary->three_one_num]->type = THREE;
    }
    else {
        return 0;
    }

    return 1;
}

int COGLordRbtAIClv::card_to_poker(int nCard)
{
    int nWhat = 0;
    if ( nCard >= 52)
    {
        nWhat = nCard - 52 + 13;
    }
    else
    {
        nWhat = nCard % 13;
    }

    return nWhat;
}

bool COGLordRbtAIClv::isBiggestBomb(int p, POKER* opp)
{
    if (p == LIT_JOKER)
    {
        return true;
    }
    if (opp->hands[LIT_JOKER] > 0 && opp->hands[BIG_JOKER] > 0)
    {
        return false;
    }
    for (int i=p+1; i<P2; i++)
    {
        if (opp->hands[i] == 4)
        {
            return false;
        }
    }
    return true;
}

int COGLordRbtAIClv::maxBombCount(POKER* opp)
{
    int bc = 0;
    for (int i=P3; i<=P2; i++)
    {
        if (opp->hands[i] == 4)
        {
            bc++;
        }
    }
    if (opp->hands[LIT_JOKER] > 0 && opp->hands[BIG_JOKER] > 0)
    {
        bc++;
    }
    return bc;
}

void COGLordRbtAIClv::checkKicker(COMBOHAND *hand, COMBOS_SUMMARY *summary)
{    
    if (hand->type == THREE_ONE)
    {
        if (summary->threeseries_one_num > 0)
        {
            if (summary->threeseries_one[0]->three_desc[0] < hand->three_desc[0])
            {
                int k = summary->threeseries_one[0]->three_desc[0];
                for (int i=0; i<summary->threeseries_one[0]->len-1; i++)
                {
                    summary->threeseries_one[0]->three_desc[i] = summary->threeseries_one[0]->three_desc[i + 1];
                }
                summary->threeseries_one[0]->three_desc[summary->threeseries_one[0]->len-1] = hand->three_desc[0];
                hand->three_desc[0] = k;
            }            
        }
        else if (summary->three_one_num > 0)
        {
            int lowestK = 0;
            for (int i=1; i<summary->three_one_num; i++)
            {
                if (summary->three_one[i]->three_desc[0] < summary->three_one[lowestK]->three_desc[0])
                {
                    lowestK = i;
                }
            }
            if (summary->three_one[lowestK]->three_desc[0] < hand->three_desc[0])
            {
                int k = summary->three_one[lowestK]->three_desc[0];
                summary->three_one[lowestK]->three_desc[0] = hand->three_desc[0];
                hand->three_desc[0] = k;
            }
        }
    }
    if (hand->type == THREE_TWO)
    {
        if (summary->threeseries_two_num > 0)
        {
            if (summary->threeseries_two[0]->three_desc[0] < hand->three_desc[0])
            {
                int k = summary->threeseries_two[0]->three_desc[0];
                for (int i=0; i<summary->threeseries_two[0]->len-1; i++)
                {
                    summary->threeseries_two[0]->three_desc[i] = summary->threeseries_two[0]->three_desc[i + 1];
                }
                summary->threeseries_two[0]->three_desc[summary->threeseries_two[0]->len-1] = hand->three_desc[0];
                hand->three_desc[0] = k;
            }            
        }
        else if (summary->three_two_num > 0)
        {
            int lowestK = 0;
            for (int i=1; i<summary->three_two_num; i++)
            {
                if (summary->three_two[i]->three_desc[0] < summary->three_two[lowestK]->three_desc[0])
                {
                    lowestK = i;
                }
            }
            if (summary->three_two[lowestK]->three_desc[0] < hand->three_desc[0])
            {
                int k = summary->three_two[lowestK]->three_desc[0];
                summary->three_two[lowestK]->three_desc[0] = hand->three_desc[0];
                hand->three_desc[0] = k;
            }
        }
    }
}

//By fsy 2012-2-9
int COGLordRbtAIClv::isStrongGood(SUITS* suit){

    int n=suit->summary->combo_total_num;
    int count=0;
    int biggest_num=suit->summary->biggest_num;
    int not_biggest_num = suit->summary->not_biggest_num;
    char unBigSingle[MAX_POKER_KIND]; 
    char changedPairs[MAX_POKER_KIND]; //remember the break pairs
    for(int i=0;i<MAX_POKER_KIND;i++){
        //0 is a position,-1 is a magic number
        unBigSingle[i]=-1;
        changedPairs[i]=-1;
    }
    //unBigSingle save the position of combo in not_biggest
    for(int j =0; j < suit->summary->not_biggest_num ;j++)
    {
        if(suit->summary->not_biggest[j]->type == SINGLE){
            unBigSingle[suit->summary->not_biggest[j]->low]=j;
        }
    }

    int max_flag_single=-1; //checked or not
    int max_flag_pairs = -1;
    for(int i = 0;i<MAX_POKER_KIND;i++)
    {
        bool isNew = true;
        if(unBigSingle[i] != -1){
            int j=0;
            for(;j<biggest_num;j++)
            {
                if(max_flag_single<j 
                    && suit->summary->biggest[j]->type == SINGLE 
                    && suit->opps->end < suit->summary->biggest[j]->low ) 
                {                        
                    //add into biggest
                    suit->summary->biggest[suit->summary->biggest_num++] = suit->summary->not_biggest[unBigSingle[i]];

                    //delete from notBiggerst
                    if(suit->summary->not_biggest_num >1)
                    {
                        suit->summary->not_biggest[unBigSingle[i]]=suit->summary->not_biggest[suit->summary->not_biggest_num-1];

                        //update the unBigSingle
                        if(suit->summary->not_biggest[suit->summary->not_biggest_num-1]->type == SINGLE)
                        {
                            unBigSingle[suit->summary->not_biggest_num-1] = unBigSingle[i];
                        }                    
                    }
                    else
                    {
                        suit->summary->not_biggest[unBigSingle[i]] = NULL;
                    }
                    suit->summary->combo_total_num--;
                    suit->summary->not_biggest_num--;
                    unBigSingle[i]=-1;
                    isNew=false;
                    max_flag_single=j;
                    count++;
                    break;
                }
            }

            if(j >= biggest_num-1 && isNew)
            {                
                for(int m=0;m<biggest_num;m++)
                {
                    if(suit->summary->biggest[m]->type == PAIRS 
                        && changedPairs[suit->summary->biggest[m]->low]<1
                        && suit->opps->end < suit->summary->biggest[m]->low )
                    {
                        if(max_flag_pairs<m)
                        {
                            //add into biggest
                            suit->summary->biggest[suit->summary->biggest_num++] = suit->summary->not_biggest[unBigSingle[i]];

                            //delete from notBiggerst
                            if(suit->summary->not_biggest_num >1)
                            {
                                suit->summary->not_biggest[unBigSingle[i]]=suit->summary->not_biggest[suit->summary->not_biggest_num-1];

                                //update the unBigSingle
                                if(suit->summary->not_biggest[suit->summary->not_biggest_num-1]->type == SINGLE)
                                {
                                    unBigSingle[suit->summary->not_biggest_num-1] = unBigSingle[i];
                                }
                            }
                            else
                            {
                                suit->summary->not_biggest[unBigSingle[i]] = NULL;
                            }
                            suit->summary->combo_total_num--;
                            suit->summary->not_biggest_num--;
                            unBigSingle[i]=-1;
                            if(changedPairs[suit->summary->biggest[m]->low] == 0)
                            {
                                max_flag_pairs=m;
                            }
                            count++;
                            changedPairs[suit->summary->biggest[m]->low]++;
                            break;
                        }
                    }
                }
            }
        }
    }
    for(int j =0; j < suit->summary->not_biggest_num ;j++)
    {
        if(suit->summary->not_biggest[j]->type == PAIRS)
        {
            for(int m=0;m<biggest_num;m++)
            {
                if(max_flag_pairs<m && suit->summary->biggest[m]->type == PAIRS && changedPairs[suit->summary->biggest[m]->low] == -1)
                {
                    //add into biggest
                    suit->summary->biggest[suit->summary->biggest_num++] = suit->summary->not_biggest[j];

                    if(suit->summary->not_biggest_num >1)
                    {
                        //delete from notBiggerst
                        suit->summary->not_biggest[j]=suit->summary->not_biggest[suit->summary->not_biggest_num-1];
                    }
                    else
                    {
                        suit->summary->not_biggest[j] = NULL;
                    }
                    suit->summary->not_biggest_num--;
                    suit->summary->combo_total_num--;

                    max_flag_pairs=m;
                    count++;
                    break;
                }
            }
        }                
    }

    //整理多余的牌型
    for(int i = suit->summary->not_biggest_num;i<not_biggest_num;i++)
    {
        suit->summary->not_biggest[i] = NULL;
    }

    if(isDangous(suit)){
        if(count>n) return true;
    }else{
        if(count>= n-1) return true;
    }
    return false;
}

int COGLordRbtAIClv::isDangous(SUITS* suit)
{
    if(suit->opps->hands[LIT_JOKER]>0 && suit->opps->hands[BIG_JOKER]>0) 
    {
        return true;
    }
    else {
        bool find=true;
        for(int k=0;k<MAX_POKER_KIND;k++)
        {
            if(suit->opps->hands[k] == 4)
            {
                find = false;
                for(int i=0;i<suit->summary->bomb_num;i++){
                    if(    suit->summary->bomb[i]->low > k){
                        find=true;
                    }
                }
            }
        }
        return !find;
    } 
}

bool COGLordRbtAIClv::RbtInSetGrabLord(int argSeat){return TRUE;}

bool COGLordRbtAIClv::RbtOutGetGrabLord(bool &grabLord, int& delay) {
    
    if(level >= NEW_AI_START_LEVEL)
    {
        int i = RobotshowCard(robot);
        grabLord=i>0?(rand()%2==0):0;
        delay = 500 + rand()% 1000; //todo update

        return true;
    }
    grabLord=0;
    return TRUE;
}

bool COGLordRbtAIClv::RbtOutDoubleGame(bool &Double_game, int &delay)
{
    if(level >= NEW_AI_START_LEVEL)
    {
        int i = doubleGame(robot);
        Double_game= i;
        delay = 500 + rand()% 500; //todo update
        return true;
    }
    Double_game=0;
    delay = 0;
    return TRUE;
}

bool COGLordRbtAIClv::RbtOutGetLastError(int &errorCode){return TRUE;}
