-- MySQL dump 10.13  Distrib 5.1.73, for redhat-linux-gnu (x86_64)
--
-- Host: localhost    Database: chess_sysdata
-- ------------------------------------------------------
-- Server version	5.1.73

/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET @OLD_CHARACTER_SET_RESULTS=@@CHARACTER_SET_RESULTS */;
/*!40101 SET @OLD_COLLATION_CONNECTION=@@COLLATION_CONNECTION */;
/*!40101 SET NAMES utf8 */;
/*!40103 SET @OLD_TIME_ZONE=@@TIME_ZONE */;
/*!40103 SET TIME_ZONE='+00:00' */;
/*!40014 SET @OLD_UNIQUE_CHECKS=@@UNIQUE_CHECKS, UNIQUE_CHECKS=0 */;
/*!40014 SET @OLD_FOREIGN_KEY_CHECKS=@@FOREIGN_KEY_CHECKS, FOREIGN_KEY_CHECKS=0 */;
/*!40101 SET @OLD_SQL_MODE=@@SQL_MODE, SQL_MODE='NO_AUTO_VALUE_ON_ZERO' */;
/*!40111 SET @OLD_SQL_NOTES=@@SQL_NOTES, SQL_NOTES=0 */;

--
-- Table structure for table `activity`
--

DROP TABLE IF EXISTS `activity`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `activity` (
  `acid` int(10) NOT NULL AUTO_INCREMENT COMMENT '活动id',
  `atype` int(10) NOT NULL DEFAULT '1' COMMENT '活动类型',
  `sortno` int(10) NOT NULL COMMENT '排序',
  `eventno` int(10) NOT NULL COMMENT '事件编号',
  `name` varchar(20) NOT NULL COMMENT '活动名称',
  `desc` text NOT NULL COMMENT '活动描述',
  `content` text NOT NULL COMMENT '活动配置信息',
  `btime` int(10) NOT NULL COMMENT '开始时间(0表示立即开启)',
  `etime` int(10) NOT NULL COMMENT '结束时间(0表示永久)',
  `imageurl` varchar(200) NOT NULL COMMENT '活动图片地址',
  `status` tinyint(1) NOT NULL COMMENT '状态(0不生效1生效)',
  PRIMARY KEY (`acid`)
) ENGINE=MyISAM AUTO_INCREMENT=8 DEFAULT CHARSET=utf8 ROW_FORMAT=DYNAMIC;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `activity`
--

