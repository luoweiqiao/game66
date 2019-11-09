
alter table chess_sysdata.roomcfg modify column param varchar(256) DEFAULT '' COMMENT '特殊参数';


alter table chess_sysdata.roomcfg add column robotminscore bigint(20) DEFAULT 0 NOT NULL COMMENT '机器人最小带入' after showpic;


alter table chess_sysdata.roomcfg add column jettonminscore bigint(20) DEFAULT 0 NOT NULL COMMENT '最小下注' after robotmaxscore;

alter table chess_sysdata.roomcfg add column exitchip int DEFAULT 0 NOT NULL COMMENT '退场筹码' after jettonminscore;
alter table chess_sysdata.roomcfg add column uproom tinyint DEFAULT 0 NOT NULL COMMENT '是否赶场，0不敢场 1强制赶场 2非强制敢场' after jettonminscore;
