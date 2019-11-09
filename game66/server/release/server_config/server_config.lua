--*********************************************
--************ 服务器配置
--*********************************************

--数据库配置
local database_config={};
--示例:dbConfig[ID]={ip="数据库IP地址",port="数据库端口",user="数据库用户名",passwd="数据库用户密码",dbname="数据库名称"}; 
database_config[0]={ip="127.0.0.1",port="3306",user="root",passwd="game123456",dbname="chess"};
database_config[1]={ip="127.0.0.1",port="3306",user="root",passwd="game123456",dbname="chess_sysdata"};
database_config[2]={ip="127.0.0.1",port="3306",user="root",passwd="game123456",dbname="chess_mission"};
database_config[3]={ip="127.0.0.1",port="3306",user="root",passwd="game123456",dbname="chess_log"};

--Redis配置
local redisConfig={};
--示例:redisConfig[ID]={ip="Redis IP地址",port="Redis端口"}; 
redisConfig[0]={ip="127.0.0.1",port="13000"};
redisConfig[1]={ip="127.0.0.1",port="13000"};
--robotnum
local robotNumConfig={1000,560,390,180,250,314,280,280,60,230,80};

-- 游戏服务器配置
function game_config(serverID,gameConfig)
	load_redis_config(serverID,gameConfig);
	load_db_config(serverID,gameConfig);
	return true
end
-- 大厅服务器配置
function lobby_config(serverID,lobbyConfig)
	lobbyConfig:SetNeedPassWD(1);
	load_redis_config(serverID,lobbyConfig);
	load_db_config(serverID,lobbyConfig);
	return true;
end
-- 机器人服务器配置
function robot_config(serverID,robotConfig)
	load_redis_config(serverID,robotConfig);
	load_db_config(serverID,robotConfig);
	return true;
end


-- 加载数据库
function load_db_config(serverID,serviceConfig)
	for k, v in pairs(database_config) do
		local cfg = serviceConfig:GetDBConf(k);
		cfg:SetDBInfo(v.ip, v.port, v.dbname, v.user, v.passwd);	
	end
end
-- 加载redis
function load_redis_config(serverID, serviceConfig)
	for k, v in pairs(redisConfig) do
		local redis = serviceConfig:GetRedisConf(k);
		redis:SetRedisHost(v.ip, v.port, k);
	end
end
-- 每次检测登陆机器人数量
function loginrobotnum(gameType)
	return robotNumConfig[gameType+1];
end
function bainiubankertime()
    return 6;
end
-- 斗牛庄家通杀概率
function bainiubankerwin()
    return 3500; 
end
function niuniurobotwin()
    return 2000;
end
function dicebankertime()
    return 6;
end