LOCK TABLES `activity` WRITE;
/*!40000 ALTER TABLE `activity` DISABLE KEYS */;
INSERT INTO `activity` VALUES (1,1,1,1,'充值赠送','充值赠送','{\"srate\":0.2,\"gc\":15,\"pid\":0}',1467907200,1487088000,'activity/a3.png',1),(4,3,3,3,'首冲翻倍','翻倍','{\"srate\":1,\"gc\":0,\"pid\":1}',1467907200,1486137600,'activity/a3.png',1),(5,2,2,2,'救济金','救济金','{\"srate\":1,\"gc\":5,\"pid\":0,\"times\":1}',1467907200,1486137600,'activity/a3.png',1);
/*!40000 ALTER TABLE `activity` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `agent`
--

DROP TABLE IF EXISTS `agent`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `agent` (
  `agid` int(10) NOT NULL AUTO_INCREMENT COMMENT '代理商id',
  `name` varchar(20) NOT NULL COMMENT '名称',
  PRIMARY KEY (`agid`)
) ENGINE=MyISAM AUTO_INCREMENT=5 DEFAULT CHARSET=utf8 ROW_FORMAT=DYNAMIC COMMENT='代理商';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `agent`
--

LOCK TABLES `agent` WRITE;
/*!40000 ALTER TABLE `agent` DISABLE KEYS */;
INSERT INTO `agent` VALUES (1,'66游艺');
/*!40000 ALTER TABLE `agent` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `cateconfig`
--

DROP TABLE IF EXISTS `cateconfig`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `cateconfig` (
  `cate` int(10) NOT NULL AUTO_INCREMENT COMMENT '统计分类',
  `type` varchar(20) NOT NULL DEFAULT 'daycount' COMMENT '统计类型',
  `name` varchar(20) NOT NULL COMMENT '统计名称',
  `mtype` varchar(2) NOT NULL DEFAULT '+' COMMENT '计算方法',
  `dtype` varchar(100) NOT NULL COMMENT '展示公式(各统计类型用<>包含)',
  `display` varchar(20) NOT NULL DEFAULT 'daycount' COMMENT '显示在后台什么地方',
  PRIMARY KEY (`cate`)
) ENGINE=MyISAM AUTO_INCREMENT=22 DEFAULT CHARSET=utf8 ROW_FORMAT=DYNAMIC COMMENT='统计类型表';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `cateconfig`
--

LOCK TABLES `cateconfig` WRITE;
/*!40000 ALTER TABLE `cateconfig` DISABLE KEYS */;
INSERT INTO `cateconfig` VALUES (14,'daycount','日订单数','+','<4>+<8>','daycount'),(1,'daycount','日注册','+','','daycount'),(8,'daycount','日活跃订单数','+','','daycount'),(2,'daycount','昨注回头','+','','daycount'),(9,'daycount','日活跃付费额','+','','daycount'),(6,'daycount','日活跃','+','','daycount'),(21,'daycount','日总活跃','+','<1>+<6>','daycount'),(15,'daycount','日付费额','+','<5>+<9>','daycount'),(16,'method','注册付费率%','','round(<3>/<1>*100,2)','daycount'),(13,'daycount','日付费用户','+','<3>+<7>','daycount'),(17,'daycount','活跃付费率%','','round(<7>/<6>*100,2)','daycount'),(10,'daycount','累计注册','=','','daycount'),(12,'daycount','累计付费额','=','','daycount'),(4,'daycount','注册订单数','+','','daycount'),(20,'daycount','最高在玩','=','','daycount'),(18,'daycount','日付费率%','','round(<13>/<10>*100,2)','daycount'),(5,'daycount','注册付费额','+','','daycount'),(11,'daycount','累计订单数','=','','daycount'),(3,'daycount','注册付费人数','+','','daycount'),(7,'daycount','日活跃付费','+','','daycount'),(19,'daycount','最高在线','=','','daycount');
/*!40000 ALTER TABLE `cateconfig` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `channel`
--

DROP TABLE IF EXISTS `channel`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `channel` (
  `cnid` int(10) NOT NULL AUTO_INCREMENT COMMENT '渠道id',
  `name` varchar(20) NOT NULL COMMENT '名称',
  PRIMARY KEY (`cnid`)
) ENGINE=MyISAM AUTO_INCREMENT=7 DEFAULT CHARSET=utf8 ROW_FORMAT=FIXED;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `channel`
--

LOCK TABLES `channel` WRITE;
/*!40000 ALTER TABLE `channel` DISABLE KEYS */;
INSERT INTO `channel` VALUES (1,'66游艺自营（Android）');
/*!40000 ALTER TABLE `channel` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `coinprice`
--

DROP TABLE IF EXISTS `coinprice`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `coinprice` (
  `id` int(10) NOT NULL AUTO_INCREMENT COMMENT 'id',
  `siteid` int(10) NOT NULL COMMENT '平台ID',
  `coin` int(10) NOT NULL COMMENT '筹码',
  `rmb` int(10) NOT NULL COMMENT '人民币',
  `ptype` int(1) NOT NULL COMMENT '0其它1苹果',
  `spid` varchar(50) NOT NULL COMMENT '平台商品id',
  `status` int(2) NOT NULL COMMENT '是否下架',
  `gtype` int(2) NOT NULL COMMENT '是否首充',
  PRIMARY KEY (`id`)
) ENGINE=MyISAM AUTO_INCREMENT=20 DEFAULT CHARSET=utf8 ROW_FORMAT=FIXED COMMENT='商城信息表';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `coinprice`
--

LOCK TABLES `coinprice` WRITE;
/*!40000 ALTER TABLE `coinprice` DISABLE KEYS */;
INSERT INTO `coinprice` VALUES (1,0,1000,1000,0,'',0,1),(2,0,5000,5000,0,'',0,0),(3,0,10000,10000,0,'',0,0),(4,0,30000,30000,0,'',0,0),(5,0,50000,50000,0,'',0,0),(6,0,100000,100000,0,'',0,0);
/*!40000 ALTER TABLE `coinprice` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `diamondprice`
--

DROP TABLE IF EXISTS `diamondprice`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `diamondprice` (
  `id` int(10) NOT NULL AUTO_INCREMENT COMMENT 'id',
  `siteid` int(10) NOT NULL COMMENT '平台ID',
  `diamond` int(10) NOT NULL COMMENT '钻石',
  `rmb` int(10) NOT NULL COMMENT '人民币',
  `ptype` int(1) NOT NULL COMMENT '0其它1苹果',
  `spid` varchar(50) NOT NULL COMMENT '平台商品id',
  PRIMARY KEY (`id`)
) ENGINE=InnoDB AUTO_INCREMENT=47 DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `diamondprice`
--

LOCK TABLES `diamondprice` WRITE;
/*!40000 ALTER TABLE `diamondprice` DISABLE KEYS */;
/*!40000 ALTER TABLE `diamondprice` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `emailfull`
--

DROP TABLE IF EXISTS `emailfull`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `emailfull` (
  `efid` int(10) NOT NULL AUTO_INCREMENT COMMENT '邮件id',
  `agid` int(10) NOT NULL COMMENT '代理商id',
  `cnid` int(10) NOT NULL COMMENT '渠道id',
  `title` varchar(50) NOT NULL COMMENT '标题',
  `content` text NOT NULL COMMENT '内容',
  `attachment` text NOT NULL COMMENT '附件',
  `nickname` varchar(50) NOT NULL COMMENT '来自',
  `ptime` int(10) NOT NULL COMMENT '发布时间',
  `expiretime` int(10) NOT NULL COMMENT '过期时间',
  `status` tinyint(1) NOT NULL COMMENT '状态(0正常,1下架)',
  PRIMARY KEY (`efid`)
) ENGINE=MyISAM AUTO_INCREMENT=23 DEFAULT CHARSET=utf8 ROW_FORMAT=DYNAMIC COMMENT='全服邮件模板';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `emailfull`
--

LOCK TABLES `emailfull` WRITE;
/*!40000 ALTER TABLE `emailfull` DISABLE KEYS */;
/*!40000 ALTER TABLE `emailfull` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `functionfbd`
--

DROP TABLE IF EXISTS `functionfbd`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `functionfbd` (
  `id` int(10) NOT NULL AUTO_INCREMENT,
  `function` varchar(20) NOT NULL COMMENT '功能',
  `optype` int(2) NOT NULL DEFAULT '1' COMMENT '操作类型(1逻辑与|2逻辑或)',
  `version` text NOT NULL COMMENT '版本号',
  `siteid` int(10) NOT NULL COMMENT '平台id',
  `remark` varchar(200) NOT NULL COMMENT '备注',
  PRIMARY KEY (`id`)
) ENGINE=InnoDB AUTO_INCREMENT=7 DEFAULT CHARSET=utf8 ROW_FORMAT=COMPACT COMMENT='功能禁用';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `functionfbd`
--

LOCK TABLES `functionfbd` WRITE;
/*!40000 ALTER TABLE `functionfbd` DISABLE KEYS */;
INSERT INTO `functionfbd` VALUES (4,'exchangeno',1,'1.0.0.9',0,'1');
/*!40000 ALTER TABLE `functionfbd` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `gameinfo`
--

DROP TABLE IF EXISTS `gameinfo`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `gameinfo` (
  `gid` tinyint(2) NOT NULL AUTO_INCREMENT COMMENT '游戏id',
  `gname` varchar(20) NOT NULL COMMENT '游戏名称',
  `packname` varchar(20) NOT NULL COMMENT '包名',
  `status` tinyint(1) NOT NULL COMMENT '状态(0不显示1已开放2不开放)',
  `num` int(11) NOT NULL COMMENT '在玩人数',
  `ver` varchar(20) NOT NULL COMMENT '版本号',
  `cover` text NOT NULL COMMENT '封面图片地址',
  `fbdversion` text NOT NULL COMMENT '禁止版本',
  `fbdsite` text NOT NULL COMMENT '禁止平台',
  `sortno` int(11) NOT NULL COMMENT '排序字段',
  PRIMARY KEY (`gid`)
) ENGINE=MyISAM AUTO_INCREMENT=3 DEFAULT CHARSET=utf8 ROW_FORMAT=DYNAMIC COMMENT='游戏配置列表';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `gameinfo`
--

LOCK TABLES `gameinfo` WRITE;
/*!40000 ALTER TABLE `gameinfo` DISABLE KEYS */;
INSERT INTO `gameinfo` VALUES (1,'斗地主','landload',1,100,'1.0','http://image3.5253.com/images/game/00/00/24/12/2412_3.jpeg','1.2,1.3','2,3',1),(2,'梭哈','showhands',1,200,'1.0','http://image3.5253.com/images/game/00/00/24/12/2412_3.jpeg','1.5,1.4','4,5',2);
/*!40000 ALTER TABLE `gameinfo` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `gameinfofbd`
--

DROP TABLE IF EXISTS `gameinfofbd`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `gameinfofbd` (
  `id` int(10) NOT NULL AUTO_INCREMENT,
  `devicetype` int(2) NOT NULL DEFAULT '2' COMMENT '平台类型，1-苹果，2-安卓',
  `version` varchar(10) NOT NULL COMMENT '版本信息',
  `siteid` int(10) NOT NULL COMMENT '平台信息',
  `gametypes` text NOT NULL COMMENT '游戏类型集合',
  `remark` varchar(50) NOT NULL COMMENT '备注',
  PRIMARY KEY (`id`)
) ENGINE=MyISAM AUTO_INCREMENT=12 DEFAULT CHARSET=utf8 ROW_FORMAT=DYNAMIC;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `gameinfofbd`
--

LOCK TABLES `gameinfofbd` WRITE;
/*!40000 ALTER TABLE `gameinfofbd` DISABLE KEYS */;
INSERT INTO `gameinfofbd` VALUES (1,2,'1.0.0.0',501,'[1,3,5]',''),(2,2,'2.0.0.0',502,'[2,3,4,5]',''),(10,2,'1.0.1.0',1001,'[1,5,3,4,6,2]',''),(4,2,'0',0,'[1,5,3,4,6,2,7,8,9]',''),(5,2,'1.0.0.8',1001,'[1,5,3,4,6,2]',''),(11,2,'1.0.0.9',0,'[1,5,3,4,6,2,7,8,9]','');
/*!40000 ALTER TABLE `gameinfofbd` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `horn`
--

DROP TABLE IF EXISTS `horn`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `horn` (
  `hid` int(10) NOT NULL AUTO_INCREMENT COMMENT '喇叭id',
  `name` varchar(20) NOT NULL COMMENT '喇叭发送者昵称',
  `msg` varchar(90) NOT NULL COMMENT '消息',
  `btime` int(10) NOT NULL COMMENT '开始时间',
  `etime` int(10) NOT NULL COMMENT '结束时间',
  `intervals` int(10) NOT NULL COMMENT '发送间隔(秒)',
  `wagid` text NOT NULL COMMENT '代理商白名单',
  `bagid` text NOT NULL COMMENT '代理商黑名单',
  `wcnid` text NOT NULL COMMENT '渠道白名单',
  `bcnid` text NOT NULL COMMENT '渠道黑名单',
  `status` tinyint(2) NOT NULL COMMENT '状态(0开放1禁止)',
  PRIMARY KEY (`hid`)
) ENGINE=MyISAM AUTO_INCREMENT=45 DEFAULT CHARSET=utf8 ROW_FORMAT=DYNAMIC COMMENT='后台发喇叭';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `horn`
--

LOCK TABLES `horn` WRITE;
/*!40000 ALTER TABLE `horn` DISABLE KEYS */;
INSERT INTO `horn` VALUES (40,'','请大家文明娱乐，严禁赌博，如发现有赌博行为，将封停账号，并向公安机关举报！其他内容请关注唯一官方微信：XXXXXXX，其它均为假冒。代理请加微信XXXXXXX、XXXXXXXX\\X',1473648911,1473735313,16,'[]','[]','[]','[]',0),(44,'系统广播00','厕所法规司地方撒旦发生大幅度反对撒法',1484650222,1485341425,30,'[]','[]','[]','[]',0),(42,'系统广播88','测试跑马灯',1484131560,1485341164,10,'[]','[]','[]','[]',0),(43,'系统广播99','测试跑马灯999',1483526801,1485341208,20,'[]','[]','[]','[]',0);
/*!40000 ALTER TABLE `horn` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `levelconfig`
--

DROP TABLE IF EXISTS `levelconfig`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `levelconfig` (
  `level` int(11) NOT NULL,
  `name` varchar(20) NOT NULL COMMENT '名称',
  `needcoin` int(10) NOT NULL COMMENT '所需财富',
  `rmb` int(10) NOT NULL COMMENT 'rmb衡量',
  PRIMARY KEY (`level`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 ROW_FORMAT=COMPACT;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `levelconfig`
--

LOCK TABLES `levelconfig` WRITE;
/*!40000 ALTER TABLE `levelconfig` DISABLE KEYS */;
INSERT INTO `levelconfig` VALUES (1,'学生',1000,1),(2,'硕士',2000,2),(3,'博士',5000,5),(4,'白领',10000,10),(5,'主管',20000,20),(6,'部门经理',50000,50),(7,'区域经理',100000,100),(8,'总监',200000,200),(9,'副总裁',500000,500),(10,'高级副总裁',1000000,1000),(11,'首席执行官',2000000,2000),(12,'董事长',5000000,5000),(13,'干部',10000000,10000),(14,'处长',20000000,20000),(15,'局长',50000000,50000),(16,'副市长',100000000,100000),(17,'市长',120000000,120000),(18,'省长',140000000,140000),(19,'委员',160000000,160000),(20,'总理',180000000,180000),(21,'副主席',200000000,200000),(22,'主席',250000000,250000);
/*!40000 ALTER TABLE `levelconfig` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `mission`
--

DROP TABLE IF EXISTS `mission`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `mission` (
  `msid` int(10) NOT NULL AUTO_INCREMENT COMMENT '任务id',
  `type` int(5) NOT NULL COMMENT '动作类型',
  `status` tinyint(1) NOT NULL COMMENT '0正常1已禁用',
  `auto` tinyint(1) NOT NULL COMMENT '是否自动完成',
  `title` varchar(20) NOT NULL COMMENT '标题',
  `desc` text NOT NULL COMMENT '描述',
  `cate1` int(11) NOT NULL COMMENT '分类1',
  `cate2` int(11) NOT NULL COMMENT '分类2',
  `cate3` int(11) NOT NULL COMMENT '分类3',
  `cate4` int(11) NOT NULL COMMENT '分类4',
  `mtimes` int(11) NOT NULL COMMENT '达到次数',
  `straight` int(11) NOT NULL COMMENT '是否连续',
  `cycle` tinyint(1) NOT NULL DEFAULT '1' COMMENT '周期(1每日2每周3每月)',
  `cycletimes` int(5) NOT NULL DEFAULT '1' COMMENT '可完成次数',
  `icon` varchar(50) NOT NULL COMMENT '图标',
  PRIMARY KEY (`msid`)
) ENGINE=MyISAM AUTO_INCREMENT=11 DEFAULT CHARSET=utf8 ROW_FORMAT=DYNAMIC COMMENT='任务配置表';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `mission`
--

LOCK TABLES `mission` WRITE;
/*!40000 ALTER TABLE `mission` DISABLE KEYS */;
INSERT INTO `mission` VALUES (1,101,0,0,'厚积薄发','游戏对局达到30局',1,0,0,0,30,0,1,1,'missionicon/mission1.png'),(8,104,0,1,'姿势优美','每局至少成功压制对手1次',1,0,0,0,1,0,1,100,'missionicon/mission8.png'),(2,102,0,0,'崭露头角','赢的局数达到30局',1,0,0,0,30,0,1,1,'missionicon/mission2.png'),(6,104,0,0,'姿势强势','压制对手次数累计80次',1,0,0,0,80,0,1,1,'missionicon/mission6_1.png'),(4,102,0,0,'节节高升','连赢6局',1,0,0,0,6,1,1,1,'missionicon/mission4.png'),(5,103,0,0,'狙击高手','打破产20个玩家',1,0,0,0,20,0,1,1,'missionicon/mission5_1.png'),(7,103,0,1,'狙击手','打破产1个玩家',1,0,0,0,1,0,1,40,'missionicon/mission7.png');
/*!40000 ALTER TABLE `mission` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `missionaction`
--

DROP TABLE IF EXISTS `missionaction`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `missionaction` (
  `type` int(4) NOT NULL COMMENT '动作类型',
  `name` varchar(20) NOT NULL COMMENT '动作名称',
  PRIMARY KEY (`type`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 ROW_FORMAT=DYNAMIC COMMENT='任务动作类型配置';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `missionaction`
--

LOCK TABLES `missionaction` WRITE;
/*!40000 ALTER TABLE `missionaction` DISABLE KEYS */;
INSERT INTO `missionaction` VALUES (105,'抽水赢'),(103,'打破产'),(101,'游戏对局'),(102,'游戏胜'),(106,'抽水输'),(104,'压制');
/*!40000 ALTER TABLE `missionaction` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `missionprize`
--

DROP TABLE IF EXISTS `missionprize`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `missionprize` (
  `msid` int(10) NOT NULL COMMENT '任务id',
  `poid` int(10) NOT NULL COMMENT '道具id',
  `qty` int(10) NOT NULL COMMENT '数量',
  PRIMARY KEY (`msid`,`poid`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 ROW_FORMAT=FIXED COMMENT='任务奖励表';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `missionprize`
--

LOCK TABLES `missionprize` WRITE;
/*!40000 ALTER TABLE `missionprize` DISABLE KEYS */;
INSERT INTO `missionprize` VALUES (1,3,5),(3,3,10),(9,3,200),(7,3,5),(4,3,10),(2,3,10),(5,3,50),(8,3,1),(10,3,0),(6,3,50);
/*!40000 ALTER TABLE `missionprize` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `notice`
--

DROP TABLE IF EXISTS `notice`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `notice` (
  `nid` int(10) NOT NULL AUTO_INCREMENT,
  `agid` int(10) NOT NULL COMMENT '代理商id',
  `cnid` int(10) NOT NULL COMMENT '渠道id',
  `status` tinyint(1) NOT NULL COMMENT '状态(0正常,1下架)',
  `title` varchar(50) NOT NULL COMMENT '标题',
  `content` text NOT NULL COMMENT '内容',
  `ptime` int(10) NOT NULL COMMENT '公告时间',
  `expiretime` int(10) NOT NULL COMMENT '过期时间',
  `nickname` varchar(50) NOT NULL COMMENT '来自',
  `mtime` int(10) NOT NULL COMMENT '最后修改时间',
  PRIMARY KEY (`nid`)
) ENGINE=MyISAM AUTO_INCREMENT=17 DEFAULT CHARSET=utf8 ROW_FORMAT=DYNAMIC COMMENT='公告信息';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `notice`
--

LOCK TABLES `notice` WRITE;
/*!40000 ALTER TABLE `notice` DISABLE KEYS */;
INSERT INTO `notice` VALUES (9,0,0,0,'健康游戏忠告','抵制不良游戏，拒绝盗版游戏。注意自我保护，谨防受骗上当。适度游戏益脑，沉迷游戏伤身。合理安排时间，享受健康生活。',1468771200,1596124800,'66游艺管理员',1482718956),(16,0,0,0,'test2','test2javascript:void(22)',1484292540,0,'test2',1484295539),(15,0,0,0,'test1','test1fsdfs1314',1484236800,1484323200,'test1',1484295847);
/*!40000 ALTER TABLE `notice` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `roomcfg`
--

DROP TABLE IF EXISTS `roomcfg`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `roomcfg` (
  `id` int(11) NOT NULL AUTO_INCREMENT COMMENT '序号',
  `gametype` tinyint(2) NOT NULL COMMENT '1斗地主,2梭哈',
  `gamesubtype` int(11) NOT NULL COMMENT '游戏类型',
  `roomid` int(11) NOT NULL COMMENT '房间ID',
  `name` varchar(32) NOT NULL COMMENT '房间名',
  `deal` int(11) NOT NULL DEFAULT '0' COMMENT '发牌类型',
  `consume` int(11) NOT NULL DEFAULT '1' COMMENT '消费类型(1积分，2财富币)',
  `entermin` bigint(20) NOT NULL COMMENT '进入门槛',
  `entermax` bigint(20) NOT NULL COMMENT '进入限制',
  `isopen` int(11) NOT NULL COMMENT '是否开放',
  `basescore` int(11) NOT NULL COMMENT '底分',
  `roomtype` int(11) NOT NULL COMMENT '房间类型(0普通房1私人房2比赛房)',
  `showhandnum` int(11) NOT NULL COMMENT '显示手牌数量',
  `tablenum` int(11) NOT NULL DEFAULT '100' COMMENT '桌子数',
  `marry` int(11) NOT NULL DEFAULT '1' COMMENT '匹配方式(0不匹配，1自动匹配）',
  `limitenter` int(11) NOT NULL DEFAULT '1' COMMENT '限制进入',
  `robot` int(11) NOT NULL DEFAULT '0' COMMENT '是否机器人',
  `showonline` int(11) NOT NULL DEFAULT '0' COMMENT '显示在线倍数',
  `sitdown` int(11) NOT NULL DEFAULT '0' COMMENT '坐下携带',
  `feetype` int(11) NOT NULL DEFAULT '0' COMMENT '台费类型',
  `fee` int(11) NOT NULL DEFAULT '0' COMMENT '台费值',
  `seatnum` int(11) NOT NULL DEFAULT '0' COMMENT '座位数',
  `showtype` int(11) NOT NULL DEFAULT '0' COMMENT '显示类型',
  `showpic` int(11) NOT NULL DEFAULT '1' COMMENT '显示图片',
  `robotmaxscore` bigint(20) NOT NULL DEFAULT '10000000' COMMENT '机器人最多带入',
  `param` varchar(120) DEFAULT '' COMMENT '特殊参数',
  PRIMARY KEY (`id`)
) ENGINE=InnoDB AUTO_INCREMENT=13 DEFAULT CHARSET=utf8 ROW_FORMAT=COMPACT;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `roomcfg`
--

LOCK TABLES `roomcfg` WRITE;
/*!40000 ALTER TABLE `roomcfg` DISABLE KEYS */;
INSERT INTO `roomcfg` VALUES (1,5,1,1,'新手场',1,2,2500,1000000000,1,10,0,0,100,0,1,1,0,1600,2,500,5,0,1,9000000,'{\"cmp\":2,\"look\":1,\"maxrd\":20}'),(2,5,1,2,'初级场',1,2,5000,1000000000,1,20,0,0,100,0,1,1,0,3200,2,500,5,0,2,5000000000,'{\"cmp\":2,\"look\":1,\"maxrd\":20}'),(3,5,1,3,'中级场',1,2,20000,1000000000,1,50,0,0,100,0,1,1,0,8000,2,500,5,0,3,5000000000,'{\"cmp\":3,\"look\":2,\"maxrd\":20}'),(4,5,1,4,'高级场',1,2,50000,1000000000,1,100,0,0,100,0,1,1,0,16000,2,500,5,0,4,5000000000,'{\"cmp\":3,\"look\":2,\"maxrd\":20}'),(5,5,1,5,'大师场',1,2,120000,1000000000,1,500,0,0,100,0,1,1,0,80000,2,400,5,0,5,5000000000,'{\"cmp\":3,\"look\":2,\"maxrd\":20}'),(6,5,1,6,'土豪场',1,2,240000,1000000000,1,1000,0,0,100,0,1,1,0,160000,2,300,5,0,6,5000000000,'{\"cmp\":3,\"look\":2,\"maxrd\":20}'),(8,1,1,2,'初级场',2,2,200,2500,1,10,0,1,100,1,1,0,0,0,2,500,0,0,2,5000000000,''),(9,1,1,3,'中级场',3,2,2000,1000000000,1,100,0,1,100,1,1,0,0,0,2,400,0,0,3,5000000000,''),(10,1,1,4,'高级场',3,2,20000,1000000000,1,1000,0,1,100,1,1,0,0,0,2,300,0,0,4,5000000000,'');
/*!40000 ALTER TABLE `roomcfg` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `scoreprice`
--

DROP TABLE IF EXISTS `scoreprice`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `scoreprice` (
  `id` int(10) NOT NULL AUTO_INCREMENT COMMENT 'id',
  `score` int(10) NOT NULL COMMENT '积分',
  `diamond` int(10) NOT NULL COMMENT '钻石',
  PRIMARY KEY (`id`)
) ENGINE=MyISAM AUTO_INCREMENT=9 DEFAULT CHARSET=utf8 ROW_FORMAT=FIXED;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `scoreprice`
--

LOCK TABLES `scoreprice` WRITE;
/*!40000 ALTER TABLE `scoreprice` DISABLE KEYS */;
INSERT INTO `scoreprice` VALUES (1,200000,100),(8,20000,10),(2,400000,200),(6,20000000,10000),(4,2000000,1000),(5,10000000,5000),(3,1000000,500),(7,100000000,50000);
/*!40000 ALTER TABLE `scoreprice` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `signtype`
--

DROP TABLE IF EXISTS `signtype`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `signtype` (
  `type` int(5) NOT NULL AUTO_INCREMENT COMMENT '标签类型',
  `name` varchar(20) NOT NULL COMMENT '标签名称',
  `remark` varchar(50) NOT NULL COMMENT '备注',
  PRIMARY KEY (`type`)
) ENGINE=InnoDB AUTO_INCREMENT=3 DEFAULT CHARSET=utf8 ROW_FORMAT=COMPACT COMMENT='标签配置表';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `signtype`
--

LOCK TABLES `signtype` WRITE;
/*!40000 ALTER TABLE `signtype` DISABLE KEYS */;
INSERT INTO `signtype` VALUES (1,'大R用户','总总值大于1000'),(2,'中R用户','总充值在500-1000的用户');
/*!40000 ALTER TABLE `signtype` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `site`
--

DROP TABLE IF EXISTS `site`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `site` (
  `siteid` int(10) NOT NULL AUTO_INCREMENT COMMENT '代理渠道联合id(1-500无sdk,501-1000有)',
  `sitename` varchar(20) NOT NULL COMMENT '平台名称',
  `agid` int(10) NOT NULL COMMENT '代理',
  `cnid` int(10) NOT NULL COMMENT '渠道id',
  PRIMARY KEY (`siteid`)
) ENGINE=MyISAM AUTO_INCREMENT=1002 DEFAULT CHARSET=utf8 ROW_FORMAT=DYNAMIC;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `site`
--

LOCK TABLES `site` WRITE;
/*!40000 ALTER TABLE `site` DISABLE KEYS */;
INSERT INTO `site` VALUES (1,'66游艺',1,1);
/*!40000 ALTER TABLE `site` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `sysconfig`
--

DROP TABLE IF EXISTS `sysconfig`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `sysconfig` (
  `sckey` varchar(20) NOT NULL COMMENT 'key',
  `value` text NOT NULL COMMENT '值',
  `mean` text NOT NULL COMMENT '意义',
  `mkfile` tinyint(2) NOT NULL COMMENT '是否生成配置文件',
  `vtype` varchar(20) NOT NULL COMMENT '数值类型',
  PRIMARY KEY (`sckey`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 ROW_FORMAT=DYNAMIC COMMENT='各种配置表';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `sysconfig`
--

LOCK TABLES `sysconfig` WRITE;
/*!40000 ALTER TABLE `sysconfig` DISABLE KEYS */;
INSERT INTO `sysconfig` VALUES ('proomprice','500','私人房日租金',1,'int'),('zdshareinfo','{\"title\":\"中大斗地主\",\"content\":\"原汁原味的线下斗地主模式，“单张发牌、五或七张连发”等多种发牌方式自选。快约上朋友一起组局吧。\",\"link\":\"http://www.zdgame.com\"}','中大分享信息',1,'json'),('bankruptbase','4000','低于此积分算破产',1,'int'),('landbasescore','[{\"deal\":1,\"score\":[10000,50000,100000,500000,1000000,2000000,5000000],\"entermin\":[40000,200000,400000,2000000,4000000,8000000,20000000]},{\"deal\":2,\"score\":[10000,50000,100000,500000,1000000,2000000,5000000],\"entermin\":[80000,400000,800000,4000000,8000000,16000000,40000000]},{\"deal\":3,\"score\":[10000,50000,100000,500000,1000000,2000000,5000000],\"entermin\":[320000,1600000,3200000,16000000,32000000,64000000,160000000]}]','斗地主私人房底分配置\r\n{\"发牌类型\":底注金额数组}',1,'json'),('pltype','[{\"type\":1,\"name\":\"平和型\"},{\"type\":2,\"name\":\"激进型\"},{\"type\":3,\"name\":\"保守型\"}]','机器人打牌风格',0,'json'),('wloginreward','[0,0,10,0,20,50,0]','每周累计登陆奖励',1,'json'),('shbasescore','[500,1000,2000,5000,10000,20000,50000]','梭哈私人房底分配置\r\n[底分....]',1,'json'),('showhandbasescore','[{\"min\":4000,\"max\":500000,\"score\":1024,\"pos\":0},{\"min\":500000,\"max\":1000000,\"score\":4096,\"pos\":1},{\"min\":1000000,\"max\":100000000000,\"score\":16384,\"pos\":2}]','梭哈底注计算系数',0,''),('giveviptax','2','VIP玩家赠送税率',0,'int'),('landenterparam','[{\"deal\":1,\"param\":4},{\"deal\":2,\"param\":8},{\"deal\":3,\"param\":16}]','发牌类型对应系数计算最小带入',0,''),('shminbuyin','[100000,500000,1000000,5000000,10000000,100000000,200000000]','梭哈私人房最小携带\r\n[带....]',1,'json'),('proomfee','[{\"feetype\":0,\"fee\":0,\"desc\":\"不收台费\"},{\"feetype\":1,\"fee\":100,\"desc\":\"每人:底分x1%\"},{\"feetype\":1,\"fee\":200,\"desc\":\"每人:底分x2%\"},{\"feetype\":1,\"fee\":500,\"desc\":\"每人:底分x5%\"},{\"feetype\":1,\"fee\":1000,\"desc\":\"每人:底分x10%\"},{\"feetype\":2,\"fee\":50,\"desc\":\"赢家0.5%\"},{\"feetype\":2,\"fee\":1,\"desc\":\"赢家1%\"},{\"feetype\":2,\"fee\":150,\"desc\":\"赢家1.5%\"},{\"feetype\":2,\"fee\":200,\"desc\":\"赢家2%\"}]','feetype:抽成类型(0不抽1每人按底分百分比抽2赢家所赢百分比抽)',1,'json'),('roll','{\"cost\":{\"poid\":3,\"qty\":100},\"prize\":[{\"rate\":30,\"poid\":3,\"qty\":10,\"imageurl\":\"activity/rollprize3.png\",\"desc\":\"10个元宝\"},{\"rate\":15,\"poid\":3,\"qty\":20,\"imageurl\":\"activity/rollprize3.png\",\"desc\":\"20元宝\"},{\"rate\":10,\"poid\":3,\"qty\":50,\"imageurl\":\"activity/rollprize3.png\",\"desc\":\"50元宝\"},{\"rate\":25,\"poid\":3,\"qty\":100,\"imageurl\":\"activity/rollprize3.png\",\"desc\":\"100元宝\"},{\"rate\":10,\"poid\":2,\"qty\":1,\"imageurl\":\"activity/rollprize2.png\",\"desc\":\"1个财富币\"},{\"rate\":5,\"poid\":2,\"qty\":2,\"imageurl\":\"activity/rollprize2.png\",\"desc\":\"2个财富币\"},{\"rate\":4,\"poid\":2,\"qty\":5,\"imageurl\":\"activity/rollprize2.png\",\"desc\":\"5个财富币\"},{\"rate\":1,\"poid\":2,\"qty\":10,\"imageurl\":\"activity/rollprize2.png\",\"desc\":\"10个财富币\"}]}','大转盘配置',1,'json'),('broadcast','{\"content\":\"为了维护公平的游戏环境，官方严厉打击游戏内双开、伙牌等作弊行为。欢迎各位玩家举报，一经核实严肃处理。官方QQ：3486184150，3303040027；官方微信：QWAAA003。\",\"time\":15}','系统广播',1,'json'),('bankruptvalue','10000','破产补助积分',1,'int'),('texasminbuyin','[100000,500000,1000000,5000000,10000000,500000000,100000000]','德州最小进入',1,'json'),('jumpqueue','2000','百牛插队费用',1,'int'),('speakcost','100','小喇叭多少财富币',1,'int'),('bankruptcount','3','破产补助次数',1,'int'),('richlv','[{\"type\":0,\"name\":\"等级0\"},{\"type\":1,\"name\":\"等级1\"},{\"type\":2,\"name\":\"等级2\"},{\"type\":3,\"name\":\"等级3\"},{\"type\":4,\"name\":\"等级4\"},{\"type\":5,\"name\":\"等级5\"},{\"type\":6,\"name\":\"等级6\"},{\"type\":7,\"name\":\"等级7\"},{\"type\":8,\"name\":\"等级8\"},{\"type\":9,\"name\":\"等级9\"},{\"type\":10,\"name\":\"等级10\"}]','财富等级',0,'json'),('texasbasescore','[1000,5000,10000,50000,100000,500000,1000000]','德州底分配置',1,'json'),('cloginreward','[100,100,100,100,100,100,200]','连续登陆奖励',1,'json'),('bankrupttype','4','破产货币4积分2财富币',1,'int'),('givetax','0','普通玩家赠送税率',0,'int'),('safeproblem','[{\"type\":1,\"name\":\"你妈妈的名字是什么?\"},{\"type\":2,\"name\":\"我的出生地在哪?\"},{\"type\":3,\"name\":\"我的小学名字叫什么?\"},{\"type\":4,\"name\":\"我的宠物名字叫什么?\"},{\"type\":5,\"name\":\"我最喜欢的人的名字?\"},{\"type\":6,\"name\":\"我最喜欢的颜色是什么?\"}]','安全问题设置\r\n{\"数字\":\"问题描述\"}',1,'json'),('banker','[{\"type\":1,\"name\":\"中国银行\"},{\"type\":2,\"name\":\"中国工商银行\"},{\"type\":3,\"name\":\"中国建设银行\"},{\"type\":4,\"name\":\"中国农业银行\"},{\"type\":5,\"name\":\"招商银行\"},{\"type\":6,\"name\":\"交通银行\"},{\"type\":7,\"name\":\"中信银行\"},{\"type\":8,\"name\":\"兴业银行\"},\r\n{\"type\":9,\"name\":\"中国民生银行\"},\r\n{\"type\":10,\"name\":\"广东发展银行\"},\r\n{\"type\":11,\"name\":\"中国邮政储蓄银行\"},\r\n{\"type\":12,\"name\":\"中国光大银行\"},\r\n{\"type\":13,\"name\":\"上海浦东发展银行\"},{\"type\":14,\"name\":\"北京农村商业银行\"},\r\n{\"type\":15,\"name\":\"华夏银行\"},\r\n{\"type\":16,\"name\":\"平安银行\"},\r\n{\"type\":17,\"name\":\"渤海银行\"},\r\n{\"type\":18,\"name\":\"北京银行\"},\r\n{\"type\":19,\"name\":\"南京市商业银行\"},\r\n{\"type\":20,\"name\":\"宁波银行\"},{\"type\":21,\"name\":\"东亚银行\"}]','银行描述\r\n{\"数字\":\"银行描述\"}',1,'json'),('signgamecount','10','每日签到游戏局数',1,'int');
/*!40000 ALTER TABLE `sysconfig` ENABLE KEYS */;
UNLOCK TABLES;
/*!40103 SET TIME_ZONE=@OLD_TIME_ZONE */;

/*!40101 SET SQL_MODE=@OLD_SQL_MODE */;
/*!40014 SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS */;
/*!40014 SET UNIQUE_CHECKS=@OLD_UNIQUE_CHECKS */;
/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
/*!40111 SET SQL_NOTES=@OLD_SQL_NOTES */;

-- Dump completed on 2017-02-04 14:39:57
