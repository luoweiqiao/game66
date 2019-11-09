

alter table chess_sysdata.robotonline add online13 int not Null DEFAULT 0;
alter table chess_sysdata.robotonline add online14 int not Null DEFAULT 0;
alter table chess_sysdata.robotonline add online15 int not Null DEFAULT 0;

alter table chess_sysdata.robotonline add online16 int not Null DEFAULT 0;
alter table chess_sysdata.robotonline add online17 int not Null DEFAULT 0;


alter table chess_sysdata.robotonline add logintype int not Null DEFAULT 0 COMMENT '登录类型 0 全天登录 1 按照时段登录';
alter table chess_sysdata.robotonline add entertime int not Null DEFAULT 0 COMMENT '开始时间';
alter table chess_sysdata.robotonline add leavetime int not Null DEFAULT 0 COMMENT '结束时间';
alter table chess_sysdata.robotonline add column roomid int DEFAULT 0 NOT NULL COMMENT '房间id' after gametype;



